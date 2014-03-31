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
/* OC_XT_OP_INSERT_RANGE.C
 *
 * Insert an array of extents into an x-tree
 */
/**********************************************************************/
/*
 * Insert an array of extents. The caller must provide a dense
 * array of extents such that there are no spaces in between them. 
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
#include "oc_xt_int.h"
#include "oc_xt_trace.h"
#include "oc_xt_nd.h"
#include "oc_xt_op_insert_range.h"

/**********************************************************************/
// prototypes
static Oc_xt_node *lookup_key_in_index_node(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p,
    int *idx_po,
    struct Oc_xt_key *hi_key_pio);
static void correct_min_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p);
static void update_hi_key(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *hi_key_p);
static uint64 fill_single_leaf_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p,
    uint64 *len_inserted_po);
/**********************************************************************/

static Oc_xt_node *lookup_key_in_index_node(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p,
    int *idx_po,
    struct Oc_xt_key *hi_key_pio)
{
    uint64 child_addr;

    
    child_addr = oc_xt_nd_index_lookup_key(wu_p, s_p, node_p, key_p,
                                           NULL, idx_po);
    if (*idx_po < oc_xt_nd_num_entries(s_p, node_p)-1) {
        // if there is a higher key then update the high-bound
        struct Oc_xt_key *hi_key_p;

        hi_key_p = oc_xt_nd_get_kth_key(s_p, node_p, *idx_po + 1);
//        printf("hi-key = %s idx=%d\n", oc_xt_nd_string_of_key(s_p, hi_key_p), idx);
//        printf("node=%s #entries=%d\n",
//               oc_xt_nd_string_of_node(s_p, node_p),
//               oc_xt_nd_num_entries(s_p, node_p));
        memcpy((char*)hi_key_pio, hi_key_p, s_p->cfg_p->key_size);
    }
               
    return oc_xt_nd_get_for_write(wu_p, s_p, child_addr, node_p, *idx_po);
}

static void correct_min_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p)
{
    switch (s_p->cfg_p->key_compare(
                key_p,
                oc_xt_nd_min_key(s_p, node_p))) {
    case 0:
    case -1:
        // nothing to do
        break;
    case 1:
        /* the key is smaller than all existing keys.
         *  - replace the minimal key
         */
        oc_xt_trace_wu_lvl(3, OC_EV_XT_REPLACE_MIN_IN_INDEX, wu_p, "");
        oc_xt_nd_index_replace_min_key(wu_p, s_p, node_p, key_p);
        break;
    }
}

