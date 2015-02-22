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
/* OC_XT_OP_STAT.C  
 *
 * Compute statistics on a given b+-tree
 */
/**********************************************************************/
#ifndef OC_XT_OP_STAT_H
#define OC_XT_OP_STAT_H

typedef struct Oc_xt_statistics {
    // Total number of keys, nodes
    uint32 num_extents; // extents stored in the leaves
    uint32 num_nodes; // root, index and leaves
    uint32 num_leaves; // leaves only
    uint32 num_indexnodes; //index only
    uint32 num_pointers; // total number of pointers in the tree

    // Tree Depth
    uint32 depth; // global tree depth
    uint32 tmp_depth; // temporary variable to store the local depth 

    uint32 root_fanout;
    
    // Internal node fanout;
    double avg_fanout;
    uint32 max_fanout;
    uint32 min_fanout;

    // Leaf capacity;
    double avg_leafcap;
    uint32 max_leafcap;
    uint32 min_leafcap;
    
} Oc_xt_statistics;

// Traverses the tree and collects its statistics
void oc_xt_op_statistics_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_statistics *st_p);

#endif



