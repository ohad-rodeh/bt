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
/* OC_BPT_OP_INSERT_RANGE.C
 *
 * Insert an array of (key,data) pairs into a b-tree
 */
/**********************************************************************/
/*
 * Insert an array of (key,data) pairs. The caller must provide a dense
 * array of keys such that there are no spaces in between them. 
 * 
 * The implementation uses a mini-insert function that can
 * atomically insert a limited-sized array into a single leaf. 
 * A loop around mini-insert takes care of long ranges. 
 * 
 * Insert-range does not provide an atomic insert of a whole range. The
 * caller is expected to isolate on-going operations so as to get
 * atomicity. The expected use case is an OSD-write into an s-node. In
 * this case the state-machine component is expected to isolate
 * concurrent overlapping reads and writes. 
 */
/**********************************************************************/
#include <alloca.h>
#include <string.h>

#include "oc_utl.h"
#include "oc_bpt_int.h"
#include "oc_bpt_trace.h"
#include "oc_bpt_nd.h"
#include "oc_bpt_op_insert_range.h"

/**********************************************************************/
// prototypes
static int chop(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_key *hi_key_p);
static Oc_bpt_node *lookup_key_in_index_node(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    int *idx_po,
    struct Oc_bpt_key *hi_key_pio);
static void correct_min_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p);
static void update_hi_key(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *hi_key_p);
static int fill_single_leaf_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_data *data_array,
    int *nkeys_inserted_po);    
/**********************************************************************/

/* divide an array of consecutive keys into those less than [hi_key_p]
 * and those greater or equal to [hi_key_p].
 *
 * return the number of keys that are strictly smaller than [hi_key_p].
 */
static int chop(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_key *hi_key_p)
{
    int i;

    oc_utl_debugassert(length > 0);
    
    // simple optimization. check if the high-key is larger than all the keys
    // in the array.
    if (s_p->cfg_p->key_compare(oc_bpt_nd_key_array_kth(s_p, key_array, length-1),
                                hi_key_p) == 1)
        return length;
    
    for (i=0; i<length; i++)
        if (s_p->cfg_p->key_compare(oc_bpt_nd_key_array_kth(s_p, key_array, i),
                                    hi_key_p) < 1)
            return i;

    // not possible, all the keys were smaller than [hi_key_p], the optimization 
    // should have caught this
    oc_utl_assert(0);
    return length;
}

static Oc_bpt_node *lookup_key_in_index_node(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    int *idx_po,
    struct Oc_bpt_key *hi_key_pio)
{
    uint64 child_addr;
    
    child_addr = oc_bpt_nd_index_lookup_key(wu_p, s_p, node_p, key_p,
                                            NULL, idx_po);
    if (*idx_po < oc_bpt_nd_num_entries(s_p, node_p)-1) {
        // if there is a higher key then update the high-bound
        struct Oc_bpt_key *hi_key_p;

        hi_key_p = oc_bpt_nd_get_kth_key(s_p, node_p, *idx_po + 1);
//        printf("hi-key = %s idx=%d\n", oc_bpt_nd_string_of_key(s_p, hi_key_p), idx);
//        printf("node=%s #entries=%d\n",
//               oc_bpt_nd_string_of_node(s_p, node_p),
//               oc_bpt_nd_num_entries(s_p, node_p));
        memcpy((char*)hi_key_pio, hi_key_p, s_p->cfg_p->key_size);
    }
               
    return oc_bpt_nd_get_for_write(wu_p, s_p, child_addr, node_p, *idx_po);
}

static void correct_min_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p)
{
    switch (s_p->cfg_p->key_compare(
                key_p,
                oc_bpt_nd_min_key(s_p, node_p))) {
    case 0:
    case -1:
        // nothing to do
        break;
    case 1:
        /* the key is smaller than all existing keys.
         *  - replace the minimal key
         */
        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REPLACE_MIN_IN_INDEX, wu_p, "");
        oc_bpt_nd_index_replace_min_key(wu_p, s_p, node_p, key_p);
        break;
    }
}