static void update_hi_key(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *hi_key_p)
{
    struct Oc_xt_key *min_key_p = oc_xt_nd_min_key(s_p, node_p);
    
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

/* A bounded insert that fills up a single leaf.
 *
 * The algorithm is like inserting a single entry. Descend through the
 * tree. If a node on the path is completely full then split it. When
 * you reach the leaf fill it up. If it becomes full then split it. 
 *
 * Return
 *  1. the (total) length overwritten 
 *  2. update the inserted-extent, record the area inserted.
 *     What remains of the extent is the part that has note been
 *     inserted yet.
 *  3. set [len_inserted_po] to the length, out of the extent
 *     [key_p, rcrd_p], that has actually been inserted. 
 *
 * note: it is possible that the function will be not be able to
 *   complete the insertion of a single extent into a node. This
 *   can happen if the extent is larger than what can fit into this
 *   node.
 */
static uint64 fill_single_leaf_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p,
    uint64 *len_inserted_po)
{
    Oc_xt_node *father_p, *child_p;
    struct Oc_xt_key *hi_bound_key_p;
    int level = 0, rc=0;
    
    oc_xt_trace_wu_lvl(
        3, OC_EV_XT_FILL_SINGLE_LEAF_1, wu_p,
        "fill_single_leaf ext=[%s]",
        oc_xt_nd_string_of_rcrd(s_p, key_p, rcrd_p));

    *len_inserted_po = 0;
    oc_xt_nd_get_for_write(wu_p, s_p, s_p->root_node_p->disk_addr,
                           NULL/*no father to update on cow*/,0);
    
    // setup the high bound, initially, it poses no restrictions
    hi_bound_key_p = (struct Oc_xt_key*)alloca(s_p->cfg_p->key_size);
    s_p->cfg_p->rcrd_end_offset(key_p, rcrd_p, hi_bound_key_p); 
    s_p->cfg_p->key_inc(hi_bound_key_p, hi_bound_key_p);
    
    if (oc_xt_nd_is_full_for_insert(s_p, s_p->root_node_p)) {
        // the root is full, split it and continue
        oc_xt_nd_split_root(wu_p, s_p, s_p->root_node_p);
    }
    
    if (oc_xt_nd_is_leaf(s_p, s_p->root_node_p))
    {
        // T has only a root node N
        // insert into it
        rc = oc_xt_nd_insert_into_leaf(
            wu_p, s_p,
            s_p->root_node_p,
            key_p, rcrd_p);
        
        oc_xt_nd_release(wu_p, s_p, s_p->root_node_p);

        *len_inserted_po = s_p->cfg_p->rcrd_length(key_p, rcrd_p);
        return rc;
    }
    
    // T is a non-trivial tree
    correct_min_key(wu_p, s_p, s_p->root_node_p, key_p);
    father_p = s_p->root_node_p;

    /* Decend the tree with father and child nodes ([father_p] and [child_p]).
     *  - Split any node that is full which you meet on the way.
     *  - use lock-coupling
     *  - take all locks in write-mode
     */
    level = 0;
    while (1) {
        int idx;
        Oc_xt_node *right_p;
        
        oc_xt_trace_wu_lvl(
            3, OC_EV_XT_FILL_SINGLE_LEAF_2, wu_p,
            "iteration father=[%s] level=%d",
            oc_xt_nd_string_of_node(s_p, father_p), level);
        
        level++;
        child_p = lookup_key_in_index_node(wu_p, s_p, father_p,
                                           key_p,
                                           &idx, hi_bound_key_p);

        if (oc_xt_nd_is_leaf(s_p, child_p)) {
            // [child_p] is a leaf. insert into it.

            if (!oc_xt_nd_is_full_for_insert(s_p, child_p)) {
                // Leaf node with room left            

                // compute what part of the extent can fit into the node
                s_p->cfg_p->rcrd_chop_top(key_p, rcrd_p, hi_bound_key_p);
                *len_inserted_po = s_p->cfg_p->rcrd_length(key_p, rcrd_p);
                
                rc = oc_xt_nd_insert_into_leaf(
                    wu_p, s_p,
                    child_p,
                    key_p, rcrd_p);
                
                oc_xt_nd_release(wu_p, s_p, father_p);
                oc_xt_nd_release(wu_p, s_p, child_p);        
                return rc;
            }
            else {
                // Leaf node with no room to spare, split and insert
                Oc_xt_node *trg_p;

                oc_utl_debugassert(!oc_xt_nd_is_root(s_p, child_p));                
                right_p = oc_xt_nd_split(wu_p, s_p, child_p);

                // add into the correct node
                switch (s_p->cfg_p->key_compare(
                            key_p,
                            oc_xt_nd_min_key(s_p, right_p))) {
                case 0:
                case -1:
                    trg_p = right_p;
                    break;
                case 1:
                    update_hi_key(s_p, right_p, hi_bound_key_p);
                    trg_p = child_p;
                    break;
                }

                // compute what subset of extents can fit into the node
                s_p->cfg_p->rcrd_chop_top(key_p, rcrd_p, hi_bound_key_p);
                *len_inserted_po = s_p->cfg_p->rcrd_length(key_p, rcrd_p);
                oc_utl_debugassert(*len_inserted_po > 0);
                
                rc = oc_xt_nd_insert_into_leaf(
                    wu_p, s_p,
                    trg_p,
                    key_p, rcrd_p);
                
                // replace the old binding for [idx] with bindings: 
                //   * min(L) -> child_p
                //   * min(R) -> right_p
                oc_xt_nd_index_replace_w2(wu_p, s_p, father_p,
                                          idx, 
                                          child_p, right_p);
                
                oc_xt_nd_release(wu_p, s_p, father_p);
                oc_xt_nd_release(wu_p, s_p, child_p);
                oc_xt_nd_release(wu_p, s_p, right_p);
                return rc;
            }
        }

        // [child_p] is an index node
        oc_xt_trace_wu_lvl(3, OC_EV_XT_FILL_SINGLE_LEAF_3, wu_p, "level=%d", level);
        oc_utl_debugassert(!oc_xt_nd_is_root(s_p, child_p));
        oc_utl_debugassert(!oc_xt_nd_is_leaf(s_p, child_p));
        correct_min_key(wu_p, s_p, child_p, key_p);

        if (oc_xt_nd_is_full_for_insert(s_p, child_p)) {
            right_p = oc_xt_nd_split(wu_p, s_p, child_p);
            
            // replace the old binding for [idx] with bindings: 
            //   * min(L) -> child_p
            //   * min(R) -> right_p
            // this cannot cause a split in F
            oc_xt_nd_index_replace_w2(wu_p, s_p, father_p,
                                      idx,
                                      child_p, right_p);
            
            // choose if we should continue with [child_p] or with [right_p]
            switch (s_p->cfg_p->key_compare(
                        key_p,
                        oc_xt_nd_min_key(s_p, right_p))) {
            case 0:
            case -1:
                oc_xt_nd_release(wu_p, s_p, child_p);
                child_p = right_p;
                break;
            case 1:
                update_hi_key(s_p, right_p, hi_bound_key_p);
                oc_xt_nd_release(wu_p, s_p, right_p);
                break;
            }
        }
        
        // release [father_p], making [child_p] the father
        oc_xt_nd_release(wu_p, s_p, father_p);
        father_p = child_p;
    }
}

