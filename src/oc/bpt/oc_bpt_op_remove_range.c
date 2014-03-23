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
/* OC_BPT_OP_REMOVE_RANGE.C
 *
 * Remove a range from the b+-tree
 */
/**********************************************************************/
/*
 * Remove range takes a range [ofs, end] and removes all the
 * keys inside it the tree. The algorithm is split into three
 * phases. The first phase deallocates values and nodes in the
 * range. This may, very likely, invalidate the b-tree invariants. The
 * second phase computes the set of edge nodes. The third phase
 * reshapes the tree in a downward traversal restoring the
 * invariants. This algorithm is based on the delete algorithm from
 * the Exodus system.
 *
 * The restoration algorithm:
 * 
 * 1. If the root is not in danger, go to step 2. If the root has only
 * one child, make this child the new root and go to 1. Otherwise,
 * merge/reshuffle those children of the root that are in danger and go
 * to 1.
 * 
 * 2. Go down to the next node along the edge. If no nodes remain,
 * then the tree is now rebalanced.
 * 
 * 3. While the current node is in danger, merge/reshuffle it with a
 * sibling. (For a given node along the edge, this will require
 * either 0, 1, or 2 iterations of the while loop.)
 * 
 * 4. Goto 2. 
 * 
 * The algorithm needs to keep track of the edge nodes. Instead of
 * maintaining the nodes in a data-structure the remaining minimum and
 * maximum keys ([rmin] and [rmax]) are computed during the first
 * phase. These keys are the closest remaining in the tree to the removed
 * range. The restoration pass then needs to traverse only nodes on the
 * path to [rmin] and [rmax].
 *
 * For example, if:
 *  - the tree contains keys that are integers
 *  - the removed range is [11,15]
 *  - keys 4 5 10 17 20 remain
 * Then:
 *  - rmin=10
 *  - rmax=17
 *
 * In order to simplify the computation of the in-danger state we define
 * a node on the edge as in-danger if it has less than b+2 entries. 
 * 
 */
/**********************************************************************/
#include <string.h>
#include <alloca.h>

#include "oc_utl.h"
#include "oc_bpt_utl.h"
#include "oc_bpt_int.h"
#include "oc_bpt_op_remove_key.h"
#include "oc_bpt_op_remove_range.h"
#include "oc_bpt_op_output_dot.h"
#include "oc_bpt_nd.h"
#include "oc_bpt_trace.h"

/**********************************************************************/
typedef struct Oc_bpt_remove {
    struct Oc_bpt_key *min_key_p;
    struct Oc_bpt_key *max_key_p;
} Oc_bpt_remove;

typedef struct Oc_bpt_children {
    Oc_bpt_node *left_p;
    Oc_bpt_node *right_p;
    int kth;
} Oc_bpt_children;

// the number of problematic children
typedef enum Oc_bpt_num {
    ZERO,
    ONE,
    TWO,
} Oc_bpt_num;

static int remove_range_from_leaf(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    Oc_bpt_remove *rmv_p);
static int remove_phase_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_remove *rmv_p,
    Oc_bpt_node *node_p,
    bool *rmv_all_po);
static void get_prev_next(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    int k,
    Oc_bpt_node **left_node_ppo,
    Oc_bpt_node **right_node_ppo);
static void fix_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *father_p,
    Oc_bpt_node *child_p,
    int kth);
static void wrap_fix_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *father_p,
    Oc_bpt_node *child_p,
    int kth);
static void find_children_in_range(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    Oc_bpt_remove *rmv_p,
    Oc_bpt_node **left_po,
    Oc_bpt_node **right_po,
    int *kth_po);
static Oc_bpt_num combine_problematic_children(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_node *father_p,
    Oc_bpt_remove *rmv_p,
    bool *modified_po,
    int *kth_po,
    Oc_bpt_children *children_po);
static Oc_bpt_num restore_root_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_remove *rmv_p,
    Oc_bpt_children *children_po);
static void restore_two_paths_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_remove *rmv_p,
    Oc_bpt_children *children_p);
static void restore_path_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    Oc_bpt_node *father_p);

static void restore_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_remove *rmv_p);
/**********************************************************************/

/* remove the range of keys from leaf node [node_p]
 *
 * assumption: the lock for the root is held in exclusive-mode
 */
