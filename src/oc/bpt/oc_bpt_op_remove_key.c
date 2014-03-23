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
/* OC_BPT_OP_REMOVE_KEY.C  
 *
 * Remove a single key from a b+-tree. 
 */
/**********************************************************************/
/*
 * Remove is implemented with a pro-active merge policy. On the way down
 * to a leaf each node with a minimal amount of keys is
 * fixed. Fixing means either moving keys into or merging it with
 * neighbor nodes so it will have more than b keys. This ensures that
 * inserting into the leaf will, at worst, effect its immediate
 * ancestor. 
 * 
 * The function remove(T, K) has T as a tree K as a key to
 * remove. The pseudo code for remove(T, K) is separated into
 * special handling of the root node and then the regular iterative
 * case. An additional helper function fix(F, C) fixes an in-danger
 * node C. 
 * 
 * Handling the root
 *   1. N = root(T)
 *   2. if the tree contains only a root node 
 *        * remove K from T 
 *        * return
 *      else 
 *        * N is an internal node
 *        * if N has a minimal amount of keys
 *            - that is ok. the root is the only node allowed to
 *              have as less than b keys.
 *            - return
 * 
 * The iterative case. F is a father node and C is it's child. F is an
 * index node that will not require merge if C is merged with a
 * neighboring node. 
 *   1. If C contains b entries than fix(F,C)
 *   2. C is a leaf node
 *        * remove K from C
 *        * return
 *      else
 *        * C is an index node
 *        * trade places between F and C
 *            - release the father 
 *            - F = C
 *            - C = lookup for K in F
 *        * iterate
 * 
 * Pseudo code for fix(F,C). F is a father node, C is a child
 * node. Node C is in-danger of underflow meaning that it has exactly
 * b entries. 
 *   * notice
 *       - C may have zero, one, or two neighbors
 *   * if C has no neighbors then 
 *       - F is the root and C is a leaf
 *       - move all C's entries to F
 *       - return
 *   * else        
 *       - Look at the right and left neighbors of C and see if they
 *         have more than b entries
 *       - If true then 
 *           = move entries into C so as to have at least b+1
 *           = leave neighbors with at least b+1 entries
 *           = fix the index entries in F
 *           = return
 *       - else
 *           = All neighbors of C have b or b+1 entries
 *           = merge C with one of its neighbors
 *           = the merged node will have at least 2b entries
 *           = remove the entry in F pointing to C
 *           = return
 */
/**********************************************************************/
#include <string.h>

#include "oc_utl.h"
#include "oc_bpt_int.h"
#include "oc_bpt_op_validate.h"
#include "oc_bpt_nd.h"
#include "oc_bpt_trace.h"
/**********************************************************************/
// prototypes
static bool verify_father_entries(struct Oc_bpt_state *s_p,
                                  Oc_bpt_node *father_p);
static void get_prev_next(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    uint32 kth,
    Oc_bpt_node **left_node_ppo,
    Oc_bpt_node **right_node_ppo);
static void fix_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *father_p,
    Oc_bpt_node *child_p,
    uint32 kth);
static Oc_bpt_node *lookup_child_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    int *kth_po);
static bool remove_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p);
/**********************************************************************/    
// Verify that the number of entries in the father is correct
static bool verify_father_entries(struct Oc_bpt_state *s_p,
                                  Oc_bpt_node *father_p)
{
    if (oc_bpt_nd_is_root(s_p, father_p))
        return (oc_bpt_nd_num_entries(s_p, father_p) > 1);
    else
        return (oc_bpt_nd_num_entries(s_p, father_p) >=
                s_p->cfg_p->min_num_ent);
}

/* get the left and right neighbors to [node_p].
 * take write-locks on them, if they exist.
 */
static void get_prev_next(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    uint32 k,
    Oc_bpt_node **left_node_ppo,
    Oc_bpt_node **right_node_ppo)
{
    uint32 num_entries = oc_bpt_nd_num_entries(s_p, node_p);
    uint64 addr;
    struct Oc_bpt_key *dummy_key_p;
    
    if (k > 0 &&
        k < num_entries) {
        oc_bpt_nd_index_get_kth(s_p, node_p, k-1, &dummy_key_p, &addr);
        *left_node_ppo = oc_bpt_nd_get_for_write(wu_p, s_p, addr,
                                                 node_p, k-1);        
    } else {
        *left_node_ppo = NULL;
    }

    if (k < num_entries-1) {
        oc_bpt_nd_index_get_kth(s_p, node_p, k+1, &dummy_key_p, &addr);
        *right_node_ppo = oc_bpt_nd_get_for_write(wu_p, s_p, addr,
                                                  node_p, k+1);
    } else {
        *right_node_ppo = NULL;
    }
}

