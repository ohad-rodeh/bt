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
/* OC_BPT_ND.C  
 *
 * Code handling the structure of a b+-tree node. 
 */
/**********************************************************************/
#ifndef OC_BPT_ND_H
#define OC_BPT_ND_H

#include "oc_bpt_int.h"

#include "oc_utl_s.h"
/**********************************************************************/
/*
 * There are four types of nodes:
 *   Root-Leaf : a root node that is also a leaf
 *   Root-Index : a root node that is also an index node
 *   Index : an index node (non-root)
 *   Leaf  : a leaf node (non-root)
 */
/*
  The structure of a node is as follows:
    [header]
       - page type
       - an array of size 256 containing the location inside the node of
         entries
       - if this is a root node, then space for attributes

  Since the size of keys and data is not determined at compile time then
  standard C structures cannot be used represent a node. 
*/
/*
  A leaf node is an array of entries: (key,data)
  An index node is an array of entries: (key,child_ptr = uint64)
 */
/**********************************************************************/

typedef struct Oc_bpt_nd_flags {
    unsigned root:1;
    unsigned leaf:1;
} Oc_bpt_nd_flags;

typedef struct Oc_bpt_nd_hdr {
    Oc_meta_data_page_hdr hdr;
    Oc_bpt_nd_flags flags;
    uint32 num_used_entries;
    uint8 entry_dir[256];
} OC_PACKED Oc_bpt_nd_hdr;

typedef struct Oc_bpt_nd_hdr_root {
    Oc_bpt_nd_hdr hdr;
    char attributes[OC_UTL_ATTRIBUTES_BUF_SIZE];
} OC_PACKED Oc_bpt_nd_hdr_root;

/**********************************************************************/
// some utilities

static inline struct Oc_bpt_key* oc_bpt_nd_key_array_kth(
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key* key_array,
    int k)
{
    return (struct Oc_bpt_key*) ((char*)key_array + s_p->cfg_p->key_size * k);
}

static inline struct Oc_bpt_data* oc_bpt_nd_data_array_kth(
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_data* data_array,
    int k)
{
    return (struct Oc_bpt_data*) ((char*)data_array + s_p->cfg_p->data_size * k);
}

static inline Oc_bpt_node *oc_bpt_nd_get_for_read(
    struct Oc_wu *wu_p, 
    struct Oc_bpt_state *s_p,
    uint64 addr)
{
    return s_p->cfg_p->node_get_sl(wu_p, addr);
}

Oc_bpt_node *oc_bpt_nd_get_for_write(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    uint64 addr,
    Oc_bpt_node * father_node_p_io,
    int idx_in_father);    

static inline void oc_bpt_nd_release(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p)
{
    s_p->cfg_p->node_release(wu_p, node_p);    
}

/* utility function, safe for use only for co-routine usage.
 * use with care. These functions use a static internal buffer, they
 * are not re-entrent.
 */
const char* oc_bpt_nd_string_of_key(struct Oc_bpt_state *s_p,
                                    struct Oc_bpt_key *key);
const char* oc_bpt_nd_string_of_2key(struct Oc_bpt_state *s_p,
                                     struct Oc_bpt_key *key1_p,
                                     struct Oc_bpt_key *key2_p);
const char* oc_bpt_nd_string_of_node(struct Oc_bpt_state *s_p,
                                     Oc_bpt_node *node_p);
/**********************************************************************/
void oc_bpt_nd_create_root(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_pi);

// Create a b-tree whose root is in address [addr]
void oc_bpt_nd_init_map(
    struct Oc_wu *wu_p,
    Oc_bpt_cfg *cfg_p, 
    uint64 addr);

/* Delete a node. Deallocate the node and also
 * call data_release for data in leaves.
 *
 * assumption: the node is unlocked
 */