static int remove_range_from_leaf(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    Oc_bpt_remove *rmv_p)
{
    int min_loc, max_loc;

    oc_utl_debugassert(oc_bpt_nd_is_leaf(s_p, node_p));
    
    // compute minimum and maximum indexes to remove
    min_loc = oc_bpt_nd_lookup_ge_key(wu_p, s_p, node_p, rmv_p->min_key_p);
    max_loc = oc_bpt_nd_lookup_le_key(wu_p, s_p, node_p, rmv_p->max_key_p);
    if (-1 == min_loc ||
        -1 == max_loc ||
        min_loc > max_loc)
        // there are no keys in the range
        return 0;
    
    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                        "leaf-node entries = [min=%d,max=%d]",
                        min_loc, max_loc);

    // remove everything between the indexes
    oc_bpt_nd_remove_range(wu_p, s_p, node_p, min_loc, max_loc);
    return max_loc - min_loc + 1;
}

/**********************************************************************/
// delete phase

/* Remove all the entries in the range [min_key_p, max_key_p] from the
 * subtree rooted in [node_p]. The resulting tree might remain
 * invalid. Deallocate all nodes that are no longer necessary.
 * If the whole subtree rooted at [node_p] should be removed then
 * do not remove anything, set [rmv_all_po] to TRUE. 
 * 
 * returned values:
 *  1. the number of removed keys in the subtree rooted at [node_p]
 *  2. [rmv_all_po] is set to TRUE if the whole sub-tree rooted at
 *     [node_p] should be removed. 
 *  
 * assumptions:
 * - the tree is fully locked during this pass.
 * - [node_p] is an index node
 * - the root is locked for write
 *
 * note: the function maintains the lock on the root node
 */
static int remove_phase_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_remove *rmv_p,
    Oc_bpt_node *node_p,
    bool *rmv_all_po)
{
    int rc = 0; // counts the total number of removed keys
    int i, min_loc, max_loc;
    int del_idx_start=-1, del_idx_end=-1;
    int num_entries = oc_bpt_nd_num_entries(s_p, node_p);
    
    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                        "remove_phase_b node=[%s]",
                        oc_bpt_nd_string_of_node(s_p, node_p));
    *rmv_all_po = FALSE;
    
    if (!oc_bpt_nd_is_leaf(s_p, node_p)) {
        /* index node:
         */
        min_loc = oc_bpt_nd_lookup_le_key(wu_p, s_p, node_p, rmv_p->min_key_p);
        max_loc = oc_bpt_nd_lookup_le_key(wu_p, s_p, node_p, rmv_p->max_key_p);
        if (-1 == min_loc) min_loc = 0;
        if (-1 == max_loc) {
            // the node contains no keys in the range
            return rc;
        }
        if (min_loc == max_loc + 1) {
            /* this is a corner case where the node contains the
             * maximal key and nothing else in the range. If the
             * maximal key is in entry [k] then the key greater or
             * equal to the minimum will be in [k] and a key smaller
             * or equal to the maximum is in [k-1].
             */
            max_loc = min_loc;
        }
        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                            "index-node entries = [min=%d,max=%d]",
                            min_loc, max_loc);
        
        /* recurse into all children with keys between [min_loc] and
         * [max_loc].
         */
        for (i=min_loc; i<= max_loc; i++) {
            struct Oc_bpt_key *dummy_key_p;
            uint64 child_addr;
            Oc_bpt_node *child_p;
            bool rmv_child = FALSE;
            
            oc_bpt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &child_addr);
            child_p = oc_bpt_nd_get_for_write(wu_p, s_p, child_addr,
                                              node_p, i);
            rc += remove_phase_b(wu_p, s_p, rmv_p, child_p, &rmv_child);
            
            if (rmv_child) {
                /* The entire child sub-tree should be removed
                 *
                 * keep track of the range of deleted children
                 */
                if (-1 == del_idx_start) {
                    del_idx_start = i;
                    del_idx_end = i;
                } else {
                    del_idx_end = i;
                }
            } 
            oc_bpt_nd_release(wu_p, s_p, child_p);
        }

        if (-1 == del_idx_start) {
            // none of the children need to be removed
        }
        else if (0 == del_idx_start &&
            (num_entries - 1) == del_idx_end) {
            // all children should be removed
            *rmv_all_po = TRUE;
        }
        else {
            // some of the children should be removed
            for (i=del_idx_start; i<=del_idx_end; i++) {
                struct Oc_bpt_key *dummy_key_p;
                uint64 child_addr;
                Oc_bpt_node *child_p;
                
                oc_bpt_nd_index_get_kth(s_p, node_p,
                                        i,
                                        &dummy_key_p, &child_addr);
                child_p = oc_bpt_nd_get_for_read(wu_p, s_p, child_addr);
                oc_bpt_utl_delete_subtree_b(wu_p, s_p, child_p);
            }
            
            // remove the set of deleted children from [node_p]
            oc_bpt_nd_remove_range(wu_p, s_p,
                                   node_p,
                                   del_idx_start, del_idx_end);
        }
        
        return rc;
    }
    else {
        if (oc_bpt_utl_covered(wu_p, s_p,
                               node_p,
                               rmv_p->min_key_p, rmv_p->max_key_p)) {
            /* The whole leaf is covered by the range, do not
             * remove it. Return the number of entries in the node.
             */
            *rmv_all_po = TRUE;
            return num_entries;
        }
        else {
            // leaf node: remove all keys in the range
            return remove_range_from_leaf(wu_p, s_p, node_p, rmv_p);
        }
    }
}

