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
/* OC_XT_INT.H
 *
 * A b-plus tree containing extents
 */
/******************************************************************/
#ifndef OC_XT_INT_H
#define OC_XT_INT_H

//#include "pl_base.h"
#include "oc_xt_s.h"


struct Oc_rm_resource;
struct Oc_wu;
struct Oc_utl_file;
struct Oc_bpt_data;

/******************************************************************/
/* Generic definitions for a generic b-plus tree.
 */

// the maximal depth of a b-tree. This limited is currently needed in the
// remove-range operation 
#define OC_XT_MAX_HEIGHT (6)

// A key
struct Oc_xt_key;

// A record bound to a key. The record includes data and length. 
struct Oc_xt_rcrd;

// A node in a b-tree
typedef Oc_meta_data_page_hndl Oc_xt_node;

// A pointer to a node is always a uint64

/* The possible results when comparing two extents A and B.
 *
 * There are six possible results when comparing extents. Each of the
 * possible outcomes is mutually exclusive to the others. For example,
 * if A=[10-13] and b=[10-13] then the only legal comparison result is
 * OC_XT_RC_EQUAL. A OC_XT_RC_FULLY_COVERS, or OC_XT_RC_COVERED value
 * is incorrect.
 *
 * In other words, the comparison is strict. 
 */
typedef enum Oc_xt_cmp {
    // A is strictly smaller than B. For example A=[10-13] B=[15-16].
    OC_XT_CMP_SML,

    // A is strictly larger than B. For example A=[10-13]  B=[8-9].
    OC_XT_CMP_GRT,   

    // A and B cover exactly the same extent. For example A=[10-13] B=[10-13].
    OC_XT_CMP_EQUAL,
    
    // A is (strictly) covered by B. For example A=[10-13]  B=[9-15].
    OC_XT_CMP_COVERED,

    // A (strictly) fully covers B. For example A=[10-13]  B=[11-12].
    OC_XT_CMP_FULLY_COVERS,

    // A and B (strictly) partially overlap. A is less than B,
    // For example A=[10-13] B=[12-20].
    OC_XT_CMP_PART_OVERLAP_SML,
    
    // A and B (strictly) partially overlap. A is greater than B,
    // For example A=[10-13] B=[5-10].
    OC_XT_CMP_PART_OVERLAP_GRT, 
    
} Oc_xt_cmp;

