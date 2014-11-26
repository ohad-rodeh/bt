/**************************************************************/
/*
 * Copyright (c) 2014, Ohad Rodeh, IBM Research
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met: 
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution. 
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies, 
 * either expressed or implied, of IBM Research.
 * 
 */
/**************************************************************/
/**********************************************************************/
/* OC_BPT_OP_VALIDATE_CLONES.C  
 *
 * Validate that a set of clones of a tree are correct. 
 */
/**********************************************************************/
/* Validate each tree separately and then check that the free-space
 * counts are correct
 */
/**********************************************************************/
#include <string.h>

#include "oc_bpt_int.h"
#include "oc_bpt_label.h"
#include "oc_bpt_op_validate.h"
#include "oc_bpt_op_validate_clones.h"
#include "oc_bpt_nd.h"

/* Label used to denote a node that has already been touched
 * The assumption is that the graph has less than [TOUCHED]
 * nodes. 
 */
#define TOUCHED (1000000)

/**********************************************************************/

static void inc_b(struct Oc_wu *wu_p,
                  struct Oc_bpt_state *s_p,
                  Oc_bpt_node *node_p);
static bool compare_b(struct Oc_wu *wu_p,
                      struct Oc_bpt_state *s_p,
                      Oc_bpt_node *node_p);
static bool check_fs_b(
    struct Oc_wu *wu_p,
    int n_clones,
    struct Oc_bpt_state *st_array[]);

/**********************************************************************/

// Erase all previous labels
static void erase_labels_b(struct Oc_wu *wu_p,
                           struct Oc_bpt_state *s_p,
                           Oc_bpt_node *node_p)
{
    int i, num_entries;
    int *node_label_p;
        
    // Erase this node's label
    node_label_p = oc_bpt_label_get(wu_p, node_p);
    *node_label_p = 0;

    num_entries = oc_bpt_nd_num_entries(s_p, node_p);
    
    // If it is an index node then recurse into the childern
    if (!oc_bpt_nd_is_leaf(s_p, node_p)) {
        for (i=0; i<num_entries; i++)
        {
            Oc_bpt_node *child_p;
            struct Oc_bpt_key *dummy_key_p;
            uint64 addr;
            
            oc_bpt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &addr); 
            child_p = oc_bpt_nd_get_for_read(wu_p, s_p, addr);
            erase_labels_b(wu_p, s_p, child_p);
            oc_bpt_nd_release(wu_p, s_p, child_p);            
        }
    }
}

/* count how many times a node is pointed-to.
 * Recurse through a tree.
 * Perform:
 *    - if a node's label is zero, then set to one
 *      and recurse into it.
 *    - if a node's label is greater than one, then
 *      increment and do not recurse.
 */
static void inc_b(struct Oc_wu *wu_p,
                  struct Oc_bpt_state *s_p,
                  Oc_bpt_node *node_p)
{
    int *node_label_p, num_entries, i;
    
    node_label_p = oc_bpt_label_get(wu_p, node_p);    

    if (*node_label_p > 0) {
        /* This node has already been labeled.
         * increment the counter by one. 
         */
        *node_label_p = 1 + *node_label_p;
        return;
    }

    // This is the first time anyone has reached this node

    // set the label to 1
    *node_label_p = 1;

    // if it is an index node then recurse down
    if (!oc_bpt_nd_is_leaf(s_p, node_p))
    {
        num_entries = oc_bpt_nd_num_entries(s_p, node_p);
        for (i=0; i<num_entries; i++) {
            Oc_bpt_node *child_p;
            struct Oc_bpt_key *dummy_key_p;
            uint64 addr;
            
            oc_bpt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &addr); 
            child_p = oc_bpt_nd_get_for_read(wu_p, s_p, addr);
            inc_b(wu_p, s_p, child_p);
            oc_bpt_nd_release(wu_p, s_p, child_p);
        }
    }
}    


// Compare the computed ref-count with the free-space count
static bool compare_b(struct Oc_wu *wu_p,
                      struct Oc_bpt_state *s_p,
                      Oc_bpt_node *node_p)
{
    int i;
    int *node_label_p, fs_refcount;

    // Check if the count for this node is correct
    node_label_p = oc_bpt_label_get(wu_p, node_p);
    fs_refcount = s_p->cfg_p->fs_get_refcount(wu_p, node_p->disk_addr);    
    if (*node_label_p != fs_refcount) {
        printf("label=%d  fs_refcount=%d\n",
               (*node_label_p), fs_refcount); 
        return FALSE;
    }

    // If it is an index node then recurse into the childern
    if (!oc_bpt_nd_is_leaf(s_p, node_p))
    {
        int num_entries = oc_bpt_nd_num_entries(s_p, node_p);
        for (i=0; i<num_entries; i++)
        {
            Oc_bpt_node *child_p;
            struct Oc_bpt_key *dummy_key_p;
            uint64 addr;
            bool rc;
            
            oc_bpt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &addr); 
            child_p = oc_bpt_nd_get_for_read(wu_p, s_p, addr);
            rc = compare_b(wu_p, s_p, child_p);
            oc_bpt_nd_release(wu_p, s_p, child_p);

            if (!rc)
                return FALSE;
        }
    }

    return TRUE;
}

/**********************************************************************/

static bool check_fs_b(
    struct Oc_wu *wu_p,
    int n_clones,
    struct Oc_bpt_state *st_array[])
{
    int i;

    /* phase 1: zero out the labels
     * phase 2: count how many times a node is pointed-to.
     *       to do this, recurse through each tree. 
     *       Perform:
     *         - if a node's label is zero, then set to one
     *           and recurse into it.
     *         - if a node's label is greater than one, then
     *           increment and do not recurse.
     * phase 3: traverse all the trees and check that the
     *       label is equal to the free-space count.
     *       if not, print-out the incorrect case. 
     */

    // phase 0, erase all previous labels
    for (i=0; i < n_clones; i++) {
        struct Oc_bpt_state *s_p = st_array[i];
        
        if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) > 0)
            erase_labels_b(wu_p,
                           s_p,
                           s_p->root_node_p);
    }
    
    // phase 1, increment the counters
    for (i=0; i < n_clones; i++) {
        struct Oc_bpt_state *s_p = st_array[i];
        
        if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) > 0)
            inc_b(wu_p,
                  s_p,
                  s_p->root_node_p);
    }
    
    
    /* phase 2, recurse through the trees and compare lables
     *    against free-space counters.
     */
    for (i=0; i < n_clones; i++) {
        struct Oc_bpt_state *s_p = st_array[i];
        bool rc;
        
        if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) > 0) {
            rc = compare_b(wu_p,
                           s_p,
                           s_p->root_node_p);
            if (!rc) return FALSE;
        }
    }
    
    return TRUE;
}

/**********************************************************************/


bool oc_bpt_op_validate_clones_b(
    struct Oc_wu *wu_p,
    int n_clones,
    struct Oc_bpt_state *st_array[])
{
    int i;
    bool rc;
    
    // First validate each tree seperately
    for (i=0; i<n_clones; i++) {
        rc = oc_bpt_op_validate_b(wu_p, st_array[i]);
        if (!rc) return FALSE;
    }

    // printf("checking free-space counters\n"); fflush(stdout);

    // now check the free-space counts
    oc_bpt_label_init(10000);
    rc = check_fs_b(wu_p, n_clones, st_array);

    oc_bpt_label_free();

    return rc;
}

/**********************************************************************/
