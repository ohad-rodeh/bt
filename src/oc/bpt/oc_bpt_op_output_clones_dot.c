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
/* OC_BPT_OP_OUTPUT_DOT.H
 *
 * write out a representation of a node in "dot"
 * format into a file.
 */
/**********************************************************************/
#include <stdio.h>

#include "oc_bpt_int.h"
#include "oc_bpt_nd.h"
#include "oc_bpt_trace.h"
#include "oc_bpt_label.h"
/**********************************************************************/

/* Label used to denote a node that has already been touched
 * The assumption is that the graph has less than [TOUCHED]
 * nodes. 
 */
#define TOUCHED (1000000)

/**********************************************************************/

static void touch_node(struct Oc_wu *wu_p,
                       struct Oc_bpt_state *s_p,
                       Oc_bpt_node *node_p);
static bool touch_already(struct Oc_wu *wu_p,
                          struct Oc_bpt_state *s_p,
                          Oc_bpt_node *node_p);

/**********************************************************************/

static void touch_node(struct Oc_wu *wu_p,
                          struct Oc_bpt_state *s_p,
                          Oc_bpt_node *node_p)
{
    int *node_label_p = oc_bpt_label_get(wu_p, node_p);
    *node_label_p += TOUCHED;
}
                
static bool touch_already(struct Oc_wu *wu_p,
                          struct Oc_bpt_state *s_p,
                          Oc_bpt_node *node_p)
{
    int *node_label_p = oc_bpt_label_get(wu_p, node_p);
    return ((*node_label_p) > TOUCHED);
}

static int get_label(int node_label)
{
    if (node_label > TOUCHED) return node_label - TOUCHED;
    else return node_label;
}
    
// Erase all previous labels
static void erase_labels_b(struct Oc_wu *wu_p,
                           struct Oc_bpt_state *s_p,
                           Oc_bpt_node *node_p)
{
    int i, num_entries;
    int *node_label_p;
        
    // Erase this node's label
    node_label_p = oc_bpt_label_get(wu_p, node_p);
    *node_label_p = 0;

    num_entries = oc_bpt_nd_num_entries(s_p, node_p);
    
    // If it is an index node then recurse into the childern
    if (!oc_bpt_nd_is_leaf(s_p, node_p)) {
        for (i=0; i<num_entries; i++)
        {
            Oc_bpt_node *child_p;
            struct Oc_bpt_key *dummy_key_p;
            uint64 addr;
            
            oc_bpt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &addr); 
            child_p = oc_bpt_nd_get_for_read(wu_p, s_p, addr);

            erase_labels_b(wu_p, s_p, child_p);

            s_p->cfg_p->node_release(wu_p, child_p);            
        }
    }
}

/* Recurse through a tree, label all the nodes with unique integer
 * labels and print out node descriptions. 
 */
static void print_labels_b(struct Oc_wu *wu_p,
                           struct Oc_bpt_state *s_p,
                           Oc_bpt_node *node_p,                  
                           char *tag_p,
                           int *max_label_p)
{
    char key_buf[20];
    int *node_label_p, num_entries, i, start_idx;
    struct Oc_bpt_key *key_p;
    
    // describe this node

    node_label_p = oc_bpt_label_get(wu_p, node_p);    

    if (*node_label_p > 0) {
        // This node has already been labeled.
        return;
    }

    /* This node has not been labeled yet.
     * Set the label to (max-label + 1)
     */
    (*max_label_p)++;
    *node_label_p = *max_label_p;
    printf("    %s_%d [label=\"", tag_p, *max_label_p);

    num_entries = oc_bpt_nd_num_entries(s_p, node_p);
    start_idx = 0;
    
    if (oc_bpt_nd_is_root(s_p, node_p)) {
        // This is a root node, print the TID
        printf("<f0> C%Lu", s_p->tid);
        if (0 != num_entries) printf ("| ");        
        start_idx=1;
    }
    else {
        // This is a non-root node, print the free-space refcount
        int fs_refcnt = s_p->cfg_p->fs_get_refcount(wu_p, node_p->disk_addr);
    
        printf("<f0> FS%d", fs_refcnt);
        if (0 != num_entries) printf ("| ");        
        start_idx=1;
    }
    
    // list the set of keys in the node
    for (i=0; i < num_entries; i++)
    {
        key_p = oc_bpt_nd_get_kth_key(s_p, node_p, i);
        s_p->cfg_p->key_to_string(key_p, key_buf, 20);
        printf("<f%d> %s", start_idx+i, key_buf);
        if (i != num_entries-1) printf ("| ");
    }
    // fprintf(out_p, "| <f%d> adr=%Lu", num_entries, node_p->disk_addr);
    printf("\"];\n");
    
    // if it is an index node then recurse down
    if (!oc_bpt_nd_is_leaf(s_p, node_p)) {
        for (i=0; i<num_entries; i++) {
            Oc_bpt_node *child_p;
            struct Oc_bpt_key *dummy_key_p;
            uint64 addr;
            
            oc_bpt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &addr); 
            child_p = oc_bpt_nd_get_for_read(wu_p, s_p, addr);
            
            print_labels_b(wu_p,
                           s_p,
                           child_p,
                           tag_p, 
                           max_label_p);
            s_p->cfg_p->node_release(wu_p, child_p);
        }
    }
}    

