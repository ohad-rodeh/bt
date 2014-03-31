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
/* OC_XT_OP_STAT.C  
 *
 *  Compute statistics on a given b+-tree
 */
/**********************************************************************/
/* Collect stattistics on a given Btree by traversing it in a DFS manner
 */
/**********************************************************************/
#include <string.h>

#include "oc_utl.h"
#include "oc_crt_int.h"
#include "oc_xt_int.h"
#include "oc_xt_op_stat.h"
#include "oc_xt_nd.h"
/**********************************************************************/

static inline int32 get_max(uint32 x, uint32 y) {
    return (x > y) ? x: y;
    
}

static inline int32 get_min(uint32 x, uint32 y) {
    return (x < y) ? x: y;
}

static void collect_statistics(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_statistics *stat_p,
    Oc_xt_node *node_p)
{


    // Collect statistics from the current node
    
    stat_p->num_nodes++;

    if (oc_xt_nd_is_root(s_p, node_p)) {
        stat_p->root_fanout = oc_xt_nd_num_entries(s_p, node_p);
    }
    
    if (oc_xt_nd_is_leaf(s_p, node_p)) {
        stat_p->num_leaves ++;
        stat_p->num_extents += oc_xt_nd_num_entries(s_p, node_p);
        
        stat_p->max_leafcap =
            get_max(oc_xt_nd_num_entries(s_p, node_p), stat_p->max_leafcap);
        stat_p->min_leafcap =
            get_min(oc_xt_nd_num_entries(s_p, node_p), stat_p->min_leafcap);
    }

    // an index node
    else {
        stat_p->num_indexnodes ++;
        stat_p->num_pointers += oc_xt_nd_num_entries(s_p, node_p);
        
        stat_p->max_fanout =
            get_max(oc_xt_nd_num_entries(s_p, node_p), stat_p->max_fanout);
        stat_p->min_fanout =
            get_min(oc_xt_nd_num_entries(s_p, node_p), stat_p->min_fanout);
    }

    // recurse
    if (oc_xt_nd_is_leaf(s_p, node_p)) {
        stat_p->tmp_depth = 0;
        stat_p->depth = 0;
        return;
    }

    else {
        int i;
        int current_fanout = oc_xt_nd_num_entries(s_p, node_p);
        uint64 child_addr = 0;
        Oc_xt_node *child_node_p;
        struct Oc_xt_key *tmp_key;
        uint32 local_depth = 0;

        oc_utl_debugassert(current_fanout != 0);
        for (i=0; i < current_fanout; i++) {
            oc_xt_nd_index_get_kth(s_p, node_p, i,
                                    &tmp_key,
                                    &child_addr);
            child_node_p = s_p->cfg_p->node_get(wu_p, child_addr);
            collect_statistics (wu_p, s_p, stat_p, child_node_p);
            s_p->cfg_p->node_release(wu_p, child_node_p);

// asserting that the tree is balanced 
            oc_utl_debugassert ((local_depth == 0) || (local_depth == stat_p->tmp_depth+1));
            
            local_depth = get_max(local_depth, stat_p->tmp_depth+1);
            if (i == current_fanout -1)
                stat_p->tmp_depth = local_depth;
        }
        stat_p->depth = stat_p->tmp_depth;
    }
}
        

static void init_statistics(
    Oc_xt_statistics *st_p,
    Oc_xt_cfg *cfg_p)
{
    st_p->num_extents = 0;
    st_p->num_nodes = 0;
    st_p->num_leaves = 0;
    st_p->num_indexnodes = 0;
    st_p->num_pointers = 0;

    st_p->depth = 0;
    st_p->tmp_depth = 0;

    st_p->root_fanout = 0;
    
    // Internal node
    st_p->avg_fanout = 0;
    st_p->max_fanout = 0;
    st_p->min_fanout = cfg_p->max_num_ent_index_node;

    // Leaf capacity
    st_p->avg_leafcap = 0;
    st_p->max_leafcap = 0;
    st_p->min_leafcap = cfg_p->max_num_ent_leaf_node;        
}

static void print_statistics(
    Oc_xt_statistics *st_p) {

    printf("\n//********Btree Statistics:\n");
    printf("//  Contains %lu extents and %lu nodes (%lu leaves, %lu index_nodes)\n",
           st_p->num_extents, st_p->num_nodes, st_p->num_leaves, st_p->num_indexnodes);
    printf("//  Tree depth is: %lu\n", st_p->depth);
    printf("//  Root degree is: %lu \n", st_p->root_fanout);
    printf("//  Tree fanout statistics: average=%.2f, max=%lu, min=%lu \n",
           st_p->avg_fanout, st_p->max_fanout, st_p->min_fanout);
    printf("//  Leaf capacity statistics: average=%.2f, max=%lu, min=%lu \n",
           st_p->avg_leafcap, st_p->max_leafcap, st_p->min_leafcap);    
    
}

void oc_xt_op_statistics_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_statistics *st_p)
{

    init_statistics(st_p, s_p->cfg_p); 
    oc_crt_lock_write(&s_p->lock);
    if (oc_xt_nd_num_entries(s_p, s_p->root_node_p) == 0) {
        printf("Empty tree/n");
        return;
    }
    else {
        collect_statistics(wu_p, s_p, st_p, s_p->root_node_p);
    }
    // compute averages
    if (st_p->num_indexnodes != 0)
        st_p->avg_fanout =  (double)((double)st_p->num_pointers / (double)st_p->num_indexnodes);
    if (st_p->num_leaves != 0)
        st_p->avg_leafcap = (double) ((double)st_p->num_extents / (double)st_p->num_leaves);
    if (st_p->num_nodes > 0) {
        /* correct for a calculation mistake in the depth. Even a tree with a single
         * node has depth 1.
         */
        st_p->depth++;
    }

    print_statistics (st_p);
    oc_crt_unlock(&s_p->lock);
    
}
