/**************************************************************/
/*
 * Copyright (c) 2014-2015, Ohad Rodeh, IBM Research
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
/* OC_XT_OP_VALIDATE.C  
 *
 * Validate a b+-tree
 */
/**********************************************************************/
/* The algorithm is: decend through the tree and very range invariants.
 * If an index node has key K pointing to a subnode N then N must include
 * keys only between K and the key after it. 
 */
/**********************************************************************/
#include <string.h>
#include <alloca.h>

#include "oc_xt_int.h"
#include "oc_xt_op_validate.h"
#include "oc_xt_nd.h"
/**********************************************************************/
typedef struct Range {
    struct Oc_xt_key *lo_p, *hi_p; // place to store the keys
} Range;


/**********************************************************************/
// validate the internal structure of a node
static bool validate_single_node(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    int i, num_entries;

    num_entries = oc_xt_nd_num_entries(s_p, node_p);
    if (oc_xt_nd_is_root(s_p, node_p) &&
        !oc_xt_nd_is_leaf(s_p, node_p)) {
        if (num_entries == 1)
            return FALSE;
    }
    
    if (num_entries < s_p->cfg_p->min_num_ent && !oc_xt_nd_is_root(s_p, node_p))
        return FALSE;

    // check that the extents are arranged in ascending order
    if (!oc_xt_nd_is_leaf(s_p, node_p)) {
        struct Oc_xt_key *key_p, *prev_key_p;
        
        prev_key_p = oc_xt_nd_get_kth_key(s_p, node_p, 0);
        for (i=1; i<num_entries; i++) {
            key_p = oc_xt_nd_get_kth_key(s_p, node_p, i);
            
            if (s_p->cfg_p->key_compare(prev_key_p, key_p) != 1)
                return FALSE;
            prev_key_p = key_p;
        }
    }
    else {
        // a leaf node, check that the extents are correctly arranged
        struct Oc_xt_key *key_p, *prev_key_p;
        struct Oc_xt_rcrd *rcrd_p, *prev_rcrd_p;
        
        oc_xt_nd_leaf_get_kth(s_p, node_p, 0, &prev_key_p, &prev_rcrd_p);
        for (i=1; i<num_entries; i++) {
            oc_xt_nd_leaf_get_kth(s_p, node_p, i, &key_p, &rcrd_p);
            
            if (s_p->cfg_p->rcrd_compare(
                    prev_key_p, prev_rcrd_p,
                    key_p, rcrd_p) != OC_XT_CMP_SML)
                return FALSE;
            prev_key_p = key_p;
            prev_rcrd_p = rcrd_p;
        }
    }

    return TRUE;
}


static bool validate_node(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    Range *range_p)
{
    struct Oc_xt_key *lo_p, *hi_p;

    lo_p = oc_xt_nd_min_key(s_p, node_p);
    hi_p = (struct Oc_xt_key *) alloca(s_p->cfg_p->key_size);
    oc_xt_nd_max_ofs(s_p, node_p, hi_p);

    if (!validate_single_node(s_p, node_p)) {
        printf("   // node %s did not pass validation\n",
               oc_xt_nd_string_of_node(s_p,node_p));
        printf("   // num_entries=%d\n", oc_xt_nd_num_entries(s_p, node_p));
        return FALSE;
    }
    
    // Check that this node falls under the range dictated from above
    if (!oc_xt_nd_is_root(s_p, node_p)) {
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
    
    if (oc_xt_nd_is_leaf(s_p, node_p)) {
        return TRUE;
    }
    else {
        // An index node, recurse through its children
        int i;
        int num_entries = oc_xt_nd_num_entries(s_p, node_p);
        Range child_range;
        uint64 child_addr = 0;
        Oc_xt_node *child_node_p;
        bool rc;
        
        for (i=0; i< num_entries; i++) {
            oc_xt_nd_index_get_kth(s_p, node_p, i,
                                    &child_range.lo_p,
                                    &child_addr);

            // the last key is bounded from above by the dictated range 
            if (i == num_entries-1) {
                child_range.hi_p = range_p->hi_p;
            } else {
                uint64 dummy;
                
                oc_xt_nd_index_get_kth(s_p, node_p, i+1,
                                        &child_range.hi_p,
                                        &dummy);
            }
            
            child_node_p = s_p->cfg_p->node_get(wu_p, child_addr);
            rc = validate_node(wu_p, s_p, child_node_p, &child_range);
            s_p->cfg_p->node_release(wu_p, child_node_p);
            if (FALSE == rc)
                return rc;
        }
        return TRUE;
    }
}

bool oc_xt_op_validate_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p)
{
    Range r;
    bool rc;
    
    if (oc_xt_nd_num_entries(s_p, s_p->root_node_p) == 0) {
        rc = TRUE;
    }
    else {
        r.lo_p = oc_xt_nd_min_key(s_p, s_p->root_node_p);
        r.hi_p = NULL;
        rc = validate_node(wu_p, s_p, s_p->root_node_p, &r);
    }

    return rc;
}
