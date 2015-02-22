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
/******************************************************************/
/* OC_BPT_INT.H
 * A generic b-plus tree module
 */
/******************************************************************/
#ifndef OC_BPT_INT_H
#define OC_BPT_INT_H

#include "pl_base.h"
#include "oc_utl_s.h"

struct Oc_rm_resource;
struct Oc_wu;

/******************************************************************/
/* Generic definitions for a generic b-plus tree.
 */

// the maximal depth of a b-tree. This limited is currently needed in the
// remove-range operation 
#define OC_BPT_MAX_HEIGHT (6)

// A key
struct Oc_bpt_key;

// A data bound to a key
struct Oc_bpt_data;

// A node in a b-tree
typedef Oc_meta_data_page_hndl Oc_bpt_node;

// A pointer to a node is always a uint64

// A set of function pointers and data to initialize a b-tree
typedef struct Oc_bpt_cfg {
    bool initialized;
    
    // size of a key (in bytes)
    int key_size;                    
    
    // size of a piece of data (in bytes)
    int data_size;                   
    
    // size of a node (in bytes)
    int node_size;                   
    
    // A way to limit the fanout of a tree nodes. Use 0 for maximum
    int root_fanout;
    int non_root_fanout;

    //--------------------------------------------------------
    // These are computed
    int max_num_ent_leaf_node;
    int max_num_ent_index_node;
    int max_num_ent_root_node;
    int leaf_ent_size;
    int index_ent_size;

    // the minimal number of entries in any node (root/leaf/index)
    int min_num_ent;
    //--------------------------------------------------------
    
    // working with nodes
    Oc_bpt_node*  ((*node_alloc)(struct Oc_wu*));             
    Oc_bpt_node*  ((*node_alloc_at)(struct Oc_wu*, uint64));             
    void          ((*node_dealloc)(struct Oc_wu*, uint64));

    // get the node with a Shared-lock
    Oc_bpt_node*  ((*node_get_sl)(struct Oc_wu*, uint64));

    // get the node with an eXclusive-lock
    Oc_bpt_node*  ((*node_get_xl)(struct Oc_wu*, uint64));

    void          ((*node_release)(struct Oc_wu*, Oc_bpt_node*));   
    void          ((*node_mark_dirty)(struct Oc_wu*, Oc_bpt_node*, bool));

    // Free-support for ref-counting
    void          ((*fs_inc_refcount)(struct Oc_wu*, uint64));
    int           ((*fs_get_refcount)(struct Oc_wu*, uint64));
    
    /* compare keys [key1] and [key2].
     *  return 0 if keys are equal
     *        -1 if key1 > key2
     *         1 if key1 < key2
     */
    int                  ((*key_compare)(struct Oc_bpt_key *key1_p,
                                         struct Oc_bpt_key *key2_p));

    // increment [key_p]. put the result in [result_p]
    void                 ((*key_inc)(struct Oc_bpt_key *key_p,
                                     struct Oc_bpt_key *result_p));
    
    // string representation of a key
    void                ((*key_to_string)(struct Oc_bpt_key *key_p,
                                          char *str_p,
                                          int max_len));
    
    /* release data. This is useful if the data is, like in an s-node,
     * an extent allocated on disk.
     */
    void                ((*data_release)(struct Oc_wu*,
                                         struct Oc_bpt_data *data_p));

    // string representation of data
    void                ((*data_to_string)(struct Oc_bpt_data *data_p,
                                           char *str_p,
                                           int max_len));
} Oc_bpt_cfg;


// The state of a b-tree. 
typedef struct Oc_bpt_state {
    // this lock is used internally. -do not- lock it externally    
    Oc_crt_rw_lock lock; 
    Oc_bpt_cfg *cfg_p;
    Oc_meta_data_page_hndl *root_node_p;
    uint64 tid;
} Oc_bpt_state;

/******************************************************************/

void oc_bpt_init(void);

void oc_bpt_init_config(Oc_bpt_cfg *cfg_p);

void oc_bpt_free_resources();

/* state operations
 *
 * [tid] is a unique id, that can be attached to this tree.
 * It is used for debugging purposes.
 */
void oc_bpt_init_state_b(
    struct Oc_wu *wu_pi,
    Oc_bpt_state *state_po,
    Oc_bpt_cfg *cfg_p,
    uint64 tid);    

void oc_bpt_destroy_state(
    struct Oc_wu *wu_pi, Oc_bpt_state *state_pi);

/* Return the TID of a tree.
 *
 * Used for debugging purposes.
 */
uint64 oc_bpt_get_tid(struct Oc_bpt_state *s_p);


/* basic operations
 */

/* create a b-tree (anywhere on disk). Return the address on disk
 * where the root is (initially) located.
 *
 */
uint64 oc_bpt_create_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p);

/* Create a b-tree whose root is in address [addr]
 *
 * The assumption is that the caller makes sure no concurrent operations
 * occur on this tree. 
 */
void oc_bpt_init_map(
    struct Oc_wu *wu_p,
    Oc_bpt_cfg *cfg_p, 
    uint64 addr);

/* If needed, COWs the root of a Btree (while write-locking the root)
 * and updates the data entry pointing to this root from a different tree;
 * We assume the node of the father tree holding this data is protected
 * under a write-lock 
 */ 
