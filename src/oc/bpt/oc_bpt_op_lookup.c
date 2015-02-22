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
/* OC_BPT_OP_LOOKUP.C  
 *
 * lookup in a b+-tree
 */
/**********************************************************************/
/*
 * lookup is implement through recursive descent in the b-tree. 
 */
/**********************************************************************/
#include <string.h>

#include "oc_bpt_int.h"
#include "oc_bpt_nd.h"
#include "oc_utl_trk.h"
/**********************************************************************/
static bool lookup_in_leaf(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_po)
{
    struct Oc_bpt_data *data_p;
    
    data_p = oc_bpt_nd_leaf_lookup_key(wu_p, s_p, node_p, key_p);
    if (NULL == data_p)
        return FALSE;
    memcpy((char*)data_po, data_p, s_p->cfg_p->data_size);
    return TRUE;
}

static bool lookup_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_po)
{
    Oc_bpt_node *father_p, *child_p;
    uint64 addr;
    bool rc;
    int idx;
    
    oc_bpt_nd_get_for_read(wu_p, s_p, s_p->root_node_p->disk_addr);

    // 1. base case, this is the root node and the root is a leaf
    if (oc_bpt_nd_is_leaf(s_p, s_p->root_node_p))
    {
        rc = lookup_in_leaf(wu_p, s_p, s_p->root_node_p, key_p, data_po);
        oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);
        return rc;
    }
        
    // 2. this is the root, and it isn't a leaf node:
    //    look for the correct child and get it
    addr = oc_bpt_nd_index_lookup_key(wu_p, s_p, s_p->root_node_p, key_p,
                                      NULL, &idx);
    if (0 == addr) {
        oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);
        return FALSE;
    }
    child_p = oc_bpt_nd_get_for_read(wu_p, s_p, addr);
    father_p = s_p->root_node_p;
    
    // 3. regular case: child, with a father, both are locked for read.
    //    if the child is a leaf node, then do a local lookup.
    //    else, release the father, get the child's son. 
    while (1) {

        if (oc_bpt_nd_is_leaf(s_p, child_p)) {
            rc = lookup_in_leaf(wu_p, s_p, child_p, key_p, data_po);
            oc_bpt_nd_release(wu_p, s_p, father_p);
            oc_bpt_nd_release(wu_p, s_p, child_p);
            return rc;
        }
        else {
            // release the father
            oc_bpt_nd_release(wu_p, s_p, father_p);

            // switch places between father and son.
            father_p = child_p;
            
            addr = oc_bpt_nd_index_lookup_key(wu_p, s_p, father_p, key_p,
                                              NULL, &idx);
            if (0 == addr) {
                oc_bpt_nd_release(wu_p, s_p, father_p);                
                return FALSE;
            }
            child_p = oc_bpt_nd_get_for_read(wu_p, s_p, addr);
        }
    }
}

bool oc_bpt_op_lookup_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_po)
{
    return lookup_b(wu_p, s_p, key_p, data_po);
}