/**********************************************************************/
// restore phase

/* get the left and right neighbors to [node_p].
 * take write-locks on them, if they exist.
 */
static void get_prev_next(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    int k,
    Oc_bpt_node **left_node_ppo,
    Oc_bpt_node **right_node_ppo)
{
    int num_entries = oc_bpt_nd_num_entries(s_p, node_p);
    uint64 addr;
    struct Oc_bpt_key *dummy_key_p;

    oc_utl_debugassert(k >= 0);
    oc_utl_debugassert(k < num_entries);
    
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

/* If [child_p] has less than b+2 keys then
 *   (1) shuffle keys into it from its siblings
 *  or
 *   (2) merge siblings with it.
 *
 * The result node may end up having less than b+2 keys in which case
 * an additional call to [fix_b] is guarantied to correct it.
 *
 * [father_p] is the father of [child_p]. 
 * [kth] is the index of of [child_p] inside [father_p].
 *
 * assumption: the child and father are locked for write.
 */
static void fix_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *father_p,
    Oc_bpt_node *child_p,
    int kth)
{
    Oc_bpt_node *left_p = NULL, *right_p = NULL;
    
    if (oc_bpt_nd_num_entries(s_p, child_p) >= s_p->cfg_p->min_num_ent + 2)
        // nothing to do
        return;

    oc_utl_debugassert(!oc_bpt_nd_is_root(s_p, child_p));
    oc_utl_debugassert (oc_bpt_nd_num_entries(s_p, father_p) > 1);

    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                        "fix kth=%d  #father=%d  #child=%d [%s]",
                        kth,
                        oc_bpt_nd_num_entries(s_p, father_p),
                        oc_bpt_nd_num_entries(s_p, child_p),
                        oc_bpt_nd_string_of_node(s_p, child_p));

    /* Entries need to be moved between [child_p] and other children
     * of [father_p]. [child_p] may have {1,2} neighbors.
     */
    
    /* Look at the right and left neighbors of [child_p] and see
     * if they have more than b entries.
     */
    get_prev_next(wu_p, s_p, father_p, kth, &left_p, &right_p);
    oc_utl_debugassert(left_p != NULL || right_p != NULL);

    if (left_p != NULL &&
        oc_bpt_nd_num_entries(s_p, left_p) + oc_bpt_nd_num_entries(s_p, child_p)
        >= 2 * s_p->cfg_p->min_num_ent + 2)
    {
        // move entries into [child_node_p] so as to have at least b+2
        oc_bpt_nd_rebalance_skewed(wu_p, s_p, child_p, left_p);

        // correct the pointer to [child_p]
        oc_bpt_nd_index_set_kth(s_p, father_p, kth,
                                oc_bpt_nd_min_key(s_p, child_p),
                                child_p->disk_addr);

        goto done;
    }
    else if (right_p != NULL &&
             oc_bpt_nd_num_entries(s_p, right_p) + oc_bpt_nd_num_entries(s_p, child_p)
             >= 2 * s_p->cfg_p->min_num_ent + 2)
    {        
        // move entries into [child_node_p] so as to have at least b+2
        oc_bpt_nd_rebalance_skewed(wu_p, s_p, child_p, right_p);

        // correct the pointer to [right_p]
        oc_bpt_nd_index_set_kth(s_p, father_p, kth+1,
                                oc_bpt_nd_min_key(s_p, right_p),
                                right_p->disk_addr);
        goto done;
    }
    
    /* All neighbors of [child_p] have too few entries to help
     *  = merge [child_p] with one of its neighbors
     *  = the merged node will have at least b+1 entries.
     *    b+1 is not sufficient which means that an additional call
     *    to [fix_b] will be made
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
    oc_utl_debugassert(oc_bpt_nd_num_entries(s_p, child_p) >=
                       s_p->cfg_p->min_num_ent + 1);
    if (right_p != NULL) oc_bpt_nd_release(wu_p, s_p, right_p);
    if (left_p != NULL) oc_bpt_nd_release(wu_p, s_p, left_p);
}

/* fix a child node so that it will not be in-danger. There may be a need 
 * to call [fix_b] twice.
 *
 * [kth] the index of the child inside the father
 * [key_p] the key of the child
 */