void oc_bpt_cow_root_and_update_b(struct Oc_wu *wu_p,
                                    struct Oc_bpt_state *s_p,
                                    struct Oc_bpt_data *father_data_p,
                                    int size);

/* insert a (key,data) pair into the tree.
 * data should not be NULL
 * return TRUE if this is a case of replacement. FALSE otherwise.
 * 
 * A partial path in the tree is locked for write.
 */
bool oc_bpt_insert_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p);

/* Look for a the data associated with [key_p]. If the key is found copy
 * the data into [data_po] and return TRUE. Otherwise return FALSE.
 * 
 * A partial path in the tree is locked for read.
 */
bool oc_bpt_lookup_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_po);

/* Remove a key from the tree. Invoke the [cfg_p->data_release] function on
 * the data. 
 * return TRUE if there was a (key,data) pair to delete. Return FALSE otherwise.
 * A partial path in the tree is locked for write.
 */
bool oc_bpt_remove_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p);

/* delete the whole b-tree. deallocate all nodes and data.
 * The whole tree is locked during this operation.
 */
void oc_bpt_delete_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p);

/* debugging support
 */
void oc_bpt_dbg_output_init(void);
void oc_bpt_dbg_output_end(void);

/* Output the tree in "dot" format to file [out_p]. 
 * The whole tree is locked during this operation.
 * 
 * [tag_p] is a tag used to distinguish the nodes of this tree
 *     from others printed in the same graph.
 */
void oc_bpt_dbg_output_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    char *tag_p);

/* Same as [oc_bpt_dbg_output_b] except that it can print out
 * a set of trees together. This is useful when trying to
 * print out a set of clones and we want to see the sharing
 * between them.
 * 
 * [tag_p] is a tag used to distinguish the nodes of this tree
 *     from others printed in the same graph.
 */
void oc_bpt_dbg_output_clones_b(
    struct Oc_wu *wu_p,
    int n_clones,    
    struct Oc_bpt_state *st_array[],
    char *tag_p);

/* Validate that the tree is valid.
 * The whole tree is locked during this operation.
 */
bool oc_bpt_dbg_validate_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p);

bool oc_bpt_dbg_validate_clones_b(
    struct Oc_wu *wu_p,
    int n_clones,
    Oc_bpt_state *s_array[]);

/* Computes statistics on the tree
 */
void oc_bpt_statistics_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p);

/* range operations
 */

/* Lookup a range of keys.
 * A partial path in the tree is locked for read.
 * 
 * Cursor stability is guarantied, nothing beyond that. 
 * if [data_array_po] is NULL then the data isn't filled in. 
 */
void oc_bpt_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p,
    int max_num_keys_i,
    struct Oc_bpt_key *key_array_po,
    struct Oc_bpt_data *data_array_po,
    int *nkeys_found_po);

/* Insert a range of (keys,data) pairs. Return the number of keys overwritten.
 * A partial path in the tree is locked for write.
 *
 * No atomicity guaranties are provided. 
 */
int oc_bpt_insert_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_data *data_array);

/* Remove a range of keys from the tree.
 * The whole tree is locked during this operation.
 * Return the number of keys removed. 
 */
int oc_bpt_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p);

/******************************************************************/
// query section

/* an enumeration that lists the set of operations exported by the
 * b-tree
 */
typedef enum Oc_bpt_fid {
    OC_BPT_FN_CREATE,                
    OC_BPT_FN_CREATE_AT,             
    OC_BPT_FN_DELETE,                
    OC_BPT_FN_INSERT_KEY,
    OC_BPT_FN_LOOKUP_KEY_WITH_COW,
    OC_BPT_FN_LOOKUP_KEY,            
    OC_BPT_FN_REMOVE_KEY,            
    OC_BPT_FN_DBG_OUTPUT,            
    OC_BPT_FN_DBG_VALIDATE,          
    OC_BPT_FN_LOOKUP_RANGE,          
    OC_BPT_FN_INSERT_RANGE,          
    OC_BPT_FN_REMOVE_RANGE,          
} Oc_bpt_fid;

/* Fill in [rm_p] the set of resources used by the b-tree
 *
 * [param] an additional parameter if needed.
 *   currently, is only used for the insert-range function in which case
 *   it contains the number of keys inserted. 
 */
void oc_bpt_query_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_cfg *cfg_p,
    struct Oc_rm_resource *rm_p,
    Oc_bpt_fid fid_i,
    void *param);



/******************************************************************/
// snapshot and clone section

/* clone b-tree [src_p] onto [trg_p]. This operation copies the root
 * node onto a new location (anywhere on disk) and increments the
 * ref-count on the immediate children.
 *
 * Return the address on disk where the root of the clone is
 * (initially) located.
 *
 * pre-requisit: a fresh b-tree state [trg_p] has to be created
 *   and initialized. The source tree [src_p] has to be valid. 
 */
uint64 oc_bpt_clone_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *src_p,
    struct Oc_bpt_state *trg_p);

/******************************************************************/
/* Traverse the set of nodes in tree [s_p] and apply
 * function [iter_f] to them. 
 */
void oc_bpt_iter_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    void (*iter_f)(struct Oc_wu *, Oc_bpt_node*));

#endif

