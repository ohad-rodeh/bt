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
/* OC_XT_UTL.H
 *
 * Some utilities. 
 */
/**********************************************************************/
#include <string.h>
#include <alloca.h>

#include "oc_utl.h"
#include "oc_xt_utl.h"
#include "oc_xt_nd.h"
/**********************************************************************/

// Check if a node is fully covered by the range [min_key_p] .. [max_key_p]
bool oc_xt_utl_covered(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key* min_key_p,
    struct Oc_xt_key* max_key_p)
{
    struct Oc_xt_key *nd_min_key_p;
    struct Oc_xt_key *nd_max_key_p;

    nd_min_key_p = oc_xt_nd_min_key(s_p, node_p);
    nd_max_key_p = (struct Oc_xt_key *)alloca(s_p->cfg_p->key_size);
    oc_xt_nd_max_ofs(s_p, node_p, nd_max_key_p);

    return (s_p->cfg_p->key_compare(min_key_p, nd_min_key_p) >= 0 &&
            s_p->cfg_p->key_compare(nd_max_key_p, max_key_p) >= 0);
}

/* Delete a sub-tree rooted at [addr].
 * 
 * Recursive decent through the tree. Remove the leaves and perform
 * the user-defined "data_release" function for all data. Deallocate all
 * internal nodes. 
 */
void oc_xt_utl_delete_subtree_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    /* recurse down and then delete it on the way back up.
     */
    if (!oc_xt_nd_is_leaf(s_p, node_p)) {
        // An index node, recurse through its children
        int i;
        int num_entries = oc_xt_nd_num_entries(s_p, node_p);
        Oc_xt_node *child_node_p;
        struct Oc_xt_key *dummy_key_p;
        uint64 child_addr;
        
        for (i=0; i< num_entries; i++) {
            oc_xt_nd_index_get_kth(s_p, node_p, i,
                                    &dummy_key_p,
                                    &child_addr);
            child_node_p = s_p->cfg_p->node_get(wu_p, child_addr);
            oc_xt_utl_delete_subtree_b(wu_p, s_p, child_node_p);
        }
    }
    
    // Delete this node
    oc_xt_nd_delete(wu_p, s_p, node_p);
}

/* Delete all the key-value pairs in the tree. 
 * Leave the root node empty but do not delete it. 
 */
void oc_xt_utl_delete_all_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p)
{
    int num_entries = oc_xt_nd_num_entries(s_p, s_p->root_node_p);

    if (!oc_xt_nd_is_leaf(s_p, s_p->root_node_p)) {
        // This is a full fledged tree. Remove all the sub-trees. 
        int i;
        Oc_xt_node *child_node_p;
        struct Oc_xt_key *dummy_key_p;
        uint64 child_addr;
        
        for (i=0; i< num_entries; i++) {
            oc_xt_nd_index_get_kth(s_p, s_p->root_node_p, i,
                                    &dummy_key_p,
                                   &child_addr);
            child_node_p = s_p->cfg_p->node_get(wu_p, child_addr);
            oc_xt_utl_delete_subtree_b(wu_p, s_p, child_node_p);
        }
    }
    
    // Remove all the entries from the root node
    oc_xt_nd_remove_range_of_entries(wu_p, s_p,
                                     s_p->root_node_p,
                                     0, num_entries - 1);
}

/**********************************************************************/