// A set of function pointers and data to initialize an x-tree
typedef struct Oc_xt_cfg {
    bool initialized;
    
    // size of a key (in bytes)
    int key_size;                    
    
    // size of a record (in bytes)
    int rcrd_size;                   
    
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
    Oc_xt_node*  ((*node_alloc)(struct Oc_wu*));             
    Oc_xt_node*  ((*node_alloc_at)(struct Oc_wu*, uint64));             
    void         ((*node_dealloc)(struct Oc_wu*, uint64));
    Oc_xt_node*  ((*node_get)(struct Oc_wu*, uint64));
    void         ((*node_release)(struct Oc_wu*, Oc_xt_node*));   
    void         ((*node_mark_dirty)(struct Oc_wu*, Oc_xt_node*));
    
    /* compare keys [key1] and [key2].
     *  return 0 if keys are equal
     *        -1 if key1 > key2
     *         1 if key1 < key2
     */
    int          ((*key_compare)(struct Oc_xt_key *key1_p,
                                 struct Oc_xt_key *key2_p));

    // increment [key_p]. put the result in [result_p]
    void         ((*key_inc)(struct Oc_xt_key *key_p,
                             struct Oc_xt_key *result_p));
    
    // string representation of a key
    void         ((*key_to_string)(struct Oc_xt_key *key_p,
                                   char *str_p,
                                   int max_len));
    
    // compare two extents 
    Oc_xt_cmp    ((*rcrd_compare)(struct Oc_xt_key *key1_p,
                                  struct Oc_xt_rcrd *rcrd1_p,
                                  struct Oc_xt_key *key2_p,
                                  struct Oc_xt_rcrd *rcrd2_p));
    
    /* compare a key to an extent.
     *  return 0 if key1 is inside the extent
     *        -1 if key1 > key2,rcrd2
     *         1 if key1 < key2
     */
    int          ((*rcrd_compare0)(struct Oc_xt_key *key1_p,
                                   struct Oc_xt_key *key2_p,
                                   struct Oc_xt_rcrd *rcrd2_p));
    
    
    /* Compare an extent against an upper and lower boundary.
     * The return value is of type Oc_xt_cmp. In the comparison
     * the boundary is considered extent B. 
     *
     * The extent can split, at most, to three parts:
     *   A) before [min_key_p]
     *   B) between [min_key_p] and [max_key_p]
     *   C) after  [max_key_p]
     *
     * The result array is filled with extents A,B and C.
     * The caller can put NULL pointers in the result; such 
     * entries will not filled. In many cases, not all of A,B,C exist.
     * In these cases the result is filled with NULL. 
     *
     * Examples:
     *   split extent=[10-14]  min_key=11  max_key=13
     *   result:
     *           [10-10] [11-13] [14-14]
     *
     *   split extent=[10-14]  min_key=8  max_key=20
     *   result:
     *           NULL [10-14] NULL
     *   
     *   split extent=[10-14]  min_key=11  max_key=14
     *   result:
     *           [10-10] [11-14] NULL
     */
    Oc_xt_cmp      ((*rcrd_bound_split)(
                      // record+key to chop
                      struct Oc_xt_key *key_p,
                      struct Oc_xt_rcrd *rcrd_p,
                      
                      // boundary keys
                      struct Oc_xt_key *min_key_p,
                      struct Oc_xt_key *max_key_p,
                      
                      // resulting sub-extents
                      struct Oc_xt_key *key_array_p[3],
                      struct Oc_xt_rcrd *rcrd_array_p[3] ));

    /* same function but with a second extent to compare against instead
     * of boundary keys.
     */
    Oc_xt_cmp      ((*rcrd_split)(
                      // record+key to chop
                      struct Oc_xt_key *key1_p,
                      struct Oc_xt_rcrd *rcrd1_p,
                      
                      // record to provide boundary keys
                      struct Oc_xt_key *key2_p,
                      struct Oc_xt_rcrd *rcrd2_p,
                      
                      // resulting sub-extents
                      struct Oc_xt_key *key_array_p[3],
                      struct Oc_xt_rcrd *rcrd_array_p[3] ));    

    /* return the end offset of an extent. The end offset is part
     * of the extent. For example, if E=[10-13] then 13 is the end-offset
     * and it is still -in- E.
     */
    void           ((*rcrd_end_offset)(struct Oc_xt_key *key_p,
                                       struct Oc_xt_rcrd *rcrd_p,
                                       
                                       // returned key 
                                       struct Oc_xt_key *end_key_po));
    
    // Chop the first [len] unit from the the record
    void           ((*rcrd_chop_length)(struct Oc_xt_key *key_po,
                                        struct Oc_xt_rcrd *rcrd_po,
                                        uint64 len));

    /* Chop extent [key_po, rcrd_po] from above. Limit it
     * from above to [hi_key_p].
     */
    void           ((*rcrd_chop_top)(struct Oc_xt_key *key_po,
                                     struct Oc_xt_rcrd *rcrd_po,
                                     struct Oc_xt_key *hi_key_p));
    
    /* Split extent [key_p, rcrd_p] into [num] sub-extents. Write the
     * new extents into [key_array] and [rcrd_array].
     */
    void           ((*rcrd_split_into_sub)(struct Oc_xt_key *key_p,
                                           struct Oc_xt_rcrd *rcrd_p,
                                           int num,
                                           struct Oc_xt_key *key_array,
                                           struct Oc_xt_rcrd *rcrd_array ));
    
    // Return the length of the extent
    uint64         ((*rcrd_length)(struct Oc_xt_key *key_p,
                                   struct Oc_xt_rcrd *rcrd_p));
    
    // release record. This is useful if the record represents an extent on disk
    void           ((*rcrd_release)(struct Oc_wu*,
                                    struct Oc_xt_key *key_p,
                                    struct Oc_xt_rcrd *rcrd_p));
    
    // string representation of an extent
    void           ((*rcrd_to_string)(struct Oc_xt_key *key_p,
                                      struct Oc_xt_rcrd *rcrd_p,
                                      char *str_p,
                                      int max_len));
    
    //--------------------------------------------------------
    // query function for FS
    void           ((*fs_query_alloc)(struct Oc_rm_resource *r_p, int n_pages));
    void           ((*fs_query_dealloc)(struct Oc_rm_resource *r_p, int n_pages));
} Oc_xt_cfg;


/******************************************************************/

