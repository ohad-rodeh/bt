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
/* OC_BPT_OP_INSERT.C  
 *
 * Insert into a b+-tree
 */
/**********************************************************************/
/*
 * Insert is implemented with a pro-active split policy. On the way down
 * to a leaf each full node is split. This ensures that inserting into
 * the leaf will, at most, split the leaf. 
 * 
 * The function insert(T, K, D) has T as a tree K as a key to
 * insert into it and D as data. The pseudo-code for insert(T, K, D)} is
 *  
 *   1. if T has only a root node N
 *      then 
 *        * if there is room left in N 
 *          then 
 *            - add (K,D) to N
 *          else
 *            - split N into two nodes: L and R
 *            - add (K,D) to the correct node
 *            - replace the keys in N with two keys min(L) and min(R)
 *                * min(L) -> L
 *                * min(R) -> R
 *      return
 *   2. T is a non-trivial tree
 *        * F = father node; C = child node
 *        * initially: F = root(T)
 *                     C = lookup K in F
 *        * if C == NULL 
 *            - abnormal case: the key is not in the tree
 *            - return 
 *    (X) * otherwise, C is non-null
 *            - if C is a leaf then search for K inside it
 *                = return
 *        * otherwise, C is non-null and is an index node
 *            - if C is full then split C into node L and R
 *                = replace the key in F for C with two keys min(L) and min(R)
 *                    * min(L) -> L
 *                    * min(R) -> R
 *                = this cannot cause a split in F
 *        * release F, making C the father
 *        * lookup in F for a child C with K in its range
 *          - if none found then return
 *        *  goto step X
 */
/**********************************************************************/
#include "oc_utl.h"
#include "oc_bpt_int.h"
#include "oc_bpt_nd.h"
#include "oc_bpt_trace.h"
/**********************************************************************/
// prototypes
static void correct_min_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p);
static Oc_bpt_node *lookup_key_in_index_node(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    int *idx_in_node_po);
static bool insert_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p);
/**********************************************************************/
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

static Oc_bpt_node *lookup_key_in_index_node(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    int *idx_in_node_po)
{
    uint64 child_addr;
    
    child_addr = oc_bpt_nd_index_lookup_key(wu_p, s_p, node_p, key_p,
                                            NULL, idx_in_node_po);
    oc_utl_assert (child_addr != 0);

    return oc_bpt_nd_get_for_write(wu_p, s_p, child_addr,
                                   node_p, *idx_in_node_po);   
}

static bool insert_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p)
{
    bool rc;
    Oc_bpt_node *father_p, *child_p, *right_p;
    int level = 0;
    int idx;
    
    oc_bpt_nd_get_for_write(wu_p, s_p, s_p->root_node_p->disk_addr,
                            NULL/*no father to update*/,0);
    
    if (oc_bpt_nd_is_full(s_p, s_p->root_node_p)) {
        oc_bpt_nd_split_root(wu_p, s_p, s_p->root_node_p);
    }
    
    if (oc_bpt_nd_is_leaf(s_p, s_p->root_node_p))
    {
        // T has only a root node N
        
        if (!oc_bpt_nd_is_full(s_p, s_p->root_node_p)) {
            // if there is room left in the root
            // add (K,D) to N            
            rc = oc_bpt_nd_leaf_insert_key(
                wu_p, s_p, s_p->root_node_p, key_p, data_p);
            oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);
            return rc;
        }
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
        level++;

        // find the index [idx] where [key_p] is located
        child_p = lookup_key_in_index_node(wu_p, s_p, father_p,
                                           key_p,
                                           &idx);
        oc_bpt_trace_wu_lvl(
            3, OC_EV_BPT_INSERT_ITER, wu_p,
            "min(father), min(child)=%s level=%d",
            oc_bpt_nd_string_of_2key(s_p,
                                     oc_bpt_nd_min_key(s_p, father_p),
                                     oc_bpt_nd_min_key(s_p, child_p)),
            level);
        
        if (oc_bpt_nd_is_leaf(s_p, child_p)) {
            // [child_p] is a leaf. insert into it.
        
            if (!oc_bpt_nd_is_full(s_p, child_p)) {
                // Leaf node with room left
                rc = oc_bpt_nd_leaf_insert_key(wu_p, s_p, child_p,
                                               key_p, data_p);
                oc_bpt_nd_release(wu_p, s_p, father_p);
                oc_bpt_nd_release(wu_p, s_p, child_p);
                return rc;
            }
            else {
                // Leaf node with no room to spare, split and insert
                oc_utl_debugassert(!oc_bpt_nd_is_root(s_p, child_p));
                oc_bpt_trace_wu_lvl(3, OC_EV_BPT_LEAF_SPLIT, wu_p, "");
                right_p = oc_bpt_nd_split(wu_p, s_p, child_p);
            
                // add (K,D) to the correct node
                switch (s_p->cfg_p->key_compare(
                            key_p,
                            oc_bpt_nd_min_key(s_p, right_p))) {
                case 0:
                case -1:
                    rc = oc_bpt_nd_leaf_insert_key(wu_p, s_p, right_p,
                                                   key_p, data_p);
                    break;
                case 1:
                    rc = oc_bpt_nd_leaf_insert_key(wu_p, s_p, child_p,
                                                   key_p, data_p);
                    break;
                }
                
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
        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_INSERT_SRC_IN_INDEX, wu_p,
                            "level=%d", level);
        correct_min_key(wu_p, s_p, child_p, key_p);
        
        if (oc_bpt_nd_is_full(s_p, child_p)) {
            // [child_p] is full, split it
            oc_utl_debugassert(!oc_bpt_nd_is_root(s_p, child_p));
            oc_bpt_trace_wu_lvl(3, OC_EV_BPT_INDEX_SPLIT, wu_p, "");
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
                        key_p,
                        oc_bpt_nd_min_key(s_p, right_p))) {
            case 0:
            case -1:
                oc_bpt_nd_release(wu_p, s_p, child_p);
                child_p = right_p;
                break;
            case 1:
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
/*  Inserts [data] into location indexed by [key]
    Used for inserting extents into SNodes
*/

bool oc_bpt_op_insert_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p)
{
    oc_utl_assert(data_p != NULL);
    return insert_b(wu_p, s_p, key_p, data_p);
}