static void wrap_fix_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *father_p,
    Oc_bpt_node *child_p,
    int kth)
{
    fix_b(wu_p, s_p, father_p, child_p, kth);
    if (oc_bpt_nd_num_entries(s_p, child_p) < s_p->cfg_p->min_num_ent + 2) {
        // the first call to [fix_b] was insufficient
        struct Oc_bpt_key *dummy_key_p;
        int new_idx;
        uint64 addr;
        
        // recompute the index, it may have changed
        addr = oc_bpt_nd_index_lookup_key(wu_p, s_p, father_p,
                                          oc_bpt_nd_min_key(s_p, child_p),
                                          &dummy_key_p, &new_idx);

        // check that we have the correct child
        oc_utl_debugassert(s_p->cfg_p->key_compare(
                               oc_bpt_nd_min_key(s_p, child_p),
                               dummy_key_p) == 0);
        
        // call fix_b again
        fix_b(wu_p, s_p, father_p, child_p, new_idx);
    }
    
    // make sure the node is not in-danger now
    oc_utl_assert(oc_bpt_nd_num_entries(s_p, child_p) >= s_p->cfg_p->min_num_ent + 2);
}

/* Find the children of [node_p] that were possibly effected by the
 * removal process.
 *
 * return values:
 *  - [left_p]   a node containing keys below [min_key]
 *  - [right_p]  a node containing keys above [max_key]; may be NULL.
 *  - [kth]      index of [left_p] in [node_p]
 *
 * There might be no children in the range, in which case both
 * [left_p] and [right_p] are NULL.
 *
 * The left and right nodes are adjacent inside [node_p]. 
 */
static void find_children_in_range(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    Oc_bpt_remove *rmv_p,
    Oc_bpt_node **left_ppo,
    Oc_bpt_node **right_ppo,
    int *kth_po)
{
    int min_loc, max_loc;
    uint64 child_addr;
    struct Oc_bpt_key *dummy_key;

    oc_utl_debugassert(!oc_bpt_nd_is_leaf(s_p, node_p));
    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                        "find_children_in_range: node=[%s]",
                        oc_bpt_nd_string_of_node(s_p, node_p));
 
    min_loc = oc_bpt_nd_lookup_le_key(wu_p, s_p, node_p, rmv_p->min_key_p);
    max_loc = oc_bpt_nd_lookup_le_key(wu_p, s_p, node_p, rmv_p->max_key_p);
    if (-1 == min_loc && max_loc != -1)
        min_loc = max_loc;

    if (min_loc != -1 &&
        max_loc != -1 &&
        min_loc != max_loc) {
        struct Oc_bpt_key *key1_p, *key2_p;        
        // found upper and lower bounds

        *kth_po = min_loc;
            
        oc_bpt_nd_index_get_kth(s_p, node_p, min_loc, &key1_p, &child_addr);
        *left_ppo = oc_bpt_nd_get_for_write(wu_p, s_p, child_addr,
                                            node_p, min_loc);        

        oc_bpt_nd_index_get_kth(s_p, node_p, max_loc, &key2_p, &child_addr);
        *right_ppo = oc_bpt_nd_get_for_write(wu_p, s_p, child_addr,
                                             node_p, max_loc);

        oc_utl_debugassert(max_loc == min_loc +1);
        return;
    }
    else if (min_loc != -1 && max_loc != -1) {
        int single_loc;

        // found a single child that contains keys in the range
        oc_utl_debugassert(max_loc == min_loc);
        single_loc = min_loc;
        *kth_po = single_loc;
        
        oc_bpt_nd_index_get_kth(s_p, node_p, single_loc, &dummy_key, &child_addr);
        *left_ppo = oc_bpt_nd_get_for_write(wu_p, s_p, child_addr,
                                             node_p, single_loc);
        *right_ppo = NULL;
        return;
    }
    else {
        // All the keys in the node are larger than the range,
        // we are done.
        oc_utl_debugassert(-1 == min_loc && -1 == max_loc);
        *kth_po = -1;
        *left_ppo = NULL;
        *right_ppo = NULL;
        return;
    }
    
    ERR(("sanity, should'nt get here"));
}