void oc_xt_init(void);
void oc_xt_free_resources(void);

void oc_xt_init_config(Oc_xt_cfg *cfg_p);

/* state operations
 */
void oc_xt_init_state_b(
    struct Oc_wu *wu_pi, Oc_xt_state *state_po, Oc_xt_cfg *cfg_p);
void oc_xt_destroy_state(
    struct Oc_wu *wu_pi, Oc_xt_state *state_pi);


/* basic operations
 */

/* create an x-tree (anywhere on disk). Return the address on disk where the root
 * is (initially) located.
 */
uint64 oc_xt_create_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p);

/* delete the whole x-tree. deallocate all nodes and records.
 * The whole tree is locked during this operation.
 */
void oc_xt_delete_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p);

/* If needed, COWs the root of an Xtree (while write-locking the root)
 * and updates the data entry pointing to this root from a different tree
 * which is a BPT tree;
 * We assume the node of the father tree holding this data is protected
 * under a write-lock 
 */ 
void oc_xt_cow_root_and_update_b(struct Oc_wu *wu_p,
                                 struct Oc_xt_state *s_p,
                                 struct Oc_bpt_data *father_data_p,
                                 int size);


/* debugging support
 */
void oc_xt_dbg_output_init(struct Oc_utl_file *file_out_p);
void oc_xt_dbg_output_end(struct Oc_utl_file *file_out_p);

/* Output the tree in "dot" format to file [out_p]. 
 * The whole tree is locked during this operation.
 */
void oc_xt_dbg_output_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    struct Oc_utl_file *file_out_p,
    char *tag_p);

/* Validate that the tree is valid.
 * The whole tree is locked during this operation.
 */
bool oc_xt_dbg_validate_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p);

/* Computes statistics on the tree
 */
void oc_xt_statistics_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p);

/* Lookup in a range, return the set of extents inside the range.
 * Return also extents that have a partial overlap with the
 * range. Chop them if necessary. For example, if the range is [3-10]
 * and there is an extent [1-4] then the returned value will include an
 * extent [3-4].
 *
 * A partial path in the tree is locked for read.
 * 
 * Cursor stability is guarantied, nothing beyond that. 
 * if [rcrd_array_po] is NULL then the data isn't filled in. 
 */
void oc_xt_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    int max_num_keys_i,
    struct Oc_xt_key *key_array_po,
    struct Oc_xt_rcrd *rcrd_array_po,
    int *n_found_po);

/* insert an extent into the tree. return the length
 * of overwrite. For example, if the inserted extent ovelaps an
 * existing extent for 1000 units then the return value will be 1000.
 * 
 * A partial path in the tree is locked for write.
 */
uint64 oc_xt_insert_range_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p);

/* Remove a range from the tree.
 * The whole tree is locked during this operation.
 * Return the total length removed. 
 */
uint64 oc_xt_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p);

/* swap references to the b-tree root between two work-units.
 * Used in order to work correctly with the strict accounting for
 * pages performed by the PM. 
 */
void oc_xt_swap_root_ref(Oc_xt_state *s_p,
                         struct Oc_wu *trg_wu_p,
                         struct Oc_wu *src_wu_p);

// A string representation of the comparison return code
const char *oc_xt_string_of_cmp_rc(Oc_xt_cmp rc);

/******************************************************************/
// query section

/* an enumeration that lists the set of operations exported by the
 * b-tree
 */
typedef enum Oc_xt_fid {
    OC_XT_FN_CREATE,                
    OC_XT_FN_CREATE_AT,             
    OC_XT_FN_DELETE,                
    OC_XT_FN_DBG_OUTPUT,            
    OC_XT_FN_DBG_VALIDATE,          
    OC_XT_FN_LOOKUP_RANGE,          
    OC_XT_FN_INSERT_RANGE,          
    OC_XT_FN_REMOVE_RANGE,          
    OC_XT_FN_GET_ATTR, 
    OC_XT_FN_SET_ATTR,
} Oc_xt_fid;

/* Fill in [rm_p] the set of resources used by the b-tree
 *
 * [param] an additional parameter if needed.
 *   currently, is only used for the insert-range function in which case
 *   it contains the number of keys inserted. 
 */
void oc_xt_query_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_cfg *cfg_p,
    struct Oc_rm_resource *rm_p,
    Oc_xt_fid fid_i,
    void *param);    

/******************************************************************/

#endif