/* [child_p] has exactly b entries. [father_p] is it's father.
 * both nodes are locked for write.
 */
static void fix_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *father_p,
    Oc_bpt_node *child_p,
    uint32 kth)
{
    Oc_bpt_node *left_p = NULL, *right_p = NULL;
    
    oc_utl_debugassert(oc_bpt_nd_num_entries(s_p, child_p) ==
                       s_p->cfg_p->min_num_ent);
    oc_utl_debugassert(!oc_bpt_nd_is_root(s_p, child_p));
    oc_utl_debugassert (oc_bpt_nd_num_entries(s_p, father_p) > 1);

    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_RMV_KEY, wu_p,
                        "fix kth=%d  #father=%d  #child=%d",
                        kth,
                        oc_bpt_nd_num_entries(s_p, father_p),
                        oc_bpt_nd_num_entries(s_p, child_p));

    /* Entries need to be moved between [child_p] and other children
     * of [father_p]. [child_p] may have {1,2} neighbors.
     */
    
    /* Look at the right and left neighbors of [child_node_p] and see
     * if they have more than b entries.
     */
    get_prev_next(wu_p, s_p, father_p, kth, &left_p, &right_p);
    oc_utl_debugassert(left_p != NULL || right_p != NULL);

    if (left_p != NULL &&
        oc_bpt_nd_num_entries(s_p, left_p) > s_p->cfg_p->min_num_ent + 1)
    {
        // move entries into [child_node_p] so as to have at least b
        oc_bpt_nd_rebalance(wu_p, s_p, child_p, left_p);

        // correct the pointer to [child_p]
        oc_bpt_nd_index_set_kth(s_p, father_p, kth,
                                oc_bpt_nd_min_key(s_p, child_p),
                                child_p->disk_addr);

        goto done;
    }
    else if (right_p != NULL &&
             oc_bpt_nd_num_entries(s_p, right_p) > s_p->cfg_p->min_num_ent + 1)
    {        
        // move entries into [child_node_p] so as to have at least b
        oc_bpt_nd_rebalance(wu_p, s_p, child_p, right_p);

        // correct the pointer to [right_p]
        oc_bpt_nd_index_set_kth(s_p, father_p, kth+1,
                                oc_bpt_nd_min_key(s_p, right_p),
                                right_p->disk_addr);
        goto done;
    }
    
    /* All neighbors of [child_p] have b/b+1 entries
     *  = merge [child_p] with one of its neighbors
     *  = the merged node will have at least 2b entries
     */
    if (left_p != NULL) {
        oc_bpt_nd_move_and_dealloc(wu_p, s_p, child_p, left_p);
        left_p = NULL;
        
        // fix the pointer to [child_p]
        oc_bpt_nd_index_set_kth(s_p, father_p, kth,
                                oc_bpt_nd_min_key(s_p, child_p),
                                child_p->disk_addr);
        
        // remove the entry in [father_p] pointing to [left_p]
        oc_bpt_nd_remove_kth(s_p, father_p, kth-1);
        goto done;
    } else {
        oc_bpt_nd_move_and_dealloc(wu_p, s_p, child_p, right_p);
        right_p = NULL;
        
        // fix the pointer to [child_p]
        oc_bpt_nd_index_set_kth(s_p, father_p, kth,
                                oc_bpt_nd_min_key(s_p, child_p),
                                child_p->disk_addr);
        
        // remove the entry in [father_p] pointing to [right_p]
        oc_bpt_nd_remove_kth(s_p, father_p, kth+1);
        goto done;
    }

 done:
    if (right_p != NULL) oc_bpt_nd_release(wu_p, s_p, right_p);
    if (left_p != NULL) oc_bpt_nd_release(wu_p, s_p, left_p);
}

/* look for a child node of [node_p] at key [key_p].
 * if found
 *    - lock it for write
 *    - return it's relative index in [node_p] and [child_p]
 * else
 *    - return NULL
 */