/* Merge the problematic children of [father_p] into a single child
 * and return it locked in write-mode. Also flag if any modifications
 * were performed during this operation. It can happen that the 
 * problematic keys cannot be shifted into a single node. In this case,
 * the [exception_po] argument is filled with the two offending children
 * for further handling by the caller. 
 *
 * assumptions:
 * - [father_p] is locked for write by the caller.
 *
 * return value:
 * - the problematic child locked in write-mode. might be NULL if none exist.
 * - [kth_po] is set to the index of the child in [father_p]
 * - [modified_po] is TRUE if the tree was modified; FALSE otherwise.
 * - [exception_po] is filled with two problematic children which are locked in
 *   write mode. 
 *
 * note: the simplest algorithm would have been to make sure all
 * children of [father_p] have at least b+2 children. Unfortunately,
 * this is not possible. Consider a tree with min_num_ent=2 and
 * max_num_ent=5 that looks like this:
 *
 *    [x, y]
 *   /     \
 * [A,B,C]  [D,E,F,G]
 *
 * Both children need to have 4 entries to be out of
 * danger. However, there is a total of only 7 entries
 * altogether so shuffling is out of the question. Another
 * possibility is to merge the two children, however, the
 * maximum number of entries in a node is 5.
 *
 * Therefore, only some of the children can be made to have b+2 entries. 
 *
 * There are four cases:
 * (1) there are two affected children
 *     - shift the edge keys so they are in a single node [X]
 * (2) there are two affected children, however, they are both full
 *     and keys cannot be shifted between them. In this case, the exception
 *     structure is used. 
 * (3) there is one affected child
 *     - return this child
 * (4) the range has been completely erased from the tree and there
 *     is no child in the range.
 */

static Oc_bpt_num combine_problematic_children(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_node *father_p,
    Oc_bpt_remove *rmv_p,
    bool *modified_po,
    int *kth_po,
    Oc_bpt_children *children_po)
{
    Oc_bpt_node *left_p, *right_p;
    int kth;
    int max_num_ent;
    
    *modified_po = FALSE;
    find_children_in_range(wu_p, s_p, father_p, rmv_p, &left_p, &right_p, &kth);
    
    if (NULL == right_p && NULL == left_p)
        // nothing to do 
        return ZERO;
    
    if (NULL == right_p) {
        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                            "combine_children: single child");
        // there is a single child in the modified range
        *kth_po = kth;
        children_po->left_p = left_p;
        return ONE;
    }

    /* there are two children in the modified range. 
     * the edge keys are the last in [left_p] and the first 
     * in [right_p].
     */
    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                        "combine_children: two children");

    max_num_ent = oc_bpt_nd_max_ent_in_node(s_p, left_p);
    if (oc_bpt_nd_num_entries(s_p, left_p) == max_num_ent &&
        oc_bpt_nd_num_entries(s_p, right_p) == max_num_ent)
    {
        /* both nodes are full. Entries cannot be moved between them.
         * record the information and return. The caller needs to handle
         * this case. 
         */
        *kth_po = kth;
        children_po->left_p = left_p;
        children_po->right_p = right_p;
        return TWO;
    }

    *modified_po = TRUE;
    
    if (oc_bpt_nd_num_entries(s_p, left_p) +
        oc_bpt_nd_num_entries(s_p, right_p) <= 2 * s_p->cfg_p->min_num_ent + 1)
    {
        // The two nodes can be merged, they have less than 2b+1 entries
        
        // 1. merge the two nodes together into [left_p]
        oc_bpt_nd_move_and_dealloc(wu_p, s_p, left_p, right_p);
        
        // fix the pointer to [left_p], not strictly necessary
        oc_bpt_nd_index_set_kth(s_p, father_p, kth,
                                oc_bpt_nd_min_key(s_p, left_p),
                                left_p->disk_addr);
        
        // remove the entry in [father_p] pointing to [right_p]
        oc_bpt_nd_remove_kth(s_p, father_p, kth+1);
        
        *kth_po = kth;
        children_po->left_p = left_p;
        return ONE;
    }
    

    /* Together, the nodes have at least 2b+2 entries.
     * shuffle from the larger node to the smaller node such that
     * both edge keys end up in the smaller node. 
     */
    
    // 1. figure out which node has less entries
    if (oc_bpt_nd_num_entries(s_p, left_p) <=
        oc_bpt_nd_num_entries(s_p, right_p)) {
        
        // 2. merge the edge keys to the node with less entries
        oc_bpt_nd_move_min_key(wu_p, s_p, left_p, right_p);
        
        // update the pointer to the larger node
        oc_bpt_nd_index_set_kth(s_p, father_p, 
                                kth+1,
                                oc_bpt_nd_min_key(s_p, right_p),
                                right_p->disk_addr);
        
        oc_bpt_nd_release(wu_p, s_p, right_p);
        *kth_po = kth;
        children_po->left_p = left_p;
        return ONE;
    } else {
        // 2. merge the edge keys on the smaller node
        oc_bpt_nd_move_max_key(wu_p, s_p, right_p, left_p);
        
        // update the pointer to the smaller node
        oc_bpt_nd_index_set_kth(s_p, father_p, 
                                kth+1,
                                oc_bpt_nd_min_key(s_p, right_p),
                                right_p->disk_addr);
        oc_bpt_nd_release(wu_p, s_p, left_p);
        *kth_po = kth+1;
        children_po->left_p = right_p;
        return ONE;
    }
}

