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
/*********************************************************************/
/* OC_XT_ND.C  
 *
 * Code handling the structure of an x-tree node. 
 */
/**********************************************************************/
#ifndef OC_XT_ND_H
#define OC_XT_ND_H

#include "oc_xt_int.h"

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

typedef struct Oc_xt_nd_flags {
    unsigned root:1;
    unsigned leaf:1;
} Oc_xt_nd_flags;

typedef struct Oc_xt_nd_hdr {
    Oc_meta_data_page_hdr hdr;
    Oc_xt_nd_flags flags;
    uint32 num_used_entries;
    uint8 entry_dir[256];
} OC_PACKED Oc_xt_nd_hdr;

typedef struct Oc_xt_nd_hdr_root {
    Oc_xt_nd_hdr hdr;
    char attributes[OC_UTL_ATTRIBUTES_BUF_SIZE];
} OC_PACKED Oc_xt_nd_hdr_root;

/**********************************************************************/
// some utilities

static inline struct Oc_xt_key* oc_xt_nd_key_array_kth(
    struct Oc_xt_state *s_p,
    struct Oc_xt_key* key_array,
    int k)
{
    return (struct Oc_xt_key*) ((char*)key_array + s_p->cfg_p->key_size * k);
}

static inline struct Oc_xt_rcrd* oc_xt_nd_rcrd_array_kth(
    struct Oc_xt_state *s_p,
    struct Oc_xt_rcrd* rcrd_array,
    int k)
{
    return (struct Oc_xt_rcrd*) ((char*)rcrd_array + s_p->cfg_p->rcrd_size * k);
}

Oc_xt_node *oc_xt_nd_get_for_read(
    struct Oc_wu *wu_p, 
    struct Oc_xt_state *s_p,
    uint64 addr);

Oc_xt_node *oc_xt_nd_get_for_write(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    uint64 addr,
    Oc_xt_node * father_node_p_io,
    int idx_in_father);

