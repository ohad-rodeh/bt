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
/* OC_BPT_OP_VALIDATE.C  
 *
 * Validate a b+-tree
 */
/**********************************************************************/
/* The algorithm is: decend through the tree and verify range invariants.
 * If an index node has key K pointing to a subnode N then N must include
 * keys only between K and the key after it. 
 */
/**********************************************************************/
#include <string.h>

#include "oc_bpt_int.h"
#include "oc_bpt_op_validate.h"
#include "oc_bpt_nd.h"
/**********************************************************************/
typedef struct Range {
    struct Oc_bpt_key *lo_p, *hi_p; // place to store the keys
} Range;


/**********************************************************************/
// validate the internal structure of a node
bool validate_single_node(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p)
{
    int i, num_entries;
    struct Oc_bpt_key *key_p, *prev_key_p;

    num_entries = oc_bpt_nd_num_entries(s_p, node_p);
    if (oc_bpt_nd_is_root(s_p, node_p) &&
        !oc_bpt_nd_is_leaf(s_p, node_p)) {
        if (num_entries == 1)
            return FALSE;
    }
    
    if (num_entries < (int)s_p->cfg_p->min_num_ent && !oc_bpt_nd_is_root(s_p, node_p))
        return FALSE;

    // check that the keys are arrange in ascending order
    prev_key_p = oc_bpt_nd_get_kth_key(s_p, node_p, 0);
    for (i=1; i<num_entries; i++) {
        key_p = oc_bpt_nd_get_kth_key(s_p, node_p, i);

        if (s_p->cfg_p->key_compare(prev_key_p, key_p) != 1)
            return FALSE;
        prev_key_p = key_p;
    }

    return TRUE;
}


static bool validate_node(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    Range *range_p)
{
    struct Oc_bpt_key *lo_p, *hi_p;

    lo_p = oc_bpt_nd_min_key(s_p, node_p);
    hi_p = oc_bpt_nd_max_key(s_p, node_p);

    if (!validate_single_node(s_p, node_p)) {
        char lo[10], hi[10];

        s_p->cfg_p->key_to_string(lo_p, lo, 10);
        s_p->cfg_p->key_to_string(hi_p, hi, 10);
        printf("   // node did not pass validation lo_p=%s hi=%s\n", lo, hi);
        printf("   // num_entries=%d\n", oc_bpt_nd_num_entries(s_p, node_p));
        return FALSE;
    }
    
    // Check that this node falls under the range dictated from above
    if (!oc_bpt_nd_is_root(s_p, node_p)) {
        if (s_p->cfg_p->key_compare(range_p->lo_p, lo_p) == -1) {
            char range_lo[10], lo[10];
            
            s_p->cfg_p->key_to_string(range_p->lo_p, range_lo, 10);
            s_p->cfg_p->key_to_string(lo_p, lo, 10);
            printf ("    // range violation range->lo_p=%s > lo=%s\n", range_lo, lo);
            return FALSE;
        }
        if (range_p->hi_p != NULL &&
            s_p->cfg_p->key_compare(hi_p, range_p->hi_p) != 1) {
            char range_hi[10], hi[10];
            
            s_p->cfg_p->key_to_string(range_p->hi_p, range_hi, 10);
            s_p->cfg_p->key_to_string(hi_p, hi, 10);
            printf ("    // range violation hi_p=%s >= range_p->hi_p=%s \n",
                    hi, range_hi);
            return FALSE;
        }
    }
    
    if (oc_bpt_nd_is_leaf(s_p, node_p)) {
        return TRUE;
    }
    else {
        // An index node, recurse through its children
        int i;
        int num_entries = oc_bpt_nd_num_entries(s_p, node_p);
        Range child_range;
        uint64 child_addr = 0;
        Oc_bpt_node *child_node_p;
        bool rc;
        
        for (i=0; i< num_entries; i++) {
            oc_bpt_nd_index_get_kth(s_p, node_p, i,
                                    &child_range.lo_p,
                                    &child_addr);

            // the last key is bounded from above by the dictated range 
            if (i == num_entries-1) {
                child_range.hi_p = range_p->hi_p;
            } else {
                uint64 dummy;
                
                oc_bpt_nd_index_get_kth(s_p, node_p, i+1,
                                        &child_range.hi_p,
                                        &dummy);
            }
            
            child_node_p = oc_bpt_nd_get_for_read(wu_p, s_p, child_addr);
            rc = validate_node(wu_p, s_p, child_node_p, &child_range);
            oc_bpt_nd_release(wu_p, s_p, child_node_p);
            if (FALSE == rc)
                return rc;
        }
        return TRUE;
    }
}

bool oc_bpt_op_validate_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p)
{
    Range r;
    bool rc;
    
    if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) == 0) {
        rc = TRUE;
    }
    else {
        r.lo_p = oc_bpt_nd_min_key(s_p, s_p->root_node_p);
        r.hi_p = NULL;
        rc = validate_node(wu_p, s_p, s_p->root_node_p, &r);
    }

    return rc;
}