void oc_bpt_nd_delete(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

// is this node full? 
bool oc_bpt_nd_is_full(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

// is this node a leaf? 
bool oc_bpt_nd_is_leaf(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

// set a node status to be a leaf
void oc_bpt_nd_set_leaf(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

bool oc_bpt_nd_is_root(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

int oc_bpt_nd_max_ent_in_node (
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

// return the number of entries in [node_p]
int oc_bpt_nd_num_entries(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

// [node_p] is leaf node. Get its kth entry
void oc_bpt_nd_leaf_get_kth(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    int k,
    struct Oc_bpt_key **key_ppo,
    struct Oc_bpt_data **data_ppo);

void oc_bpt_nd_index_get_kth(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    int k,
    struct Oc_bpt_key **key_ppo,
    uint64 *addr_po);

// [node_p] is an index node. Set its kth pointer. 
void oc_bpt_nd_index_set_kth(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    int k,
    struct Oc_bpt_key *key_p,
    uint64 addr);

// Remove the kth entry from [node_p]
void oc_bpt_nd_remove_kth(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    int k);

/* remove a range of keys
 * if this is a leaf node then also call the data_release function for the data.
 */
void oc_bpt_nd_remove_range(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    int idx_start,
    int idx_end);

/* return the location of the first key in [node_p] that is greater or equal to
 * [key_p]. If no such key exists return -1.
 */
int oc_bpt_nd_lookup_ge_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p);

/* return the location of the first key in [node_p] that is strictly greater than
 * [key_p]. If no such key exists return -1.
 */
int oc_bpt_nd_lookup_gt_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p);

/* return the location of the first key in [node_p] that is smaller or equal to
 * [key_p]. If no such key exists return -1.
 */
int oc_bpt_nd_lookup_le_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p);

struct Oc_bpt_key *oc_bpt_nd_get_kth_key(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    int k);

// return the smallest key in [node_p]
struct Oc_bpt_key* oc_bpt_nd_min_key(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

// return the largest key in [node_p]
struct Oc_bpt_key* oc_bpt_nd_max_key(
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

// insert a (key,data) pair into a leaf node
bool oc_bpt_nd_leaf_insert_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p);

// return the data associated with a key in a leaf node
struct Oc_bpt_data *oc_bpt_nd_leaf_lookup_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p);

/* return the node-address associated with a key in an index node.
 * The lookup is -not exact-. For example, if an index node contains two
 * keys: (5,addrA) and (10,addrB) then a lookup for 7 will return addrA.
 * In fact, a lookup for all keys between 5 and 9 will return addrA.
 *
 * There are three returned values: the child address, the key-value
 * pointing to it, and the relative location of the entry in the node.
 *
 * A returned address of zero means that the key is smaller than all keys
 * in the index-node. 
 */
uint64 oc_bpt_nd_index_lookup_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_key **exact_key_ppo,
    int *kth_po);

// return the node-address for the minimal valued key
uint64 oc_bpt_nd_index_lookup_min_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

// return the node-address for the maximal valued key
uint64 oc_bpt_nd_index_lookup_max_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

bool oc_bpt_nd_remove_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,
    struct Oc_bpt_key *key_p);

/**********************************************************************/
// used for split

/* in an index node [index_node_p] replace the bindings at index [idx]
 * with two bindings for two nodes [left_node_p] and [right_node_p].
 * It is assumed that the index-node has enough space. 
 */
void oc_bpt_nd_index_replace_w2(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *index_node_p,
    int idx, 
    Oc_bpt_node *left_node_p,
    Oc_bpt_node *right_node_p);

// replace the minimum key with a different (smaller) key
void oc_bpt_nd_index_replace_min_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *index_node_p,
    struct Oc_bpt_key *key_p);

/* split a non-root node into two. The node can be a leaf or an index node.
 *
 * assumption:
 *    the node is locked for write.
 * 
 * 1. allocate a new node [right_p]
 * 2. take a write-lock on [right_p] and mark it dirty
 * 3. move half the keys from [node_p] into [right_p]
 * 4. return [right_p]   (in write-lock mode)
 */
Oc_bpt_node *oc_bpt_nd_split(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p);

/* split a root node into two and create a new root with pointers to
 * the two childern. The root cannot move during this operation. 
 */
void oc_bpt_nd_split_root(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *root_node_p );
/**********************************************************************/
// used for remove-key

/* copy all entries from [rmv_node_p] into [root_node_p].
 * After copying [rmv_node_p] is deallocated. 
 */
void oc_bpt_nd_copy_into_root_and_dealloc(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *root_node_p,  
    Oc_bpt_node *node_p);

/* move entries from [node_p] into node [under_p] which has
 * an underflow. 
 */
void oc_bpt_nd_rebalance(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *under_p,  
    Oc_bpt_node *node_p);

/* copy all entries of [src_p] into [trg_p] 
 * pre-requisites:
 *   - the sum of entries must fit in a node.
 *   - both nodes must be taken in write-mode.
 * deallocate [src_p] when done
 */
void oc_bpt_nd_move_and_dealloc(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *trg_p,  
    Oc_bpt_node *src_p);

/* used only in remove-range. Same as [oc_bpt_nd_rebalance] except
 * that it favours [under_p]. After shuffling keys [under_p] will have
 * least b+2 entries even if [node_p] remains with only b.
 */
void oc_bpt_nd_rebalance_skewed(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *under_p,  
    Oc_bpt_node *node_p);

/**********************************************************************/
// used for insert-range

/* Try to insert an array of consecutive entries [key_array_p,
 * data_array_p] of size [length] into a leaf node.
 * Stop when there are no more entries or when the leaf is full. 
 *
 * return:
 * - the number of overwritten entries
 * - the total number of entries inserted
 *   = increment [nkeys_inserted_po]
 *
 * Assumptions:
 * 1. the leaf is locked in write mode
 * 2. The leaf is initially not full
 */
int oc_bpt_nd_insert_array_into_leaf(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *node_p,    
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_data *data_array,
    int *nkeys_inserted_po);

/**********************************************************************/
// used by remove-range

/* move the largest entry from [src_p] to [trg_p].
 *
 * assumptions:
 * - nodes [src_p] and [trg_p] are locked for write
 * - all the keys in [src_p] are smaller than all keys in [trg_p]
 */
void oc_bpt_nd_move_max_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *trg_p,
    Oc_bpt_node *src_p);

/* move the smallest entry from [src_p] to [trg_p].
 *
 * assumptions:
 * - nodes [src_p] and [trg_p] are locked for write
 * - all the keys in [src_p] are greater than all keys in [trg_p]
 */
void oc_bpt_nd_move_min_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    Oc_bpt_node *trg_p,
    Oc_bpt_node *src_p);

/**********************************************************************/

/* swap references to the b-tree root between two work-units.
 * Used in order to work correctly with the strict accounting for
 * pages performed by the PM. 
 */
void oc_bpt_nd_swap_root_ref(Oc_bpt_state *s_p,
                             struct Oc_wu *trg_wu_p,
                             struct Oc_wu *src_wu_p);
/**********************************************************************/

/* This function is used only by the clone operation.
 *
 * Copy the root node of [src_p] onto a new location, increment the
 * ref-count on the immediate children. 
 */
void oc_bpt_nd_clone_root(struct Oc_wu *wu_p,
                          struct Oc_bpt_state *src_p,
                          struct Oc_bpt_state *trg_p);

/**********************************************************************/

#endif