/* Restore the root node. This function makes sure that the root is
 * not in-danger.
 *
 * return value:
 *   - the next child to work on. It is taken in write-mode.
 *   - if there are two children then they are indicated in the [exn_po]
 *     structure.
 * 
 * algorithm:
 *    If the root has only one child, make this child the new root
 *    and go to 1. Otherwise, merge/reshuffle those children of the
 *    root that have less than b+2 entries and go to 1.
 *
 * assumption: the root node is locked for write
 * result: the root remains locked when the function returns
 */
static Oc_bpt_num restore_root_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_remove *rmv_p,
    Oc_bpt_children *children_po)
{
    bool modified;
    oc_utl_debugassert (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) > 0);
    
    while (1) {
        uint64 child_addr;
        Oc_bpt_node *child_p;
        int kth;
        Oc_bpt_num num;

        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                            "restore_root iteration root=[%s]",
                            oc_bpt_nd_string_of_node(s_p, s_p->root_node_p));

        if (oc_bpt_nd_is_leaf(s_p, s_p->root_node_p))
            return ZERO;
        
        if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) == 1) {
            // the root has just one child, we need to do something
            child_addr = oc_bpt_nd_index_lookup_min_key(
                wu_p, s_p, s_p->root_node_p);

            /* It would be optimal to take the child lock in read-mode;
             * avoiding the overhead of COW. However, we have
             * sufficient resources to COW it, if necessary. So, we
             * take the lock in write mode. 
             */
            child_p = oc_bpt_nd_get_for_write(wu_p, s_p,
                                             child_addr,
                                             s_p->root_node_p, 0);

            if ( oc_bpt_nd_num_entries(s_p, child_p) <=
                 s_p->cfg_p->max_num_ent_root_node )
            {
                // the child fits inside the root.
                // collapse the child into the root
                oc_bpt_nd_copy_into_root_and_dealloc(
                    wu_p, s_p,
                    s_p->root_node_p, child_p);
            } else {
                // the child does not fit inside the root
                // split the child in two
                Oc_bpt_node *right_p = oc_bpt_nd_split(wu_p, s_p, child_p);
            
                oc_bpt_nd_index_replace_w2(
                    wu_p, s_p, s_p->root_node_p,
                    0,        // index-zero is the minimal key-value pair
                    child_p, right_p);
                oc_bpt_nd_release(wu_p, s_p, child_p);
                oc_bpt_nd_release(wu_p, s_p, right_p);
            }
            
            continue;
        }
        
        // make sure all the children are not in-danger
        oc_utl_debugassert (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) > 1);

        // combine problematic children into a single node
        num = combine_problematic_children(
            wu_p, s_p, s_p->root_node_p, rmv_p, &modified, &kth, children_po
            );
        if (ZERO == num) {
            // no problematic children, we are done
            return ZERO;
        }
        else if (TWO == num) {
            /* Exceptional case: there are two problematic children.
             * They are both full, so there is no further handling here. 
             */
            return TWO;
        }

        // normal case
        oc_utl_debugassert(ONE == num);
        child_p = children_po->left_p;
        
        if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) == 1) {
            /* combining the problematic children caused a merge which
             * reduced the number of entries in the root. There is now
             * a single entry left; go back to the beginning of the
             * loop and handle this case.
             */
            oc_bpt_nd_release(wu_p, s_p, child_p);            
            continue;
        }
        
        // if the node has too few entries then fix it
        if (oc_bpt_nd_num_entries(s_p, child_p) < s_p->cfg_p->min_num_ent + 2) {
            fix_b(wu_p, s_p, s_p->root_node_p, child_p, kth);
            modified = TRUE;
        }

        if (!modified) {
            // we are done. return the caller the child that it needs to continue
            // with
            children_po->left_p = child_p;
            return ONE;
        }
        
        // somthing has been modifed; re-run the loop.
        oc_bpt_nd_release(wu_p, s_p, child_p);
    }
}