static Oc_bpt_node *lookup_child_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    int *kth_po)
{
    uint64 child_addr;
    struct Oc_bpt_key *exact_key_p;
    
    child_addr = oc_bpt_nd_index_lookup_key(wu_p, s_p, node_p, key_p,
                                            &exact_key_p,
                                            kth_po);
    if (0 == child_addr) {
        // The key is not in the tree, we are done
        return NULL;
    }
    return oc_bpt_nd_get_for_write(wu_p, s_p, child_addr,
                                   node_p, *kth_po);
}

// remove [key_p] from the subtree rooted at [node_p]
static bool remove_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p)
{
    bool rc;
    Oc_bpt_node *child_p, *father_p;
    int kth;

    oc_bpt_nd_get_for_write(wu_p, s_p, s_p->root_node_p->disk_addr,
                            NULL/*no father to update on COW*/,0);    

    if (oc_bpt_nd_is_leaf(s_p, s_p->root_node_p))
    {
        // T has only a root node N
        // remove [key_p] from T, return
        rc = oc_bpt_nd_remove_key(wu_p, s_p, s_p->root_node_p, key_p);
        oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);
        return rc;
    }
    else {
        // The root is an internal node.
        // There is no need to check it for underflow. It can have as few as two children
    }

    father_p = s_p->root_node_p;
    oc_utl_debugassert(oc_bpt_nd_num_entries(s_p, father_p) > 1);

    // setup the iteration
    child_p = lookup_child_b(wu_p, s_p, father_p, key_p, &kth);
    if (NULL == child_p) {
        oc_bpt_nd_release(wu_p, s_p, father_p);
        return FALSE;
    }            
    
    if (oc_bpt_nd_num_entries(s_p, child_p) == s_p->cfg_p->min_num_ent)
    {
        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_RMV_KEY, wu_p, "minimal child");
        fix_b(wu_p, s_p, father_p, child_p, kth);
        
        if (oc_bpt_nd_num_entries(s_p, father_p) == 1) {
            /* separate handling for the case of collapse into root.
             * [child_p] has no neighbors. Copy the child into the root
             * and deallocate it.
             *
             * This can only happen if fixing the child included a merge
             * which eliminated the other child of the root. 
             */
            oc_bpt_nd_copy_into_root_and_dealloc(wu_p, s_p, father_p, child_p);
            oc_utl_debugassert(oc_bpt_nd_num_entries(s_p, father_p) > 2);
            
            if (oc_bpt_nd_is_leaf(s_p, father_p)) {
                // The whole tree is gone, only the root remains
                rc = oc_bpt_nd_remove_key(wu_p, s_p, father_p, key_p);
                oc_bpt_nd_release(wu_p, s_p, father_p);
                return rc;                    
            }
            else {
                // We lost a level in the tree, but the tree is still non-trivial
                child_p = lookup_child_b(wu_p, s_p, father_p, key_p, &kth);
                if (NULL == child_p) {
                    oc_bpt_nd_release(wu_p, s_p, father_p);
                    return FALSE;
                }            
            }
        }
    }
    
    // loop body
    while (1) {
        /* The iterative case. F is a father node and C is it's child. F is an
         * index node that will not require merge if C is merged with a
         * neighboring node.
         */
        oc_utl_debugassert(oc_bpt_nd_num_entries(s_p, child_p) >=
                           s_p->cfg_p->min_num_ent);
        oc_utl_debugassert(verify_father_entries(s_p, father_p));

        if (oc_bpt_nd_num_entries(s_p, child_p) == s_p->cfg_p->min_num_ent) {
            // C contains exactly b entries, call fix(F,C)
            fix_b(wu_p, s_p, father_p, child_p, kth);
        }
        
        if (oc_bpt_nd_is_leaf(s_p, child_p)) {
            // C is a leaf node
            //  remove K from C
            rc = oc_bpt_nd_remove_key(wu_p, s_p, child_p, key_p);
            oc_bpt_nd_release(wu_p, s_p, father_p);
            oc_bpt_nd_release(wu_p, s_p, child_p);
            return rc;
        }
        
        // trade places between F and C
        oc_bpt_nd_release(wu_p, s_p, father_p);
        father_p = child_p;
        child_p = lookup_child_b(wu_p, s_p, father_p, key_p, &kth);
        if (NULL == child_p) {
            oc_bpt_nd_release(wu_p, s_p, father_p);
            return FALSE;
        }
    }
}

/**********************************************************************/
bool oc_bpt_op_remove_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p)
{
    return remove_b(wu_p, s_p, key_p);
}