static void update_hi_key(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *hi_key_p)
{
    struct Oc_bpt_key *min_key_p = oc_bpt_nd_min_key(s_p, node_p);
    
    switch (s_p->cfg_p->key_compare(hi_key_p, min_key_p))
    {
    case -1:
        // the minimal key is smaller that the hi-bound, update the hi-bound
        memcpy((char*)hi_key_p, min_key_p, s_p->cfg_p->key_size);
    case 0:
    case 1:
        // do nothing
        break;
    }
}

/* An bounded insert that fills up a single leaf.
 *
 * The algorithm is like inserting a single entry. Descend through the
 * tree. If a node on the path is completely full then split it. When
 * you reach the leaf fill it up. If it becomes full then split it. 
 *
 * Return
 *  1. the number of overwritten keys
 *  2. update the counter of inserted keys [nkeys_inserted_po]
 *
 * note: it is possible that the function will be unable to add entries to a leaf.
 *   this can happen if the leaf is full. 
 */
static int fill_single_leaf_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_data *data_array,
    int *nkeys_inserted_po)
{
    Oc_bpt_node *father_p, *child_p;
    struct Oc_bpt_key *min_key_p, *hi_bound_key_p;
    int level = 0, rc=0;
    
    oc_bpt_trace_wu_lvl(
        3, OC_EV_BPT_INSERT_RNG, wu_p,
        "fill_single_leaf min_key=%s len=%d",
        oc_bpt_nd_string_of_key(s_p, oc_bpt_nd_key_array_kth(s_p, key_array, 0)), length);
    
    min_key_p = oc_bpt_nd_key_array_kth(s_p, key_array, 0);
    oc_bpt_nd_get_for_write(wu_p, s_p, s_p->root_node_p->disk_addr,
                            NULL/*no father to update on cow*/,0);
    
    // setup the high bound, initially, it poses no restrictions
    hi_bound_key_p = (struct Oc_bpt_key*)alloca(s_p->cfg_p->key_size);
    s_p->cfg_p->key_inc(oc_bpt_nd_key_array_kth(s_p, key_array, length-1),
                        hi_bound_key_p);
    
    if (oc_bpt_nd_is_full(s_p, s_p->root_node_p)) {
        // the root is full, split it and continue
        oc_bpt_nd_split_root(wu_p, s_p, s_p->root_node_p);
    }
    
    if (oc_bpt_nd_is_leaf(s_p, s_p->root_node_p))
    {
        // T has only a root node N
        // insert as much as possible into it.
        rc = oc_bpt_nd_insert_array_into_leaf(
            wu_p, s_p,
            s_p->root_node_p,
            length,
            key_array, data_array, nkeys_inserted_po);
        
        oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);
        return rc;
    }
    
    // T is a non-trivial tree
    correct_min_key(wu_p, s_p, s_p->root_node_p, min_key_p);
    father_p = s_p->root_node_p;

    /* Decend the tree with father and child nodes ([father_p] and [child_p]).
     *  - Split any node that is full which you meet on the way.
     *  - use lock-coupling
     *  - take all locks in write-mode
     */
    level = 0;
    while (1) {
        int idx;
        Oc_bpt_node *right_p;
        
        oc_bpt_trace_wu_lvl(
            3, OC_EV_BPT_INSERT_RNG, wu_p,
            "iteration father=[%s] level=%d",
            oc_bpt_nd_string_of_node(s_p, father_p), level);
        
        level++;
        child_p = lookup_key_in_index_node(wu_p, s_p, father_p,
                                           min_key_p,
                                           &idx, hi_bound_key_p);

        if (oc_bpt_nd_is_leaf(s_p, child_p)) {
            // [child_p] is a leaf. insert into it.

            if (!oc_bpt_nd_is_full(s_p, child_p)) {
                // Leaf node with room left            
                int eligible_len;
                
                oc_utl_debugassert(!oc_bpt_nd_is_full(s_p, child_p));
                eligible_len = chop(wu_p, s_p, length, key_array, hi_bound_key_p);
                oc_utl_debugassert(eligible_len > 0);
                
                rc = oc_bpt_nd_insert_array_into_leaf(
                    wu_p, s_p,
                    child_p,
                    eligible_len,
                    key_array, data_array, nkeys_inserted_po);
                
                oc_bpt_nd_release(wu_p, s_p, father_p);
                oc_bpt_nd_release(wu_p, s_p, child_p);        
                return rc;
            }
            else {
                // Leaf node with no room to spare, split and insert
                int eligible_len;
                Oc_bpt_node *trg_p;

                oc_utl_debugassert(!oc_bpt_nd_is_root(s_p, child_p));                
                right_p = oc_bpt_nd_split(wu_p, s_p, child_p);

                // add into the correct node
                switch (s_p->cfg_p->key_compare(
                            min_key_p,
                            oc_bpt_nd_min_key(s_p, right_p))) {
                case 0:
                case -1:
                    trg_p = right_p;
                    break;
                case 1:
                    update_hi_key(s_p, right_p, hi_bound_key_p);
                    trg_p = child_p;
                    break;
                }
                
                eligible_len = chop(wu_p, s_p, length, key_array, hi_bound_key_p);
                oc_utl_debugassert(eligible_len > 0);
                
                rc = oc_bpt_nd_insert_array_into_leaf(
                    wu_p, s_p,
                    trg_p,
                    eligible_len,
                    key_array, data_array, nkeys_inserted_po);
                
                // replace the old binding for [idx] with bindings: 
                //   * min(L) -> child_p
                //   * min(R) -> right_p
                oc_bpt_nd_index_replace_w2(wu_p, s_p, father_p,
                                           idx,
                                           child_p, right_p);
                
                oc_bpt_nd_release(wu_p, s_p, father_p);
                oc_bpt_nd_release(wu_p, s_p, child_p);
                oc_bpt_nd_release(wu_p, s_p, right_p);
                return rc;
            }
        }

        // [child_p] is an index node
        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_INSERT_RNG, wu_p, "level=%d", level);
        oc_utl_debugassert(!oc_bpt_nd_is_root(s_p, child_p));
        oc_utl_debugassert(!oc_bpt_nd_is_leaf(s_p, child_p));
        correct_min_key(wu_p, s_p, child_p, min_key_p);

        if (oc_bpt_nd_is_full(s_p, child_p)) {
            right_p = oc_bpt_nd_split(wu_p, s_p, child_p);
            
            // replace the old binding for [idx] with bindings: 
            //   * min(L) -> child_p
            //   * min(R) -> right_p
            // this cannot cause a split in F
            oc_bpt_nd_index_replace_w2(wu_p, s_p, father_p,
                                       idx,
                                       child_p, right_p);
            
            // choose if we should continue with [child_p] or with [right_p]
            switch (s_p->cfg_p->key_compare(
                        min_key_p,
                        oc_bpt_nd_min_key(s_p, right_p))) {
            case 0:
            case -1:
                oc_bpt_nd_release(wu_p, s_p, child_p);
                child_p = right_p;
                break;
            case 1:
                update_hi_key(s_p, right_p, hi_bound_key_p);
                oc_bpt_nd_release(wu_p, s_p, right_p);
                break;
            }
        }
        
        // release [father_p], making [child_p] the father
        oc_bpt_nd_release(wu_p, s_p, father_p);
        father_p = child_p;
    }
}

/**********************************************************************/

int oc_bpt_op_insert_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_data *data_array)
{
    int rc, sum;

    oc_utl_debugassert(length > 0);

    for (rc=0, sum=0; sum < length; )
        rc += fill_single_leaf_b(
            wu_p, s_p,
            length - sum,
            oc_bpt_nd_key_array_kth(s_p, key_array, sum),
            oc_bpt_nd_data_array_kth(s_p, data_array, sum),
            &sum);
    oc_utl_assert(sum == length);
    
    return rc;
}