/* Fix a damaged path in the tree starting at node [father_p].
 * assumption:
 *   - [father_p] is locked for write
 *   - [father_p] is a full node
 */
static void restore_path_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    Oc_bpt_node *father_p)
{
    Oc_bpt_node *child_p;
    int kth;
    uint64 child_addr;
    struct Oc_bpt_key *dummy_key_p;
        
    while (1) {
        if (oc_bpt_nd_is_leaf(s_p, father_p)) {
            oc_bpt_nd_release(wu_p, s_p, father_p);
            return;
        }
        
        // find the next damaged node
        kth = oc_bpt_nd_lookup_le_key(wu_p, s_p, father_p, key_p);
        if (-1 == kth) {
            // we are done
            oc_bpt_nd_release(wu_p, s_p, father_p);
            return;
        }
        oc_bpt_nd_index_get_kth(s_p, father_p, kth, &dummy_key_p, &child_addr);
        child_p = oc_bpt_nd_get_for_write(wu_p, s_p, child_addr,
                                           father_p, kth);
            
        // fix it
        fix_b(wu_p, s_p, father_p, child_p, kth);
        
        // continue
        oc_bpt_nd_release(wu_p, s_p, father_p);
        father_p = child_p;
    }
}

// Fix the two paths in the tree
static void restore_two_paths_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_remove *rmv_p,
    Oc_bpt_children *children_p)
{
    restore_path_b(wu_p, s_p, rmv_p->min_key_p, children_p->left_p);
    restore_path_b(wu_p, s_p, rmv_p->max_key_p, children_p->right_p);
}

/* Restore a path in the tree. Iteratively search for [key_p]. Restore
 * all nodes on the path to key [key_p] so they will have at least b+2
 * entries.
 *
 * assumption: the root node is held with a write lock.
 *
 * note: The function releases the root lock. 
 */