/**********************************************************************/
uint64 oc_xt_op_insert_range_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p)
{
    uint64 rc, ext_len, tot_len, len_inserted;
    struct Oc_xt_key* crnt_key_p;
    struct Oc_xt_rcrd* crnt_rcrd_p;
    
    /* Make another copy of the record and key, they will be modified by
     * the [fill_single_leaf_b] function. 
     */
    crnt_key_p = (struct Oc_xt_key*)alloca(s_p->cfg_p->key_size);
    crnt_rcrd_p = (struct Oc_xt_rcrd*)alloca(s_p->cfg_p->rcrd_size);
    memcpy((char*)crnt_key_p,  key_p, s_p->cfg_p->key_size);
    memcpy((char*)crnt_rcrd_p,  rcrd_p, s_p->cfg_p->rcrd_size);
    
    ext_len = s_p->cfg_p->rcrd_length(key_p, rcrd_p);

    rc = tot_len = 0;
    while (1) {
        rc += fill_single_leaf_b(
            wu_p, s_p,
            crnt_key_p, crnt_rcrd_p,
            &len_inserted);
        
        tot_len += len_inserted;
        oc_utl_debugassert(len_inserted <= tot_len);
        oc_xt_trace_wu_lvl(3, OC_EV_XT_INSERT_RNG_LEN, wu_p, "len=%Lu", len_inserted);
        
        if (tot_len == ext_len)
            // we're done, everything has been inserted
            return rc;
        
        // move cursor up
        memcpy((char*)crnt_key_p,  key_p, s_p->cfg_p->key_size);
        memcpy((char*)crnt_rcrd_p,  rcrd_p, s_p->cfg_p->rcrd_size);
        s_p->cfg_p->rcrd_chop_length(crnt_key_p, crnt_rcrd_p, tot_len);
    }
}

