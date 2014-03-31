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
/* OC_XT_OP_OUTPUT_DOT.H
 *
 * write out a representation of a node in "dot"
 * format into a file.
 */
/**********************************************************************/
#include <stdio.h>

#include "oc_xt_int.h"
#include "oc_xt_nd.h"
#include "oc_xt_trace.h"

/**********************************************************************/
static void output_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    char *tag_p, int level, 
    FILE *out_p)
{
    int i, num_entries;
    struct Oc_xt_key *min_key_p;
    char min_key_buf[20], buf[20];
    
    min_key_p = oc_xt_nd_min_key(s_p, node_p);
    s_p->cfg_p->key_to_string(min_key_p, min_key_buf, 20);
    
    fprintf(out_p, "    %s_%d_%s [label=\"",
            tag_p, level, min_key_buf);
    
    // describe this node
    if (!oc_xt_nd_is_leaf(s_p, node_p)) {
        // An index node
        num_entries = oc_xt_nd_num_entries(s_p, node_p);
        for (i=0; i< num_entries; i++)
        {
            struct Oc_xt_key *key_p;
            
            key_p = oc_xt_nd_get_kth_key(s_p, node_p, i);
            s_p->cfg_p->key_to_string(key_p, buf, 20);
            fprintf(out_p, "<f%d> %s", i, buf);
            if (i != num_entries-1) fprintf (out_p, "| ");
        }
        fprintf(out_p, "\"];\n");
    }
    else {
        // A leaf node, we need to describe the extents
        num_entries = oc_xt_nd_num_entries(s_p, node_p);
        for (i=0; i< num_entries; i++)
        {
            struct Oc_xt_key *key_p;
            struct Oc_xt_rcrd *rcrd_p;
            
            oc_xt_nd_leaf_get_kth(s_p, node_p, i, &key_p, &rcrd_p);
            s_p->cfg_p->rcrd_to_string(key_p, rcrd_p, buf, 20);
            fprintf(out_p, "<f%d> %s", i, buf);
            if (i != num_entries-1) fprintf (out_p, "| ");
        }
        fprintf(out_p, "\"];\n");
    }
    
    // if it is an index node then recurse down
    if (!oc_xt_nd_is_leaf(s_p, node_p)) {
        for (i=0; i<num_entries; i++) {
            Oc_xt_node *child_p;
            struct Oc_xt_key *dummy_key_p;
            uint64 addr;
            
            oc_xt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &addr); 
            child_p = s_p->cfg_p->node_get(wu_p, addr);
            
            output_b(wu_p, s_p,
                     child_p,
                     tag_p, level+1,
                     out_p);
            s_p->cfg_p->node_release(wu_p, child_p);
        }
    }
    
    // If it is an index node add pointers to the childern
    if (!oc_xt_nd_is_leaf(s_p, node_p)) {
        for (i=0; i<num_entries; i++)
        {
            Oc_xt_node *child_p;
            struct Oc_xt_key *dummy_key_p, *key_p;
            uint64 addr;
            
            oc_xt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &addr); 
            child_p = s_p->cfg_p->node_get(wu_p, addr);
            
            key_p = oc_xt_nd_min_key(s_p, child_p);
            s_p->cfg_p->key_to_string(key_p, buf, 20);
            fprintf(out_p, "    %s_%d_%s -> %s_%d_%s;\n",
                    tag_p, level, min_key_buf,
                    tag_p, level+1, buf );
            s_p->cfg_p->node_release(wu_p, child_p);            
        }
    }
}


void oc_xt_op_output_dot_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    struct Oc_utl_file *_out_p,
    char *tag_p)
{
    FILE *out_p = (FILE*) _out_p;
        
    if (oc_xt_nd_num_entries(s_p, s_p->root_node_p) == 0) {
        fprintf(out_p, "    // tree is empty\n");
    }
    else {
        output_b(wu_p, s_p, s_p->root_node_p, tag_p, 0, out_p);
    }
}
