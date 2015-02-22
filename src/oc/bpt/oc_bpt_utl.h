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
/* OC_BPT_UTL.H
 *
 * Some utilities. 
 */
/**********************************************************************/
#ifndef OC_BPT_UTL_H
#define OC_BPT_UTL_H

#include "pl_base.h"
#include "oc_bpt_int.h"

// Check if a node is fully covered by the range [min_key_p] .. [max_key_p]
bool oc_bpt_utl_covered(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key* min_key_p,
    struct Oc_bpt_key* max_key_p);

/* Delete a sub-tree rooted at [node_p].
 *
 * assumptions:
 *  - [node_p] is unlocked
 *  - The whole tree is locked
 *
 * [node_p] is released after the operation.
 */
void oc_bpt_utl_delete_subtree_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

/* Delete all the key-value pairs in the tree. 
 * Leave the root node empty but do not delete it.
 * 
 * assumptions:
 *  - The whole tree is locked
 *  - The ref-count of [node_p] greater than one and
 *    it can be modified. 
 *
 * The root is modified, it's ref-count remains the same. 
 */
void oc_bpt_utl_delete_all_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p);


/* Traverse the set of nodes in tree [s_p] and apply
 * function [iter_f] to them. 
 */
void oc_bpt_utl_iter_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    void (*iter_f)(struct Oc_wu *, Oc_bpt_node*),
    Oc_bpt_node *node_p );    

#endif