void oc_xt_nd_release(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

/* utility function, safe for use only for co-routine usage.
 * use with care. These functions use a static internal buffer, they
 * are not re-entrent.
 */
const char* oc_xt_nd_string_of_key(struct Oc_xt_state *s_p,
                                    struct Oc_xt_key *key);
const char* oc_xt_nd_string_of_rcrd(struct Oc_xt_state *s_p,
                                    struct Oc_xt_key *key,
                                    struct Oc_xt_rcrd *rcrd);
const char* oc_xt_nd_string_of_2key(struct Oc_xt_state *s_p,
                                     struct Oc_xt_key *key1_p,
                                     struct Oc_xt_key *key2_p);
const char* oc_xt_nd_string_of_node(struct Oc_xt_state *s_p,
                                     Oc_xt_node *node_p);
/**********************************************************************/
void oc_xt_nd_create_root(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_pi);

/* Delete a node. Deallocate the node and also
 * call data_release for data in leaves.
 *
 * assumption: the node is unlocked
 */
void oc_xt_nd_delete(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

// same as [oc_xt_nd_delete] except that the node is locked
void oc_xt_nd_delete_locked(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

// is this node full? 
bool oc_xt_nd_is_full(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

/* Does [node_p] have enough space for an insert-extent operation?
 * - if [node_p] is a leaf node then we check if it 
 *   has at least two empty slots. 
 * - if [node_p] is an index node, then we just check if has zero
 *   empty slots. 
 */
bool oc_xt_nd_is_full_for_insert(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

// is this node a leaf? 
bool oc_xt_nd_is_leaf(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

// set a node status to be a leaf
void oc_xt_nd_set_leaf(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

bool oc_xt_nd_is_root(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

/* return the maximal number of entries that can fit in [node_p].
 * The node type is taken into account (root/leaf/..).
 */
int oc_xt_nd_max_ent_in_node (
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

// return the number of entries in [node_p]
int oc_xt_nd_num_entries(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

// [node_p] is leaf node. Get its kth entry
void oc_xt_nd_leaf_get_kth(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k,
    struct Oc_xt_key **key_ppo,
    struct Oc_xt_rcrd **rcrd_ppo);

// return a pointer to the attribute area in root node [node_p]
char *oc_xt_nd_get_root_attrs(Oc_xt_node *node_p);

// [node_p] is an index node. Get its kth pointer. 
void oc_xt_nd_index_get_kth(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k,
    struct Oc_xt_key **key_ppo,
    uint64 *addr_po);

// [node_p] is an index node. Set its kth pointer. 
void oc_xt_nd_index_set_kth(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k,
    struct Oc_xt_key *key_p,
    uint64 addr);

// Remove the kth entry from [node_p]
void oc_xt_nd_remove_kth(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k);

typedef struct Oc_xt_nd_spill {
    bool flag;
    struct Oc_xt_key *key_p;
    struct Oc_xt_rcrd *rcrd_p;
} Oc_xt_nd_spill;

/* Remove a range from a leaf node, call the data_release function for
 * the data. return the total length released.
 *
 * There is a corner case where the removed range is in the middle of
 * an existing extent E. This causes E to be split into E1,E2,E3; E2
 * is released and E3 is added to the node. If the node was full when
 * the operation began then there is no space to put E3 in. The spill
 * structure provided by the caller then holds E3. 
 *
 * If the [spill_p] structure is fill in then spill_p->flag is set to
 * TRUE.
 * 
 * [spill_p] may be NULL. This means that the caller has checked in
 * advance and there will be space for this case. 
 */
uint64 oc_xt_nd_leaf_remove_range(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    Oc_xt_nd_spill *spill_p);

// Compute the total length of the extents in leaf [node_p]
uint64 oc_xt_nd_leaf_length(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p);

// Remove a range of entries from a node
void oc_xt_nd_remove_range_of_entries(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int idx_start,
    int idx_end);

struct Oc_xt_key *oc_xt_nd_get_kth_key(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k);

// return the smallest key in [node_p]
struct Oc_xt_key* oc_xt_nd_min_key(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

/* return the largest key in [node_p].
 *
 * For example, if the extents in [node_p] are [1-3] [10-13]
 * then the maximal key is 10.
 */
struct Oc_xt_key* oc_xt_nd_max_key(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

/* return the largest offset in [node_p]
 *
 * Since we are dealing with extents, this means the end-key of the
 * highest extent. For example, if the extents in the node are:
 * [1-3] [10-13] then the maximal key is 13. 
 *
 * The caller needs to allocate space for the result key [max_ofs_po].
 */
void oc_xt_nd_max_ofs(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *max_ofs_po);    

/* return the location of the first key in [node_p] that is greater or
 * equal to [key_p]. If no such key exists return -1.
 */
int oc_xt_nd_index_lookup_ge_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p);

/* return the location of the first key in [node_p] that is smaller or equal to
 * [key_p]. If no such key exists return -1.
 */
int oc_xt_nd_index_lookup_le_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p);

/* return the location of the first extent in [node_p] that is greater or
 * equal to [key_p]. If no such key exists return -1.
 *
 * note: [key_p] may be in the middle of the returned extent.
 */
int oc_xt_nd_leaf_lookup_ge_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p);

/* return the location of the first extent in [node_p] that is smaller or equal to
 * [key_p]. If no such key exists return -1.
 *
 * note: [key_p] may be in the middle of the returned extent.
 */
int oc_xt_nd_leaf_lookup_le_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p);

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
uint64 oc_xt_nd_index_lookup_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_key **exact_key_ppo,
    int *kth_po);

// return the node-address for the minimal valued key
uint64 oc_xt_nd_index_lookup_min_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

/**********************************************************************/
// used for split

/* in an index node [index_node_p] replace binding for index [k]
 * with two bindings for two nodes [left_node_p] and [right_node_p].
 * It is assumed that the index-node has enough space. 
 */
void oc_xt_nd_index_replace_w2(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *index_node_p,
    int k,
    Oc_xt_node *left_node_p,
    Oc_xt_node *right_node_p);

// replace the minimum key with a different (smaller) key
void oc_xt_nd_index_replace_min_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *index_node_p,
    struct Oc_xt_key *key_p);

/* split a non-root node into two. The node can be a leaf or an index node.
 * 1. allocate a new node [right_p]
 * 2. take a write-lock on [right_p] and mark it dirty
 * 3. move half the keys from [node_p] into [right_p]
 * 4. return [right_p]   (in write-lock mode)
 */
Oc_xt_node *oc_xt_nd_split(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p);

/* split a root node into two and create a new root with pointers to
 * the two childern. The root cannot move during this operation. 
 */
void oc_xt_nd_split_root(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *root_node_p );
/**********************************************************************/
// used for remove-key

/* copy all entries from [rmv_node_p] into [root_node_p].
 * After copying [rmv_node_p] is deallocated. 
 */
void oc_xt_nd_copy_into_root_and_dealloc(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *root_node_p,  
    Oc_xt_node *node_p);

/* copy all entries of [src_p] into [trg_p] 
 * pre-requisites:
 *   - the sum of entries must fit in a node.
 *   - both nodes must be taken in write-mode.
 * deallocate [src_p] when done
 */
void oc_xt_nd_move_and_dealloc(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *trg_p,  
    Oc_xt_node *src_p);

/* used only in remove-range. Same as [oc_xt_nd_rebalance] except
 * that it favours [under_p]. After shuffling keys [under_p] will have
 * least b+2 entries even if [node_p] remains with only b.
 */
void oc_xt_nd_rebalance_skewed(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *under_p,  
    Oc_xt_node *node_p);

/**********************************************************************/
// used for insert-range

/* Insert an extent [key_p, rcrd_p] into a node. 
 *
 * return:
 * - the length of the area overwritten
 *
 * Assumptions:
 * 1. the leaf is locked in write mode
 * 2. The leaf is initially not full
 */
uint64 oc_xt_nd_insert_into_leaf(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,    
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p);

/**********************************************************************/
// used by remove-range

/* move the largest entry from [src_p] to [trg_p].
 *
 * assumptions:
 * - nodes [src_p] and [trg_p] are locked for write
 * - all the keys in [src_p] are smaller than all keys in [trg_p]
 */
void oc_xt_nd_move_max_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *trg_p,
    Oc_xt_node *src_p);

/* move the smallest entry from [src_p] to [trg_p].
 *
 * assumptions:
 * - nodes [src_p] and [trg_p] are locked for write
 * - all the keys in [src_p] are greater than all keys in [trg_p]
 */
void oc_xt_nd_move_min_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *trg_p,
    Oc_xt_node *src_p);

/**********************************************************************/

/* swap references to the b-tree root between two work-units.
 * Used in order to work correctly with the strict accounting for
 * pages performed by the PM. 
 */
void oc_xt_nd_swap_root_ref(Oc_xt_state *s_p,
                            struct Oc_wu *trg_wu_p,
                            struct Oc_wu *src_wu_p);
/**********************************************************************/

#endif