static void print_edges_b(struct Oc_wu *wu_p,
                          struct Oc_bpt_state *s_p,
                          Oc_bpt_node *node_p,                  
                          char *tag_p)
{
    int i, num_entries, node_label, child_label;
    
    num_entries = oc_bpt_nd_num_entries(s_p, node_p);
    
    // If it is an index node add pointers to the childern
    if (!oc_bpt_nd_is_leaf(s_p, node_p)) {
        for (i=0; i<num_entries; i++)
        {
            Oc_bpt_node *child_p;
            struct Oc_bpt_key *dummy_key_p;
            uint64 addr;
            
            oc_bpt_nd_index_get_kth(s_p, node_p, i, &dummy_key_p, &addr); 
            child_p = oc_bpt_nd_get_for_read(wu_p, s_p, addr);
            
            node_label = *(oc_bpt_label_get(wu_p, node_p));
            child_label = *(oc_bpt_label_get(wu_p, child_p));
            
            printf("    %s_%d -> %s_%d;\n",
                    tag_p, get_label(node_label),
                    tag_p, get_label(child_label) );
            
            if (!touch_already(wu_p, s_p, child_p)) {
                // mark the node so this edge will not be drawn again.
                touch_node(wu_p, s_p, child_p);
            
                // recurse to the child node
                print_edges_b(wu_p, s_p, child_p, tag_p);
            }
            else {
                /* this node has already been touched, the tree
                 * below it has already been drawn.
                 */
            }
            s_p->cfg_p->node_release(wu_p, child_p);            
        }
    }
}

/**********************************************************************/

void oc_bpt_op_output_clones_dot_b(
    struct Oc_wu *wu_p,
    int n_clones,
    struct Oc_bpt_state *st_array[],
    char *tag_p)
{
    int i;
    int max_label;

    oc_bpt_label_init(10000);
    
     /* phase 1: recurse through the trees:
      *     - put unique integer labels on all nodes
      *     - print descriptions of all nodes
      * phase 2: recurse through the tree printing the edges.
      */

    /* Separate this set of clones from other sets
     * by putting a rectangle around them. 
     */
    printf("subgraph cluster_%s {\n", tag_p);
    
    // phase 0, erase all previous labels
    for (i=0; i < n_clones; i++) {
        struct Oc_bpt_state *s_p = st_array[i];
        
        if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) > 0)
            erase_labels_b(wu_p,
                           s_p,
                           s_p->root_node_p);
    }
    
    // phase 1, label all the nodes
    max_label = 0;
    for (i=0; i < n_clones; i++) {
        struct Oc_bpt_state *s_p = st_array[i];
        
        if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) == 0)
            printf("    // tree is empty\n");
        else 
            print_labels_b(wu_p,
                           s_p,
                           s_p->root_node_p,
                           tag_p,
                           &max_label);
    }
    
    
    // phase 2, recurse through the tree printing the edges.
    for (i=0; i < n_clones; i++) {
        struct Oc_bpt_state *s_p = st_array[i];
        
        if (oc_bpt_nd_num_entries(s_p, s_p->root_node_p) > 0)
            print_edges_b(wu_p,
                          s_p,
                          s_p->root_node_p,
                          tag_p);
    }

    printf("}\n");
    oc_bpt_label_free();
}

/**********************************************************************/