static void restore_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    Oc_bpt_remove *rmv_p)
{
    Oc_bpt_node *father_p, *child_p, *grand_child_p;
    int kth;
    bool modified;
    Oc_bpt_children children;
    Oc_bpt_num num;

    // restore the root. 
    num  = restore_root_b(wu_p, s_p, rmv_p, &children);
    if (oc_bpt_nd_is_leaf(s_p, s_p->root_node_p)) {
        // All that remains of the tree is the root
        // we are done.
        oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);
        return;
    }
    father_p = s_p->root_node_p;

    if (TWO == num) {
        // there are two children to fix
        oc_bpt_nd_release(wu_p, s_p, father_p);
        restore_two_paths_b(wu_p, s_p, rmv_p, &children);
        return;
    }
    else if (ZERO == num) {
        // the whole left side of the tree has been removed.
        // continue there. ??
        /*
        uint64 child_addr;
        
        child_addr = oc_bpt_nd_index_lookup_min_key(wu_p, s_p, father_p);
        child_p = oc_bpt_nd_get_for_write(wu_p, s_p, child_addr,
                                          father_p, idx);
        */
        oc_bpt_nd_release(wu_p, s_p, father_p);
        return;
    }

    // normal case, single child to fix
    child_p = children.left_p;
    
    // loop body
    while (1) {
        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                            "restore_b iteration node=[%s]",
                            oc_bpt_nd_string_of_node(s_p, child_p));
        
        if (oc_bpt_nd_is_leaf(s_p, child_p)) {
            // we have reached the bottom of the tree, we are done
            oc_bpt_nd_release(wu_p, s_p, child_p);
            oc_bpt_nd_release(wu_p, s_p, father_p);
            return;
        }
        
        // combine problematic grand-children into a single node
        num = combine_problematic_children(
            wu_p, s_p, child_p, rmv_p, &modified, &kth, &children
            );
        if (TWO == num) {
            // There are two problematic children which can't be merged.
            // Each needs to be fixed separately. 
            oc_bpt_nd_release(wu_p, s_p, child_p);
            oc_bpt_nd_release(wu_p, s_p, father_p);
            restore_two_paths_b(wu_p, s_p, rmv_p, &children);
            return;
        }
        else if (ZERO == num) {
            /* no more problematic children, this -might- mean that the
             * removed range was the whole left side of the tree. we are done.
             */
            oc_bpt_nd_release(wu_p, s_p, child_p);
            oc_bpt_nd_release(wu_p, s_p, father_p);
            return;
        }

        // normal case, a single problematic node
        grand_child_p = children.left_p;

        // if the node has too few entries then fix it
        wrap_fix_b(wu_p, s_p, child_p, grand_child_p, kth);
        
        // trade places between F and C
        oc_bpt_nd_release(wu_p, s_p, father_p);
        father_p = child_p;
        child_p = grand_child_p;
    }
}


/**********************************************************************/
int oc_bpt_op_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p)
{
    int rc = 0;
    Oc_bpt_remove rmv;

    if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) == 0)
        // the tree is empty, there is nothing to do 
        return 0;
    
    // make sure the range is valid
    switch (s_p->cfg_p->key_compare(min_key_p, max_key_p)) {
    case -1: return 0;
    case 0:
        // there is exactly one key to remove
        rc = oc_bpt_op_remove_key_b(wu_p, s_p, min_key_p);
        if (rc) return 1;
        else return 0;
    case 1:
        // normal case, the [max_key_p] is strictly larger than [min_key_p]
        break;
    }

    memset(&rmv, 0, sizeof(rmv));
    rmv.min_key_p = min_key_p;
    rmv.max_key_p = max_key_p;
    
    oc_bpt_nd_get_for_write(wu_p, s_p, s_p->root_node_p->disk_addr,
                            NULL/*no father to update on COW*/,0);

    // phase 0: check if this tree contains a root only
    if (oc_bpt_nd_is_leaf(s_p, s_p->root_node_p)) {
        oc_bpt_trace_wu_lvl(3, OC_EV_BPT_REMOVE_RNG, wu_p,
                            "trivial tree root=[%s]",
                            oc_bpt_nd_string_of_node(s_p, s_p->root_node_p));
        rc = remove_range_from_leaf(wu_p, s_p, s_p->root_node_p,&rmv);
        oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);
    }
    else {
        bool rmv_all = FALSE;
        
        // phase 1: remove entries
        rc = remove_phase_b(wu_p, s_p, &rmv, s_p->root_node_p, &rmv_all);
        
        if (0 == rc) {
            // Nothing was removed from the tree
            // we are done
            oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);
        }
        else if (rmv_all) {
            // We need to remove all the keys in the tree
            oc_bpt_utl_delete_all_b(wu_p, s_p);
            oc_bpt_nd_set_leaf(s_p, s_p->root_node_p);
            oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);            
        }
        else {
#if 0
            // this prints out the tree after the delete phase.
            // use this for debugging purposes only
            printf("// printing partial result\n"); fflush(stdout);
            oc_bpt_nd_release(NULL, s_p, s_p->root_node_p);
            oc_bpt_op_output_dot_b(NULL, s_p, stdout, "after_remove");
            oc_bpt_nd_get_for_write(NULL, s_p, s_p->root_node_p->disk_addr,
                                    NULL/*no father to update on COW*/,0);
            fflush(stdout);
#endif
            
            // phase 2: restoration

            // There must still be keys in the tree
            oc_utl_assert(oc_bpt_nd_num_entries(s_p, s_p->root_node_p) > 0);
            
            // restore the edges
            restore_b(wu_p, s_p, &rmv);
        }
    }

    return rc;
}


