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
/* OC_XT_ND.C  
 *
 * Code handling the structure of a b+-tree node. 
 */
/**********************************************************************/
#include <string.h>
#include <alloca.h>

#include "oc_xt_int.h"
#include "oc_xt_nd.h"
#include "oc_xt_trace.h"
#include "oc_utl_trk.h"
/**********************************************************************/

typedef struct Nd_leaf_ent_ptrs {
    struct Oc_xt_key *key_p;
    struct Oc_xt_rcrd *rcrd_p;
} Nd_leaf_ent_ptrs;

typedef struct Nd_index_ent_ptrs {
    struct Oc_xt_key *key_p;
    uint64 *addr_p;
} Nd_index_ent_ptrs;

typedef enum Nd_search {
    SEARCH_HI,    
    SEARCH_LO,
    SEARCH_MID
} Nd_search;

/* An opaque type used to represent the array of (key,data/addr) pairs beyond the
 * node header.
 */
struct Oc_xt_nd_array;

// prototypes
static inline Oc_xt_nd_hdr* get_hdr(Oc_xt_node *node_p);
static inline uint64 get_node_addr(Oc_xt_node *node_p);
static inline int num_entries(Oc_xt_nd_hdr *hdr_p);

static void oc_xt_nd_dealloc_node(struct Oc_wu *wu_p,
                                   struct Oc_xt_state *s_p,
                                   Oc_xt_node *node_p);
static int max_ent_in_hdr (struct Oc_xt_state *s_p, Oc_xt_nd_hdr *hdr_p);
static struct Oc_xt_nd_array* get_start_array(struct Oc_xt_state *s_p,
                                               Oc_xt_node *node_p);
static bool check_bounds( struct Oc_xt_state *s_p,
                          Oc_xt_nd_hdr *hdr_p,
                          int kth);
static void get_kth_leaf_entry(struct Oc_xt_state *s_p,
                               Oc_xt_nd_hdr *hdr_p,
                               struct Oc_xt_nd_array *arr_p, 
                               struct Nd_leaf_ent_ptrs *ent_ptrs_p,
                               int k);
static void get_kth_index_entry(struct Oc_xt_state *s_p,
                                Oc_xt_nd_hdr *hdr_p,
                                struct Oc_xt_nd_array *arr_p, 
                                struct Nd_index_ent_ptrs *ent_ptrs_p,
                                int k);
static struct Oc_xt_key* get_kth_key(struct Oc_xt_state *s_p,
                                     Oc_xt_nd_hdr *hdr_p,
                                     struct Oc_xt_nd_array *arr_p, 
                                     int k);
static int search_for_key(struct Oc_xt_state *s_p,
                          Oc_xt_node *node_p,
                          struct Oc_xt_key *key_p,
                          int *idx_for_insert_po,
                          Nd_search *src_po);
static int oc_xt_nd_lookup_le_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p);
static int oc_xt_nd_lookup_ge_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p);

static void entry_swap(Oc_xt_nd_hdr *hdr_p, int i, int j);
static void shuffle_insert_key(Oc_xt_nd_hdr *hdr_p, int loc);
static void shuffle_remove_key(Oc_xt_nd_hdr *hdr_p, int idx);
static void shuffle_remove_all_above_k(Oc_xt_nd_hdr *hdr_p, int idx);
static void shuffle_remove_all_below_k(Oc_xt_nd_hdr *hdr_p, int idx);
static void shuffle_remove_key_range(Oc_wu *wu_p,
                                     Oc_xt_nd_hdr *hdr_p,
                                     int idx_start, int idx_end);
static void replace_index_entry(struct Oc_xt_state *s_p,
                                Oc_xt_nd_hdr *hdr_p,
                                struct Oc_xt_nd_array *arr_p, 
                                int idx,
                                struct Oc_xt_key *key_p,
                                uint64 *addr_p);

static void replace_index_entry_addr(struct Oc_xt_state *s_p,
                                     Oc_xt_nd_hdr *hdr_p,
                                     struct Oc_xt_nd_array *arr_p, 
                                     int idx,
                                     uint64 *old_addr_p,
                                     uint64 *new_addr_p);

static void alloc_new_leaf_entry(struct Oc_wu *wu_p,
                                 struct Oc_xt_state *s_p,
                                 Oc_xt_nd_hdr *hdr_p,
                                 struct Oc_xt_nd_array *arr_p,
                                 struct Oc_xt_key *key_p,
                                 struct Oc_xt_rcrd *rcrd_p);
static void alloc_new_index_entry(struct Oc_xt_state *s_p,
                                  Oc_xt_nd_hdr *hdr_p,
                                  struct Oc_xt_nd_array *arr_p,
                                  struct Oc_xt_key *key_p,
                                  uint64 *addr_p);
#if 0
static bool pre_remove_verify_num_entries(struct Oc_xt_state *s_p,
                                          Oc_xt_node *node_p);
#endif

static void rebalance(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *under_p,  
    Oc_xt_node *node_p,
    bool skewed);
static void init_root(
    struct Oc_wu *wu_p,
    Oc_xt_node *node_pi);
static uint64 remove_part(
    Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    Oc_xt_nd_hdr *hdr_p,
    struct Oc_xt_nd_array *arr_p,
    int k,
    bool one_subextent_at_most,
    Oc_xt_nd_spill *spill_p);


/**********************************************************************/

static inline int num_entries(Oc_xt_nd_hdr *hdr_p)
{
    return (int) hdr_p->num_used_entries;
}

static inline void copy_rcrd(Oc_xt_state *s_p,
                             struct Oc_xt_key *trg_key_p,
                             struct Oc_xt_rcrd *trg_rcrd_p,
                             struct Oc_xt_key *src_key_p,
                             struct Oc_xt_rcrd *src_rcrd_p)
{
    memcpy((char*)trg_key_p, (char*)src_key_p, s_p->cfg_p->key_size);
    memcpy((char*)trg_rcrd_p, (char*)src_rcrd_p, s_p->cfg_p->rcrd_size);
}

Oc_xt_node *oc_xt_nd_get_for_read(
    Oc_wu *wu_p, 
    struct Oc_xt_state *s_p,
    uint64 addr)
{
    Oc_xt_node *node_p;
    
    node_p = s_p->cfg_p->node_get(wu_p, addr);
    oc_utl_trk_crt_lock_read(wu_p, &node_p->lock);
    return node_p;
}

/* grabs write lock; if child address has changed due to COW,
 * father's entr pointing to child is updated.
 */

Oc_xt_node *oc_xt_nd_get_for_write(
    Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    uint64 addr,
    Oc_xt_node * father_node_p_io,
    int idx_in_father)
{
    Oc_xt_node *node_p;
    uint64 new_addr;    
    
    node_p = s_p->cfg_p->node_get(wu_p, addr);
    oc_utl_trk_crt_lock_write(wu_p, &node_p->lock);
    s_p->cfg_p->node_mark_dirty(wu_p, node_p);
  
    /* If node was just COWed, and we got a father node that points to it
     * then the address in the father should be updated.
     * (father is supposed to be already write-locked and is necessarily
     * an index node)
     */
    new_addr = get_node_addr(node_p);
    if ( (father_node_p_io) && (new_addr != addr) ) { 
        // child node was COWed
        struct Oc_xt_nd_array *arr_p = 
            get_start_array(s_p, father_node_p_io);
        Oc_xt_nd_hdr *hdr_p = 
            get_hdr(father_node_p_io);

        replace_index_entry_addr(s_p, hdr_p, arr_p, idx_in_father,
                                 &addr, &new_addr);
    }
       
    return node_p;
}

void oc_xt_nd_release(
    Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    oc_utl_trk_crt_unlock(wu_p, &node_p->lock);
    s_p->cfg_p->node_release(wu_p, node_p);
}

const char* oc_xt_nd_string_of_key(struct Oc_xt_state *s_p,
                                    struct Oc_xt_key *key)
{
    static char s[20];

    s_p->cfg_p->key_to_string(key, s, 20);
    return s;
}

const char* oc_xt_nd_string_of_rcrd(struct Oc_xt_state *s_p,
                                    struct Oc_xt_key *key_p,
                                    struct Oc_xt_rcrd *rcrd_p)
{
    static char s[30];

    s_p->cfg_p->rcrd_to_string(key_p, rcrd_p, s, 30);
    return s;
}

const char* oc_xt_nd_string_of_2key(struct Oc_xt_state *s_p,
                                     struct Oc_xt_key *key1_p,
                                     struct Oc_xt_key *key2_p)
{
    static char s[40];
    int len;
    
    s_p->cfg_p->key_to_string(key1_p, s, 20);
    len = strlen(s);
    s[len] = ',';
    s_p->cfg_p->key_to_string(key2_p, &s[len+1], 20);
    return s;
}

const char* oc_xt_nd_string_of_node(struct Oc_xt_state *s_p,
                                    Oc_xt_node *node_p)
{
    static char s[40];

    if (oc_xt_nd_num_entries(s_p, node_p) > 0) {
        struct Oc_xt_key *max_ofs_p, *min_key_p;
        int len;
        
        max_ofs_p = (struct Oc_xt_key*) alloca(s_p->cfg_p->key_size);
        oc_xt_nd_max_ofs(s_p, node_p, max_ofs_p);
        min_key_p = oc_xt_nd_min_key(s_p, node_p);

        s_p->cfg_p->key_to_string(min_key_p, s, 20);
        len = strlen(s);
        s[len] = ',';
        s_p->cfg_p->key_to_string(max_ofs_p, &s[len+1], 20);
        return s;
    } else
        return "_,_";
}

static inline Oc_xt_nd_hdr* get_hdr(Oc_xt_node *node_p)
{
    return (Oc_xt_nd_hdr*)node_p->data;
}

static inline uint64 get_node_addr(Oc_xt_node *node_p)
{
    return node_p->disk_addr;
}

static void oc_xt_nd_dealloc_node(struct Oc_wu *wu_p,
                                   struct Oc_xt_state *s_p,
                                   Oc_xt_node *node_p)
{
    uint64 addr = node_p->disk_addr;
    oc_xt_nd_release(wu_p, s_p, node_p);
    s_p->cfg_p->node_dealloc(wu_p, addr);
}

static int max_ent_in_hdr (struct Oc_xt_state *s_p, Oc_xt_nd_hdr *hdr_p)
{
    if (hdr_p->flags.root) {
        return s_p->cfg_p->max_num_ent_root_node;
    }
    else {
        if (hdr_p->flags.leaf)
            return s_p->cfg_p->max_num_ent_leaf_node;
        else
            return s_p->cfg_p->max_num_ent_index_node;
    }
}
    
// utility functions
int oc_xt_nd_max_ent_in_node (struct Oc_xt_state *s_p, Oc_xt_node *node_p)
{
    return max_ent_in_hdr(s_p, get_hdr(node_p));
}

static struct Oc_xt_nd_array* get_start_array(struct Oc_xt_state *s_p,
                                               Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p;
    char *p;
    
    hdr_p = get_hdr(node_p);
    
    // 1. move to the start of the array area
    if (hdr_p->flags.root)
        p = (char*)node_p->data + sizeof(Oc_xt_nd_hdr_root);
    else 
        p = (char*)node_p->data + sizeof(Oc_xt_nd_hdr);
    
    return (struct Oc_xt_nd_array*) p;    
}

static bool check_bounds( struct Oc_xt_state *s_p,
                          Oc_xt_nd_hdr *hdr_p,
                          int kth )
{
    return ( num_entries(hdr_p) <= max_ent_in_hdr(s_p, hdr_p) &&
             kth >=0 &&
             kth <= num_entries(hdr_p));
}


static void get_kth_leaf_entry(struct Oc_xt_state *s_p,
                               Oc_xt_nd_hdr *hdr_p,
                               struct Oc_xt_nd_array *arr_p, 
                               struct Nd_leaf_ent_ptrs *ent_ptrs_p,
                               int k)
{
    char *p = (char*) arr_p;

    oc_utl_debugassert(check_bounds(s_p, hdr_p, k));

    // 2. multiply [k] by the size of an entry
    p += (hdr_p->entry_dir[k]) * s_p->cfg_p->leaf_ent_size;

    ent_ptrs_p->key_p = (struct Oc_xt_key*) p;
    ent_ptrs_p->rcrd_p = (struct Oc_xt_rcrd*) (p + s_p->cfg_p->key_size);
}
                               
static void get_kth_index_entry(struct Oc_xt_state *s_p,
                                Oc_xt_nd_hdr *hdr_p,
                                struct Oc_xt_nd_array *arr_p, 
                                struct Nd_index_ent_ptrs *ent_ptrs_p,
                                int k)
{
    char *p = (char*) arr_p;

    oc_utl_debugassert(check_bounds(s_p, hdr_p, k));

    // 2. multiply [k] by the size of an entry
    p += (hdr_p->entry_dir[k]) * s_p->cfg_p->index_ent_size;

    ent_ptrs_p->key_p = (struct Oc_xt_key*) p;
    ent_ptrs_p->addr_p = (uint64*) (p + s_p->cfg_p->key_size);
}
                               

static struct Oc_xt_key* get_kth_key(struct Oc_xt_state *s_p,
                                      Oc_xt_nd_hdr *hdr_p,
                                      struct Oc_xt_nd_array *arr_p, 
                                      int k)
{
    oc_utl_debugassert(hdr_p->num_used_entries > 0);
    oc_utl_debugassert(check_bounds(s_p, hdr_p, k));

    if (hdr_p->flags.leaf) 
        return (struct Oc_xt_key*) ((char*)arr_p +
                                     hdr_p->entry_dir[k] * s_p->cfg_p->leaf_ent_size);
    else
        return (struct Oc_xt_key*) ((char*)arr_p +
                                     hdr_p->entry_dir[k] * s_p->cfg_p->index_ent_size);
}


char *oc_xt_nd_get_root_attrs(Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p = get_hdr(node_p);

    // make -sure- this is a root node
    oc_utl_assert(hdr_p->flags.root);

    return ((Oc_xt_nd_hdr_root*)hdr_p)->attributes;
}

struct Oc_xt_key *oc_xt_nd_get_kth_key(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k)
{
    return get_kth_key(s_p,
                       get_hdr(node_p),
                       get_start_array(s_p, node_p),
                       k);
}


static int generic_comparison(struct Oc_xt_state *s_p,
                              Oc_xt_nd_hdr *hdr_p,
                              struct Oc_xt_nd_array *arr_p,
                              struct Oc_xt_key *key_p,
                              int loc)
{
    struct Oc_xt_key *in_key_p;
    Nd_leaf_ent_ptrs ent;
    
    if (hdr_p->flags.leaf) {
        get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, loc);
        return s_p->cfg_p->rcrd_compare0(key_p, ent.key_p, ent.rcrd_p);
    }
    else {
        in_key_p = get_kth_key(s_p, hdr_p, arr_p, loc);
        return s_p->cfg_p->key_compare(key_p, in_key_p);
    }
}

/*
 * Search for key [key_p] in index node [node_p]. Return -1 if not found.
 * If not found also set the the index in which to insert the new key.
 * This information can be used by an insert routine.
 *
 * return
 *     SRC_HI : if it is higher than all the keys in the node
 *     SRC_LO : if it is smaller than all the keys in the node
 *     SRC_MID: if it somewhere in the range
 *
 * note: this function works for index and leaf nodes. 
 */
static int search_for_key(struct Oc_xt_state *s_p,
                          Oc_xt_node *node_p,
                          struct Oc_xt_key *key_p,
                          int *idx_for_insert_po,
                          Nd_search *src_po)
{
    Oc_xt_nd_hdr *hdr_p;
    int lo_idx, hi_idx, mid_idx, rc;
    struct Oc_xt_nd_array *arr_p;

    arr_p = get_start_array(s_p, node_p);
    hdr_p = get_hdr(node_p);
    
    // special cases
    if (0 == hdr_p->num_used_entries) {
        if (src_po) *src_po = SEARCH_LO;
        if (idx_for_insert_po) *idx_for_insert_po = 0;
        return -1;
    }
    else if (1 == hdr_p->num_used_entries) {
        // node contains one key
        rc = generic_comparison(s_p, hdr_p, arr_p, key_p, 0); 
        switch (rc) {
        case 0:
            if (src_po) *src_po = SEARCH_MID;
            return 0;
        case -1:
            if (src_po) *src_po = SEARCH_HI;
            if (idx_for_insert_po) *idx_for_insert_po = 1;
            return -1;
        case 1:
            if (src_po) *src_po = SEARCH_LO;
            if (idx_for_insert_po) *idx_for_insert_po = 0;
            return -1;
        }
    }
    
    // there are at least two entries in the node, binary search will work
    lo_idx = 0;
    hi_idx = hdr_p->num_used_entries-1;
    
    // check if the key is smaller than the lowest key
    rc = generic_comparison(s_p, hdr_p, arr_p, key_p, 0); 
    switch (rc) {
    case 0:
        if (src_po) *src_po = SEARCH_MID;
        return lo_idx;
    case -1:
        break;
    case 1:
        if (src_po) *src_po = SEARCH_LO;
        if (idx_for_insert_po) *idx_for_insert_po = 0;
        return -1;
    }
    
    // check if the key is larger than the highest key
    rc = generic_comparison(s_p, hdr_p, arr_p, key_p, hi_idx); 
    switch (rc) {
    case 0:
        if (src_po) *src_po = SEARCH_MID;
        return hi_idx;
    case -1:
        if (src_po) *src_po = SEARCH_HI;
        if (idx_for_insert_po) *idx_for_insert_po = hi_idx+1;
        return -1;
    case 1:
        break;
    }
    
    // binary search
    if (src_po) *src_po = SEARCH_MID;
    mid_idx = hdr_p->num_used_entries/2;
    while (1) {
        // termination condition
        if (lo_idx+1 == hi_idx) {
            if (idx_for_insert_po) *idx_for_insert_po = hi_idx;
            return -1;
        }
        
        rc = generic_comparison(s_p, hdr_p, arr_p, key_p, mid_idx);
        switch (rc) {
        case 0:
            if (idx_for_insert_po) *idx_for_insert_po = -1;
            return mid_idx;
            break;
        case -1:
            lo_idx = mid_idx;
            break;
        case 1:
            hi_idx = mid_idx;
            break;
        }
        mid_idx = (lo_idx + hi_idx)/2;
    }    
}

/**********************************************************************/
/* Move all entries between [loc] and [hdr_p->num_used-2] one step up and put
 * the new index at [loc].
 *
 * The assumption here is that the new index is at [hdr_p->num_used-1].
 */
static void shuffle_insert_key(Oc_xt_nd_hdr *hdr_p, int loc)
{
    int i, val;

    oc_utl_debugassert(hdr_p->num_used_entries>=1);
    if (1 == hdr_p->num_used_entries ||
        num_entries(hdr_p)-1 == loc)
        // nothing to do
        return;

//    printf("    //");
//    for (i=0; i < hdr_p->num_used_entries; i++)
//        printf(" (%d::%d)", i, hdr_p->entry_dir[i]);        
//    printf("\n");
    
    val = hdr_p->entry_dir[hdr_p->num_used_entries-1];
    for (i=hdr_p->num_used_entries-2; i>=loc; i--) {
//        printf("    // shuffle %d->%d\n", i, i+1);
        hdr_p->entry_dir[i+1] = hdr_p->entry_dir[i];
    }
    hdr_p->entry_dir[loc] = val;
}


/* free up index [idx]. This is done by moving all allocated entries
 * one step down and moving [idx] to the end.
 */
static void shuffle_remove_key(Oc_xt_nd_hdr *hdr_p, int idx)
{
    int i, val;

    oc_utl_debugassert(hdr_p->num_used_entries>=1);
    val = hdr_p->entry_dir[idx];
    
    for (i=idx+1; i < num_entries(hdr_p); i++) {
//        printf("shuffle %d ", i);
        hdr_p->entry_dir[i-1] = hdr_p->entry_dir[i];
    }
    hdr_p->entry_dir[hdr_p->num_used_entries-1] = val;
    hdr_p->num_used_entries--;
}

// remove all the keys from [idx] and above (including [idx])
static void shuffle_remove_all_above_k(Oc_xt_nd_hdr *hdr_p, int idx)
{
    // the directory is already ordered, there is no need to move indexes
    hdr_p->num_used_entries = idx;
}

// swap between entries [i] and [j]
static void entry_swap(Oc_xt_nd_hdr *hdr_p, int i, int j)
{
    int tmp;
    
    oc_utl_debugassert(num_entries(hdr_p) > i && i>=0);
    oc_utl_debugassert(num_entries(hdr_p) > j && j>=0);
    oc_utl_debugassert(i != j);

    tmp = hdr_p->entry_dir[j];
    hdr_p->entry_dir[j] = hdr_p->entry_dir[i];
    hdr_p->entry_dir[i] = tmp;
}

// remove all the keys from [idx] and below (including [idx])
static void shuffle_remove_all_below_k(Oc_xt_nd_hdr *hdr_p, int idx)
{
    int i;
    
    oc_utl_debugassert(hdr_p->num_used_entries>=2);
    oc_utl_debugassert(idx < num_entries(hdr_p)-1);

    for (i=idx+1; i < num_entries(hdr_p); i++)
        entry_swap(hdr_p, i, i-(idx+1));

    hdr_p->num_used_entries-=(idx+1);
}

/* This function efficiently removes the set of entries between [idx_start] up to
 * and including [idx_end]. The algorithm is to swap between any entry [i] in the
 * range and another valid entry.
 *
 * For example, assume the starting state is
 *     value: 3 4 5 0 2 6 1
 *     index: 0 1 2 3 4 5 6
 * the number of valid entries is 6. This means that entry number 6 is invalid, the
 * rest are valid. 
 * We'd like to remove the entries 2 and 3. The end result is:
 *     value: 3 4 [2 6] [5 0] 1
 *     index: 0 1  2 3   4 5  6
 * we had swapped between the entries at indexes 2,3 and 4,5. The number of valid
 * entries is reduced to 4. 
 */
static void shuffle_remove_key_range(
    Oc_wu *wu_p,
    Oc_xt_nd_hdr *hdr_p,
    int idx_start, int idx_end)
{
    int i, len;
    
    oc_utl_debugassert(hdr_p->num_used_entries >= 1);
    oc_utl_debugassert(idx_end >= idx_start);
    oc_utl_debugassert(idx_end >= 0 && idx_start >= 0);
    oc_utl_debugassert(num_entries(hdr_p) > idx_end);

    oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_RMV_KEY_RNG, wu_p,
                        "idx = [%d,%d]", idx_start, idx_end);
    len = idx_end - idx_start + 1;
        
    for (i=idx_end+1; i < num_entries(hdr_p); i++)
        entry_swap(hdr_p, i, i-len);

    hdr_p->num_used_entries -= len;
}

static void replace_index_entry(struct Oc_xt_state *s_p,
                                Oc_xt_nd_hdr *hdr_p,
                                struct Oc_xt_nd_array *arr_p, 
                                int idx,
                                struct Oc_xt_key *key_p,
                                uint64 *addr_p)
{
    Nd_index_ent_ptrs ent;
    
    oc_utl_debugassert(!hdr_p->flags.leaf);
    get_kth_index_entry(s_p, hdr_p, arr_p, &ent, idx);
    memcpy((char*)ent.key_p, key_p, s_p->cfg_p->key_size);
    memcpy((char*)ent.addr_p, addr_p, sizeof(uint64));
}

static void replace_index_entry_addr(struct Oc_xt_state *s_p,
                                     Oc_xt_nd_hdr *hdr_p,
                                     struct Oc_xt_nd_array *arr_p, 
                                     int idx,
                                     uint64 *old_addr_p,
                                     uint64 *new_addr_p)
{
    Nd_index_ent_ptrs ent;
    
    oc_utl_debugassert(!hdr_p->flags.leaf);
    get_kth_index_entry(s_p, hdr_p, arr_p, &ent, idx);

    // verify correct entry by old addr
    oc_utl_debugassert(
        0 == memcmp((char*)ent.addr_p, old_addr_p, sizeof(uint64)) );

    // replace it with new addr
    memcpy((char*)ent.addr_p, new_addr_p, sizeof(uint64));
}


// find a free index and write the key,data pair into it
static void alloc_new_leaf_entry(struct Oc_wu *wu_p,
                                 struct Oc_xt_state *s_p,
                                 Oc_xt_nd_hdr *hdr_p,
                                 struct Oc_xt_nd_array *arr_p,
                                 struct Oc_xt_key *key_p,
                                 struct Oc_xt_rcrd *rcrd_p)
{
    Nd_leaf_ent_ptrs ent;
    int free_idx;

    oc_utl_debugassert(hdr_p->flags.leaf);
    oc_utl_debugassert(num_entries(hdr_p) < max_ent_in_hdr(s_p, hdr_p));

    // Find space
    free_idx = hdr_p->num_used_entries;
    
    // write into the entry
    get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, free_idx);
    memcpy((char*)ent.key_p, key_p, s_p->cfg_p->key_size);
    memcpy((char*)ent.rcrd_p, rcrd_p, s_p->cfg_p->rcrd_size);

    // update node state
    hdr_p->num_used_entries++;

    oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_NEW_LEAF_ENTRY, wu_p,
                        "#entries=%d  ext=[%s]",
                        hdr_p->num_used_entries,
                        oc_xt_nd_string_of_rcrd(s_p, key_p, rcrd_p));
}

// find a free index and write the key,data pair into it
static void alloc_new_index_entry(struct Oc_xt_state *s_p,
                                 Oc_xt_nd_hdr *hdr_p,
                                 struct Oc_xt_nd_array *arr_p,
                                 struct Oc_xt_key *key_p,
                                 uint64 *addr_p)
{
    Nd_index_ent_ptrs ent;
    int free_idx;

    oc_utl_debugassert(!hdr_p->flags.leaf);
    oc_utl_debugassert(num_entries(hdr_p) < max_ent_in_hdr(s_p, hdr_p));

    // Find space
    free_idx = hdr_p->num_used_entries;
    
    // write into the entry
    get_kth_index_entry(s_p, hdr_p, arr_p, &ent, free_idx);
    memcpy((char*)ent.key_p, key_p, s_p->cfg_p->key_size);
    memcpy((char*)ent.addr_p, addr_p, sizeof(uint64));

    // update node state
    hdr_p->num_used_entries++;
}

static void init_root(
    struct Oc_wu *wu_p,
    Oc_xt_node *node_pi)
{
    Oc_xt_nd_hdr *hdr_p;
    int i;

    hdr_p = get_hdr(node_pi);
    hdr_p->flags.root = TRUE;
    hdr_p->flags.leaf = TRUE;
    hdr_p->num_used_entries = 0;

    for (i=0; i<256; i++)
        hdr_p->entry_dir[i] = i;
}
/**********************************************************************/

void oc_xt_nd_create_root(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_pi)
{
    // erase the page except for the header
    memset(node_pi->data + sizeof(Oc_meta_data_page_hdr),
           0,
           s_p->cfg_p->node_size - sizeof(Oc_meta_data_page_hdr));
    init_root(wu_p, node_pi);
}

/**********************************************************************/

/* return the location of the first key in [node_p] that is greater or equal to
 * [key_p]. If no such key exists return -1.
 */
static int oc_xt_nd_lookup_ge_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p)
{
    int k, insert_loc;
    Nd_search search;
    
    k = search_for_key(s_p, node_p, key_p, &insert_loc, &search);
    if (k != -1) {
        // Exact match 
        return k;
    } else {
        // no exact match, return the higher key
        switch (search) {
        case SEARCH_LO:
            return 0;
        case SEARCH_MID:
            return insert_loc;
        case SEARCH_HI:
            return -1;
        }
        return -1;
    }
}

/* return the location of the first key in [node_p] that is smaller or equal to
 * [key_p]. If no such key exists return -1.
 */
static int oc_xt_nd_lookup_le_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p)
{
    int k, insert_loc;
    Nd_search search;
    
    k = search_for_key(s_p, node_p, key_p, &insert_loc, &search);
    if (k != -1) {
        // key found
        return k;
    } else {
        // no exact match
        switch (search) {
        case SEARCH_LO:
            // [key_p] is smaller than all keys in the node
            return -1;
        case SEARCH_MID:
            oc_utl_debugassert(insert_loc > 0);
            return insert_loc-1;
        case SEARCH_HI:
            // [key_p] is larger than all keys in the node
            oc_utl_debugassert(oc_xt_nd_num_entries(s_p, node_p) > 0);
            return (oc_xt_nd_num_entries(s_p, node_p) -1);
        }
        return -1;
    }
}


// External APIs
int oc_xt_nd_index_lookup_ge_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p)
{
    return oc_xt_nd_lookup_ge_key(wu_p, s_p, node_p, key_p);
}

int oc_xt_nd_index_lookup_le_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p)
{
    return oc_xt_nd_lookup_le_key(wu_p, s_p, node_p, key_p);
}

int oc_xt_nd_leaf_lookup_ge_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p)
{
    return oc_xt_nd_lookup_ge_key(wu_p, s_p, node_p, key_p);
}

int oc_xt_nd_leaf_lookup_le_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p)
{
    return oc_xt_nd_lookup_le_key(wu_p, s_p, node_p, key_p);
}

/**********************************************************************/

// return the node-address which (may) contain the key
uint64 oc_xt_nd_index_lookup_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_key **exact_key_ppo,
    int *kth_po)
{
    int k;
    struct Oc_xt_nd_array *arr_p;
    Oc_xt_nd_hdr *hdr_p = get_hdr(node_p);
    Nd_index_ent_ptrs ent;

    oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_INDEX_LOOKUP, wu_p,
                        "node=[%s] #entries=%d root=%d leaf=%d",
                        oc_xt_nd_string_of_node(s_p, node_p),
                        oc_xt_nd_num_entries(s_p, node_p),
                        oc_xt_nd_is_root(s_p, node_p),
                        oc_xt_nd_is_leaf(s_p, node_p));
    
    k = oc_xt_nd_lookup_le_key(wu_p, s_p, node_p, key_p);
    if (-1 == k) 
        // the key is outside the tree.
        return 0;
    
    arr_p = get_start_array(s_p, node_p);
    get_kth_index_entry(s_p, hdr_p, arr_p, &ent, k);
    if (exact_key_ppo != NULL)
        *exact_key_ppo = ent.key_p;
    if (kth_po != NULL)
        *kth_po = k;
    return (*ent.addr_p);
}

uint64 oc_xt_nd_index_lookup_min_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Nd_index_ent_ptrs ent;
    struct Oc_xt_nd_array *arr_p;
    Oc_xt_nd_hdr *hdr_p = get_hdr(node_p);
    
    arr_p = get_start_array(s_p, node_p);
    get_kth_index_entry(s_p, hdr_p, arr_p, &ent, 0);
    return (*ent.addr_p);
}

/**********************************************************************/
// make sure a node has a minimal number of entries prior to the removal
// of a key
#if 0
static bool pre_remove_verify_num_entries(struct Oc_xt_state *s_p,
                                          Oc_xt_node *node_p)
{
    if (oc_xt_nd_is_root(s_p, node_p))
        return TRUE;
    return (oc_xt_nd_num_entries(s_p, node_p) > s_p->cfg_p->min_num_ent);
}
#endif

/**********************************************************************/
// delete a node
void oc_xt_nd_delete(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p;
    struct Oc_xt_nd_array *arr_p;
    uint64 addr;
    
    hdr_p = get_hdr(node_p);
    arr_p = get_start_array(s_p, node_p);

    if (hdr_p->flags.leaf) {
        int i;
        Nd_leaf_ent_ptrs ent;
        
        for (i=0; i<num_entries(hdr_p); i++)
        {
            get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, i);
            s_p->cfg_p->rcrd_release(wu_p, ent.key_p, ent.rcrd_p);
        }
    }
    
    addr = node_p->disk_addr;
    s_p->cfg_p->node_release(wu_p, node_p);
    s_p->cfg_p->node_dealloc(wu_p, addr);
}

void oc_xt_nd_delete_locked(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    oc_utl_trk_crt_unlock(wu_p, &node_p->lock);
    oc_xt_nd_delete(wu_p, s_p, node_p);
}

/**********************************************************************/
// print a node
void oc_xt_nd_print(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p;
    struct Oc_xt_nd_array *arr_p;
    int i;
    Nd_leaf_ent_ptrs ent;
    char s[30];
    
    hdr_p = get_hdr(node_p);
    arr_p = get_start_array(s_p, node_p);
    
    for (i=0; i<num_entries(hdr_p); i++)
    {
        get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, i);

        s_p->cfg_p->rcrd_to_string(ent.key_p, ent.rcrd_p, s, 30);
    
        printf("(ext=%s) ", s);
    }
}

/**********************************************************************/

// is this node full? 
bool oc_xt_nd_is_full(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    
    return (num_entries(hdr_p) == oc_xt_nd_max_ent_in_node (s_p, node_p));
}

bool oc_xt_nd_is_full_for_insert(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);

    if (!hdr_p->flags.leaf)
        return (num_entries(hdr_p) == oc_xt_nd_max_ent_in_node (s_p, node_p));
    else
        return (num_entries(hdr_p) >
                (oc_xt_nd_max_ent_in_node (s_p, node_p) - 2));
}

bool oc_xt_nd_is_leaf(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    return hdr_p->flags.leaf;
}

void oc_xt_nd_set_leaf(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    hdr_p->flags.leaf = TRUE;
}

bool oc_xt_nd_is_root(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    return hdr_p->flags.root;
}
    
// return the number of entries in [node_p]
int oc_xt_nd_num_entries(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    return hdr_p->num_used_entries;
}

// [node_p] is leaf node. Get its kth entry
void oc_xt_nd_leaf_get_kth(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k,
    struct Oc_xt_key **key_ppo,
    struct Oc_xt_rcrd **rcrd_ppo)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p); 
    Nd_leaf_ent_ptrs ent;

    oc_utl_assert(k < num_entries(hdr_p));
    oc_utl_debugassert(hdr_p->flags.leaf);

    get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, k);
    *key_ppo = ent.key_p;
    *rcrd_ppo = ent.rcrd_p;
}

// [node_p] is an index node. Get its kth pointer.
void oc_xt_nd_index_get_kth(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k,
    struct Oc_xt_key **key_ppo,
    uint64 *addr_po)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p); 
    Nd_index_ent_ptrs ent;

    oc_utl_assert(k < num_entries(hdr_p));
    oc_utl_debugassert(!hdr_p->flags.leaf);

    get_kth_index_entry(s_p, hdr_p, arr_p, &ent, k);
    *key_ppo = ent.key_p;
    *addr_po = *ent.addr_p;
}

// [node_p] is an index node. Set its kth pointer. 
void oc_xt_nd_index_set_kth(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k,
    struct Oc_xt_key *key_p,
    uint64 addr)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p); 
    Nd_index_ent_ptrs ent;

    oc_utl_assert(k < num_entries(hdr_p));
    oc_utl_debugassert(!hdr_p->flags.leaf);

    get_kth_index_entry(s_p, hdr_p, arr_p, &ent, k);
    memcpy((char*)ent.key_p, (char*)key_p, s_p->cfg_p->key_size);
    *(ent.addr_p) = addr;
}

// Remove the kth entry from [node_p]
void oc_xt_nd_remove_kth(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int k)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    shuffle_remove_key(hdr_p, k);
}

/**********************************************************************/
// return the smallest key in [node_p]
struct Oc_xt_key* oc_xt_nd_min_key(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p);
    
    return get_kth_key(s_p, hdr_p, arr_p, 0);
}

struct Oc_xt_key* oc_xt_nd_max_key(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p);
    
    return get_kth_key(s_p, hdr_p, arr_p, hdr_p->num_used_entries-1);
}

// return the largest key in [node_p]
void oc_xt_nd_max_ofs(
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *max_ofs_po)
{
    Nd_leaf_ent_ptrs ent;
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p);

    if (hdr_p->flags.leaf) {
        get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, hdr_p->num_used_entries-1);
        s_p->cfg_p->rcrd_end_offset(ent.key_p, ent.rcrd_p, max_ofs_po);
    }
    else {
        memcpy((char*)max_ofs_po,
               (char*)get_kth_key(s_p, hdr_p, arr_p, hdr_p->num_used_entries-1),
               s_p->cfg_p->key_size);
    }
}

/**********************************************************************/
/* in an index node [index_node_p] replace binding for index [k]
 * with two bindings for two nodes [left_node_p] and [right_node_p].
 */
void oc_xt_nd_index_replace_w2(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *index_node_p,
    int k, 
    Oc_xt_node *left_node_p,
    Oc_xt_node *right_node_p)
{
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, index_node_p);
    Oc_xt_nd_hdr *hdr_p = get_hdr(index_node_p);
    uint64 left_addr, right_addr;

    oc_utl_debugassert(!hdr_p->flags.leaf);

    // The caller guaranties that the key is in the range of the index-node
    oc_utl_debugassert(k >= 0 && k <= num_entries(hdr_p));
    
    // replace the original (key,addr) with the left-node (key,addr)
    left_addr = left_node_p->disk_addr;
    replace_index_entry(s_p, hdr_p, arr_p, k,
                        oc_xt_nd_min_key(s_p, left_node_p),
                        &left_addr);

    // add an additional binding for the (key,addr) of the right-node
    right_addr = right_node_p->disk_addr;
    alloc_new_index_entry(s_p, hdr_p, arr_p,
                          oc_xt_nd_min_key(s_p, right_node_p),
                          &right_addr);
    shuffle_insert_key(hdr_p, k+1);
}

/**********************************************************************/
// replace the minimum key with a different (smaller) key
void oc_xt_nd_index_replace_min_key(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *index_node_p,
    struct Oc_xt_key *key_p)
{
    Nd_index_ent_ptrs ent;
    Oc_xt_nd_hdr *hdr_p = get_hdr(index_node_p);
    struct Oc_xt_nd_array *arr_p;
    
    oc_utl_debugassert(!hdr_p->flags.leaf);
    oc_utl_debugassert(hdr_p->num_used_entries >= 1);
    
    arr_p = get_start_array(s_p, index_node_p);
    get_kth_index_entry(s_p, hdr_p, arr_p, &ent, 0);

    memcpy((char*)ent.key_p, key_p, s_p->cfg_p->key_size);
}

/**********************************************************************/
// split a non-root node into two. The node can be a leaf or an index node.
Oc_xt_node *oc_xt_nd_split(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p = get_hdr(node_p);
    Oc_xt_node *right_p;
    int k;
    
    oc_utl_debugassert(!hdr_p->flags.root);
    
    // 1. make a copy of [node_p], mark the two copies by L and R
    right_p = s_p->cfg_p->node_alloc(wu_p);
    oc_utl_trk_crt_lock_write(wu_p, &right_p->lock);
    s_p->cfg_p->node_mark_dirty(wu_p, right_p);
    memcpy(right_p->data, node_p->data, s_p->cfg_p->node_size);
    
    // 2. decide on a division between L, and R: choose a number k
    k = hdr_p->num_used_entries /2;
    oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_SPLIT, wu_p,
                        "%s [%s] splitting %d into %d+%d",
                        ((hdr_p->flags.leaf) ? "leaf" : "index"),
                        oc_xt_nd_string_of_node(s_p, node_p),
                        hdr_p->num_used_entries, k, hdr_p->num_used_entries - k);
        
    // 3. remove from L all entries above k
    shuffle_remove_all_above_k(hdr_p, k);    
    
    // 4. remove from R all entries below k (including the k'th entry)
    shuffle_remove_all_below_k(get_hdr(right_p), k-1);
    
    oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_SPLIT, wu_p,
                        "left_node=[%s]",
                        oc_xt_nd_string_of_node(s_p, node_p));
    oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_SPLIT, wu_p,
                        "right_node=[%s]",
                        oc_xt_nd_string_of_node(s_p, right_p));
    return right_p;
}

/**********************************************************************/
/* split a root node into two and create a new root with pointers to
 * the two childern. The root cannot move during this operation. 
 *
 * There are two cases:
 *   1. the root is also a leaf node
 *     - create two (non-root) leaf nodes and spread the keys
 *       between them.
 *   2. the root is also an index node
 *     - create two (non-root) index nodes and spread the keys
 *       between them.
 *
 * in order to simplify coding we make use of the [oc_xt_nd_split]
 * function.
 *  1. copy the root node into a regular, non-root node. Mark this node by N.
 *  2. split L into R using [oc_xt_nd_split].
 *  3. Erase the original root node.
 *  4. Convert it into an index node.
 *  5. Add two entries to the root: one for the right node and one for the left node.
 */
void oc_xt_nd_split_root(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *root_node_p )
{
    Oc_xt_nd_hdr *hdr_p = get_hdr(root_node_p);
    Oc_xt_nd_hdr *lt_hdr_p;
    Oc_xt_node *left_p, *right_p;
    struct Oc_xt_nd_array *arr_p, *lt_arr_p;
    uint64 left_addr, right_addr;
    
    oc_utl_debugassert(hdr_p->flags.root);
    oc_xt_trace_wu_lvl(3, OC_EV_XT_ROOT_SPLIT, wu_p, "");

    //  1. copy the root node into a regular, non-root node. Mark this node by N.
    left_p = s_p->cfg_p->node_alloc(wu_p);
    oc_utl_trk_crt_lock_write(wu_p, &left_p->lock);
    s_p->cfg_p->node_mark_dirty(wu_p, left_p);
    
    memcpy(left_p->data, root_node_p->data, sizeof(Oc_xt_nd_hdr));
    lt_hdr_p = get_hdr(left_p);
    lt_hdr_p->flags.root = FALSE;
    arr_p = get_start_array(s_p, root_node_p);
    lt_arr_p = get_start_array(s_p, left_p);
    memcpy((char*)lt_arr_p,
           (char*)arr_p,
           s_p->cfg_p->node_size - sizeof(Oc_xt_nd_hdr_root));
    
    //  2. split L into R using [oc_xt_nd_split].
    right_p = oc_xt_nd_split(wu_p, s_p, left_p);

    //  3. Erase the entries from the original root node.
    hdr_p->num_used_entries = 0;
    
    //  4. Convert it into an index node.
    hdr_p->flags.leaf = FALSE;
    
    //  5. Add two entries to the root: one for the right node and one for the left node.
    left_addr = left_p->disk_addr;
    alloc_new_index_entry(s_p,
                          hdr_p,
                          arr_p,
                          oc_xt_nd_min_key(s_p, left_p),
                          &left_addr);
    right_addr = right_p->disk_addr;
    alloc_new_index_entry(s_p,
                          hdr_p,
                          arr_p,
                          oc_xt_nd_min_key(s_p, right_p),
                          &right_addr);

    oc_xt_nd_release(wu_p, s_p, left_p);
    oc_xt_nd_release(wu_p, s_p, right_p);
}

/**********************************************************************/
// used for remove-key

/* compute if the keys in [node1_p] are higher or lower than [node2_p].
 * return:
 *   TRUE -  node1_p < node2_p
 *   FALSE - node1_p > node2_p
 */
static bool node_compare(struct Oc_xt_state *s_p,
                         Oc_xt_node *node1_p,
                         Oc_xt_node *node2_p)
{
    struct Oc_xt_key *key1_p, *key2_p;
    
    key1_p = oc_xt_nd_max_key(s_p, node1_p);
    key2_p = oc_xt_nd_max_key(s_p, node2_p);
    
    switch (s_p->cfg_p->key_compare(key1_p, key2_p)) {
    case 0:
        ERR(("sanity"));
    case 1:
        return TRUE;
        break;
    case -1:
        return FALSE;
        break;        
    }

    // we should not reach here
    return TRUE;
}

/* copy [n] entries starting at [start_idx] from node [src_p] to node [trg_p].
 */
static void copy_n_entries(
    struct Oc_wu *wu_p,    
    struct Oc_xt_state *s_p,    
    Oc_xt_node *trg_p,
    Oc_xt_node *src_p,
    int start_idx,
    int num)
{
    Oc_xt_nd_hdr *trg_hdr_p = get_hdr(trg_p);
    Oc_xt_nd_hdr *src_hdr_p = get_hdr(src_p);
    int i,k;
    struct Oc_xt_nd_array *src_arr_p, *trg_arr_p;
    bool lo;

    oc_utl_debugassert(src_hdr_p->flags.leaf == trg_hdr_p->flags.leaf);
    oc_utl_debugassert(start_idx >= 0);
    oc_utl_debugassert(num > 0);
    oc_utl_debugassert(start_idx + num <= num_entries(src_hdr_p));
    oc_utl_debugassert(num_entries(trg_hdr_p) + num
                       <= oc_xt_nd_max_ent_in_node(s_p, trg_p));
                       
    if (trg_hdr_p->num_used_entries > 0) 
        lo = node_compare(s_p, src_p, trg_p);
    else
        lo = FALSE;
    
    src_arr_p = get_start_array(s_p, src_p);
    trg_arr_p = get_start_array(s_p, trg_p);

    // copy entries from [src_p] into [trg_p] 
    for (i=0; i < num; i++) {
        k = i + start_idx;
        if (src_hdr_p->flags.leaf) {
            Nd_leaf_ent_ptrs leaf_ent;
            
            get_kth_leaf_entry(s_p, src_hdr_p, src_arr_p, &leaf_ent, k);
            alloc_new_leaf_entry(wu_p, s_p, trg_hdr_p, trg_arr_p,
                                 leaf_ent.key_p,
                                 leaf_ent.rcrd_p);
        }
        else {
            Nd_index_ent_ptrs index_ent;
            
            get_kth_index_entry(s_p, src_hdr_p, src_arr_p, &index_ent, k);
            alloc_new_index_entry(s_p, trg_hdr_p, trg_arr_p,
                                  index_ent.key_p,
                                  index_ent.addr_p);
        }

        /* move the entry to its correct place.
         * If [src_p] < [trg_p] then all the keys copied from [src_p]
         * are small and should be inserted at the beginning of [trg_p].
         * the alloc_new_* primitives allocate the copied entry at the end
         * of [src_p]. This code corrects this problem.
         */
        if (lo)
            shuffle_insert_key(trg_hdr_p, i);        
    }
}

/* copy all entries from [node_p] into [root_node_p].
 * After copying [node_p] is deallocated. 
 */
void oc_xt_nd_copy_into_root_and_dealloc(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *root_node_p,  
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *root_hdr_p = get_hdr(root_node_p);
    Oc_xt_nd_hdr *node_hdr_p = get_hdr(node_p);
    
    oc_utl_assert(root_hdr_p->flags.root);
    oc_utl_debugassert(!node_hdr_p->flags.root);
    oc_utl_assert(oc_xt_nd_max_ent_in_node(s_p, root_node_p) >=
                  num_entries(node_hdr_p));

    oc_xt_trace_wu_lvl(3, OC_EV_XT_COPY_INTO_ROOT, wu_p,
                        "#entries=%d leaf=%d",
                        node_hdr_p->num_used_entries,
                        node_hdr_p->flags.leaf);

    // erase the root node
    init_root(wu_p, root_node_p);
    root_hdr_p->flags.leaf = node_hdr_p->flags.leaf;
        
    // copy the entries
    copy_n_entries(wu_p, s_p, root_node_p, node_p, 0, node_hdr_p->num_used_entries);
        
    // deallocate [node_p]
    oc_xt_nd_dealloc_node(wu_p, s_p, node_p);
}

/* move entries from [node_p] into node [under_p] which has
 * an underflow. 
 */
static void rebalance(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *under_p,  
    Oc_xt_node *node_p,
    bool skewed)
{
    Oc_xt_nd_hdr *under_hdr_p = get_hdr(under_p);
    Oc_xt_nd_hdr *node_hdr_p = get_hdr(node_p);
    int k;
    bool hi;
    
    oc_utl_debugassert(!node_hdr_p->flags.root);
    oc_utl_debugassert(!under_hdr_p->flags.root);
    oc_utl_debugassert(num_entries(node_hdr_p) > s_p->cfg_p->min_num_ent);
    oc_utl_debugassert(!num_entries(under_hdr_p) < s_p->cfg_p->min_num_ent);
    oc_utl_debugassert(num_entries(node_hdr_p) +
                       num_entries(under_hdr_p) >= 2 * s_p->cfg_p->min_num_ent);
    if (skewed)
        oc_utl_debugassert(num_entries(node_hdr_p) +
                           num_entries(under_hdr_p) >=
                           2 + 2 * s_p->cfg_p->min_num_ent);
        
    hi = node_compare(s_p, under_p, node_p);

    // compute how many entries to move
    k = (1 + node_hdr_p->num_used_entries - s_p->cfg_p->min_num_ent) /2;
    oc_utl_debugassert(num_entries(node_hdr_p) - k >= s_p->cfg_p->min_num_ent);
    if (skewed) {
        // make sure the node with the underflow remains with at least b+2 entries
        while (num_entries(under_hdr_p) + k < s_p->cfg_p->min_num_ent + 2)
            k++;
    }

    oc_xt_trace_wu_lvl(2, OC_EV_XT_REBALANCE, wu_p,
                        "%s #moved_keys=%d src=[%s]",
                        ((hi) ? "from right" : "from left"),
                        k,
                        oc_xt_nd_string_of_node(s_p, node_p));
    oc_xt_trace_wu_lvl(2, OC_EV_XT_REBALANCE2, wu_p,
                        "trg=[%s]", oc_xt_nd_string_of_node(s_p, under_p));

    if (hi) {
        // [node_p] > [under_p]
        copy_n_entries(wu_p, s_p, under_p, node_p, 0, k);
        shuffle_remove_all_below_k(node_hdr_p, k-1);
    }
    else {
        // [node_p] < [under_p]
        copy_n_entries(wu_p, s_p, under_p, node_p,
                       node_hdr_p->num_used_entries-k,
                       k);
        shuffle_remove_all_above_k(node_hdr_p, node_hdr_p->num_used_entries-k);
    }

    // verify that in the end both nodes are legal
    oc_utl_debugassert(num_entries(node_hdr_p) >= s_p->cfg_p->min_num_ent);
    oc_utl_debugassert(num_entries(under_hdr_p) >= s_p->cfg_p->min_num_ent);
}
    
void oc_xt_nd_rebalance_skewed(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    Oc_xt_node *under_p,  
    Oc_xt_node *node_p)
{
    rebalance(wu_p, s_p, under_p, node_p, TRUE);    
}

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
    Oc_xt_node *src_p)
{
    Oc_xt_nd_hdr *trg_hdr_p = get_hdr(trg_p);
    Oc_xt_nd_hdr *src_hdr_p = get_hdr(src_p);
    
    oc_utl_debugassert(!src_hdr_p->flags.root);
    oc_utl_debugassert(!trg_hdr_p->flags.root);
    oc_utl_debugassert(num_entries(trg_hdr_p) +
                       num_entries(src_hdr_p) <=
                       oc_xt_nd_max_ent_in_node (s_p, src_p));
        
    oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_MERGE_AND_DEALLOC, wu_p,
                        "copying node (%d entries) into node with (%d entries)",
                        src_hdr_p->num_used_entries, trg_hdr_p->num_used_entries);

    /* make sure both nodes are of the same type:
     *  either both are leaves or index-nodes
     */
    oc_utl_debugassert(src_hdr_p->flags.leaf == trg_hdr_p->flags.leaf);

    // copy all the entries from [src_p] into [trg_p]
    copy_n_entries(wu_p, s_p, trg_p, src_p, 0, src_hdr_p->num_used_entries);

    // deallocate [src_p]
    oc_xt_nd_dealloc_node(wu_p, s_p, src_p);    
}

/**********************************************************************/
/* Remove the beginning or the end of extent E located at [k]. The range to remove
 * is between [min_key_p] and [max_key_p]. E is split into
 * E1,E2,E3. Remove E and add {E1, E3} instead. It is possible that E1 or E3
 * are empty. 
 *
 * return the length removed.
 *
 * if [one_subextent_at_most] is TRUE then the maximal number of sub-extents
 * remaining can be one at the most. 
 */
static uint64 remove_part(
    Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    Oc_xt_nd_hdr *hdr_p,
    struct Oc_xt_nd_array *arr_p,
    int k,
    bool one_subextent_at_most,
    Oc_xt_nd_spill *spill_p)
{
    struct Oc_xt_key *key_array_p[3], *res_key_array;
    struct Oc_xt_rcrd *rcrd_array_p[3], *res_rcrd_array;
    Oc_xt_cmp rc;
    uint64 len;
    int cnt=0, i;
    Nd_leaf_ent_ptrs ent;
    
    get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, k);        
    oc_xt_trace_wu_lvl(3, OC_EV_XT_REMOVE_PART_1, wu_p,
                       "range=[%s] ext=[%s]",
                       oc_xt_nd_string_of_2key(s_p, min_key_p, max_key_p),
                       oc_xt_nd_string_of_rcrd(s_p, ent.key_p, ent.rcrd_p));

    // setup location to receive sub-extents
    res_key_array = (struct Oc_xt_key *)alloca(s_p->cfg_p->key_size * 3);
    res_rcrd_array = (struct Oc_xt_rcrd *)alloca(s_p->cfg_p->rcrd_size * 3);
    for (i=0; i<3; i++) {
        key_array_p[i] = oc_xt_nd_key_array_kth(s_p, res_key_array, i);
        rcrd_array_p[i] = oc_xt_nd_rcrd_array_kth(s_p, res_rcrd_array, i);
    }
    
    rc = s_p->cfg_p->rcrd_bound_split(
        // record+key to chop
        ent.key_p, ent.rcrd_p,
        
        // boundary keys
        min_key_p, max_key_p,
        
        // resulting sub-extents
        key_array_p, rcrd_array_p);
    
    len = s_p->cfg_p->rcrd_length(key_array_p[1], rcrd_array_p[1]);
    s_p->cfg_p->rcrd_release(wu_p, key_array_p[1], rcrd_array_p[1]);

    if (NULL != key_array_p[0]) cnt++;
    if (NULL != key_array_p[2]) cnt++;
    
    if (0 == cnt) {
        // The whole extent at [k] was removed
        shuffle_remove_key(hdr_p, k);
    }
    else if (1 == cnt) {
        /* one sub-extent is left.
         * overwrite the record at [k] with the sub-extent
         */
        if (NULL != key_array_p[0])
            copy_rcrd(s_p, ent.key_p, ent.rcrd_p,
                      key_array_p[0], rcrd_array_p[0]);
        else
            copy_rcrd(s_p, ent.key_p, ent.rcrd_p,
                      key_array_p[2], rcrd_array_p[2]);
    }
    else {
        /* one sub-extent is left.
         * overwrite the record at [k] with the sub-extent E1 and
         * insert E3 at location [k+1].
         */
        oc_utl_assert(!one_subextent_at_most);
        
        copy_rcrd(s_p, ent.key_p, ent.rcrd_p,
                  key_array_p[0], rcrd_array_p[0]);
        
        if (num_entries(hdr_p) < max_ent_in_hdr(s_p, hdr_p))
        {
            // There is room in this node to insert E3
            alloc_new_leaf_entry(wu_p, s_p, hdr_p, arr_p,
                                 key_array_p[2], rcrd_array_p[2]);
            shuffle_insert_key(hdr_p, k+1);
        }
        else {
            /* There is no room. This case can only happen if
             * the caller provides a spill-over option
             */
            oc_utl_assert(spill_p);
            oc_utl_assert(!spill_p->flag);
            
            spill_p->flag = TRUE;
            copy_rcrd(s_p,
                      spill_p->key_p, spill_p->rcrd_p,
                      key_array_p[2], rcrd_array_p[2]);
        }
    }

    oc_xt_trace_wu_lvl(3, OC_EV_XT_REMOVE_PART_2, wu_p,
                       "len=%lu", len);
    return len;
}

/* remove the range of keys from leaf node [node_p]
 */
uint64 oc_xt_nd_leaf_remove_range(
    Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    Oc_xt_nd_spill *spill_p)
{
    uint64 rc=0;
    int i;
    Nd_leaf_ent_ptrs ent;
    int min_loc, max_loc;
    Oc_xt_nd_hdr *hdr_p = get_hdr(node_p);
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p);   

    oc_utl_debugassert(oc_xt_nd_is_leaf(s_p, node_p));
    if (spill_p)
        oc_utl_assert(!spill_p->flag);
            
    // compute minimum and maximum indexes to remove
    min_loc = oc_xt_nd_lookup_ge_key(wu_p, s_p, node_p, min_key_p);
    max_loc = oc_xt_nd_lookup_le_key(wu_p, s_p, node_p, max_key_p);
    if (-1 == min_loc ||
        -1 == max_loc ||
        min_loc > max_loc)
        // there are no keys in the range
        return 0;
    
    oc_xt_trace_wu_lvl(3, OC_EV_XT_LEAF_REMOVE_RNG, wu_p,
                       "range=[%s] leaf-node entries = [min=%d,max=%d]",
                       oc_xt_nd_string_of_2key(s_p, min_key_p, max_key_p),
                       min_loc, max_loc);
    
    if (min_loc == max_loc) {
        /* 1. The range is encapsulated by a single extent E; this means
         *    that E is split into E1,E2,E3. Remove E and add E1, E3.
         */
        rc = remove_part(wu_p, s_p,
                         min_key_p, max_key_p,
                         hdr_p, arr_p,
                         min_loc, FALSE, spill_p);
        goto done;
    }
    
    /* 2. Handle the highest-extent. it can have a partial intersection
     *    with the range [min_key, max_key].
     *
     * note: we are going in decending order from high to low because
     *   removing a low extent shuffles the higher extents and
     *   invalidates the location computation.
     */
    rc += remove_part(wu_p, s_p,
                      min_key_p, max_key_p,
                      hdr_p, arr_p,
                      max_loc, TRUE, NULL);
    
    /* 3. Handle the middle extents which are all completely covered. 
     */
    // Release the data, sum up the areas released.
    if (min_loc+1 <= max_loc-1) {
        for (i=min_loc+1; i<= max_loc-1; i++) {
            get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, i);
            rc += s_p->cfg_p->rcrd_length(ent.key_p, ent.rcrd_p);
            s_p->cfg_p->rcrd_release(wu_p, ent.key_p, ent.rcrd_p);
        }
        
        // remove the entries from the node
        shuffle_remove_key_range(wu_p, hdr_p, min_loc+1, max_loc-1);
    }

    /* 4. Handle the lowest-extent. it can have a partial intersection
     *    with the range [min_key, max_key].
     */
    rc += remove_part(wu_p, s_p,
                      min_key_p, max_key_p,
                      hdr_p, arr_p,
                      min_loc, TRUE, NULL);
 done:
    oc_xt_trace_wu_lvl(3, OC_EV_XT_LEAF_REMOVE_RNG_2, wu_p,
                       "remove-length=%lu", rc);
    return rc;
}

uint64 oc_xt_nd_leaf_length(
    Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    int i;
    int n_entries = num_entries(hdr_p);
    Nd_leaf_ent_ptrs ent;
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p);
    uint64 rc = 0;
    
    for (i=0; i < n_entries; i++) {
        get_kth_leaf_entry(s_p, hdr_p, arr_p, &ent, i);
        rc += s_p->cfg_p->rcrd_length(ent.key_p, ent.rcrd_p);
    }

    return rc;
}
    
void oc_xt_nd_remove_range_of_entries(
    Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    int idx_start,
    int idx_end)
{
    Oc_xt_nd_hdr *hdr_p= get_hdr(node_p);
    
    oc_utl_debugassert(hdr_p->num_used_entries>=1);
    oc_utl_debugassert(idx_end >= idx_start);
    oc_utl_debugassert(idx_end >= 0 && idx_start >= 0);
    oc_utl_debugassert(num_entries(hdr_p) > idx_end);
    
    // remove the entries from the node
    shuffle_remove_key_range(wu_p, hdr_p, idx_start, idx_end);
}

/**********************************************************************/
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
    struct Oc_xt_rcrd *rcrd_p)
{
    Oc_xt_nd_hdr *hdr_p = get_hdr(node_p);
    struct Oc_xt_nd_array *arr_p = get_start_array(s_p, node_p);
    int loc, insert_loc, rc=0;
    struct Oc_xt_key *tmp_key_p;
    
    oc_utl_debugassert(oc_xt_nd_is_leaf(s_p, node_p));
    oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_INSERT_INTO_LEAF, wu_p,
                       "[%s] ext=%s  used_entries=%d",
                       oc_xt_nd_string_of_node(s_p, node_p),
                       oc_xt_nd_string_of_rcrd(s_p, key_p, rcrd_p),
                       hdr_p->num_used_entries);
    
    /* 1. remove from the node all entries that overlap with the
     *    new area. 
     * 2. insert as many new extents as can fit. 
     *
     * Corner case:
     *    As a result of operation (1) the node might underflow.
     *    This is taken care of by splitting the new extent into
     *    enough sub-extents as to prevent underflow. 
     */

    // 1. the node is completely empty
    if (0 == hdr_p->num_used_entries) {
        alloc_new_leaf_entry(wu_p, s_p, hdr_p,
                             arr_p,
                             key_p, rcrd_p);
        goto done;
    }

    // temporary key, used for computing end-keys
    tmp_key_p = (struct Oc_xt_key*)alloca(s_p->cfg_p->key_size);
    
    // 2. The extent is larger then all the keys in the node.
    //    Allocate the extent at the end of the node.
    oc_xt_nd_max_ofs(s_p, node_p, tmp_key_p);
    if (s_p->cfg_p->key_compare(key_p, tmp_key_p) == -1)
    {
        alloc_new_leaf_entry(wu_p, s_p, hdr_p,
                             arr_p,
                             key_p, rcrd_p);
        goto done;
    }
    
    // 3. Normal case, the new keys are inside the range 
    
    // remove all entries that overlap with the inserted extent
    s_p->cfg_p->rcrd_end_offset(key_p, rcrd_p, tmp_key_p);
    rc = oc_xt_nd_leaf_remove_range(wu_p, s_p, node_p, key_p, tmp_key_p, NULL);

    // insert the record into a new location in the node
    oc_utl_debugassert(num_entries(hdr_p) < oc_xt_nd_max_ent_in_node(s_p, node_p));

    if (hdr_p->flags.root ||
        num_entries(hdr_p) >= s_p->cfg_p->min_num_ent - 1)
    {
        // We haven't created underflow
        loc = search_for_key(s_p, node_p, key_p, &insert_loc, NULL);    
        alloc_new_leaf_entry(wu_p, s_p, hdr_p, arr_p, key_p, rcrd_p);
        shuffle_insert_key(hdr_p, insert_loc);
    }
    else {
        // The remove-range operation has created underflow. We need
        // to fix this.
        int i;
        int num_ext_to_crt = s_p->cfg_p->min_num_ent - hdr_p->num_used_entries;
        struct Oc_xt_key* key_array =
            (struct Oc_xt_key*) alloca(num_ext_to_crt * s_p->cfg_p->key_size);
        struct Oc_xt_rcrd* rcrd_array =
            (struct Oc_xt_rcrd*) alloca(num_ext_to_crt * s_p->cfg_p->rcrd_size);

        oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_INSERT_INTO_LEAF_UNDERFLOW_FIX, wu_p,
                           "num extents to add=%d", num_ext_to_crt);
                           
        s_p->cfg_p->rcrd_split_into_sub(key_p, rcrd_p,
                                        num_ext_to_crt,
                                        key_array, rcrd_array);

        /* We need to insert entries in decending order so that they end
         * up in ascending order in the leaf.
        */
        for (i = num_ext_to_crt-1; i >= 0; i--)
        {
            loc = search_for_key(s_p, node_p, key_p, &insert_loc, NULL);    
            alloc_new_leaf_entry(wu_p, s_p, hdr_p, arr_p,
                                 oc_xt_nd_key_array_kth(s_p, key_array, i),
                                 oc_xt_nd_rcrd_array_kth(s_p, rcrd_array, i));
            shuffle_insert_key(hdr_p, insert_loc);
        }
    }

 done:
    oc_xt_trace_wu_lvl(
        3, OC_EV_XT_ND_INSERT_INTO_LEAF_DONE, wu_p,
        "length overwritten=%d", rc);
    return rc;
}

/**********************************************************************/
// used in remove-range

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
    Oc_xt_node *src_p)
{
    Oc_xt_nd_hdr *src_hdr_p = get_hdr(src_p);
    Oc_xt_nd_hdr *trg_hdr_p = get_hdr(trg_p);

    oc_utl_debugassert(src_hdr_p->flags.leaf == trg_hdr_p->flags.leaf);
    oc_utl_debugassert(!src_hdr_p->flags.root);
    oc_utl_debugassert(!trg_hdr_p->flags.root);
    
    if (src_hdr_p->flags.leaf) {
        struct Oc_xt_key *key_p;
        struct Oc_xt_rcrd *rcrd_p;
        
        // This is a leaf node
        // 1. record the largest entry (E) in [src_p]
        oc_xt_nd_leaf_get_kth(s_p, src_p,
                              src_hdr_p->num_used_entries - 1,
                              &key_p, &rcrd_p);

        oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_MOVE_MAX_KEY, wu_p,
                            "key=%s", oc_xt_nd_string_of_key(s_p, key_p));
            
        // 2. add E as the minimum in [trg_p]
        alloc_new_leaf_entry(wu_p, s_p, trg_hdr_p,
                             get_start_array(s_p, trg_p),
                             key_p, rcrd_p);
        shuffle_insert_key(trg_hdr_p, 0);
        
        // 3. remove E from [src_p]
        shuffle_remove_key(src_hdr_p, src_hdr_p->num_used_entries - 1);
    }
    else {
        struct Oc_xt_key *key_p;
        uint64 addr;
        
        // This is an index node
        // 1. record the largest entry (E) in [src_p]
        oc_xt_nd_index_get_kth(s_p, src_p,
                                src_hdr_p->num_used_entries - 1,
                                &key_p, &addr);
        
        oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_MOVE_MAX_KEY, wu_p,
                            "key=%s", oc_xt_nd_string_of_key(s_p, key_p));

        // 2. add E as the minimum in [trg_p]
        alloc_new_index_entry(s_p, trg_hdr_p,
                              get_start_array(s_p, trg_p),
                              key_p, &addr);
        shuffle_insert_key(trg_hdr_p, 0);        
        
        // 3. remove E from [src_p]
        shuffle_remove_key(src_hdr_p, src_hdr_p->num_used_entries - 1);
    }
}


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
    Oc_xt_node *src_p)
{
    Oc_xt_nd_hdr *src_hdr_p = get_hdr(src_p);
    Oc_xt_nd_hdr *trg_hdr_p = get_hdr(trg_p);

    oc_utl_debugassert(src_hdr_p->flags.leaf == trg_hdr_p->flags.leaf);
    oc_utl_debugassert(!src_hdr_p->flags.root);
    oc_utl_debugassert(!trg_hdr_p->flags.root);
    
    if (src_hdr_p->flags.leaf) {
        struct Oc_xt_key *key_p;
        struct Oc_xt_rcrd *rcrd_p;
        
        // This is a leaf node
        // 1. record the smallest entry (E) in [src_p]
        oc_xt_nd_leaf_get_kth(s_p, src_p,
                              0,
                              &key_p, &rcrd_p);
            
        oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_MOVE_MIN_KEY, wu_p,
                            "key=%s", oc_xt_nd_string_of_key(s_p, key_p));

        // 2. add E as the maximum in [trg_p]
        alloc_new_leaf_entry(wu_p, s_p, trg_hdr_p,
                             get_start_array(s_p, trg_p),
                             key_p, rcrd_p);
        shuffle_insert_key(trg_hdr_p, trg_hdr_p->num_used_entries-1);
        
        // 3. remove E from [src_p]
        shuffle_remove_key(src_hdr_p, 0);
    }
    else {
        struct Oc_xt_key *key_p;
        uint64 addr;
        
        // This is an index node
        // 1. record the smallest entry (E) in [src_p]
        oc_xt_nd_index_get_kth(s_p, src_p,
                                0,
                                &key_p, &addr);
        
        oc_xt_trace_wu_lvl(3, OC_EV_XT_ND_MOVE_MIN_KEY, wu_p,
                            "key=%s", oc_xt_nd_string_of_key(s_p, key_p));
        
        // 2. add E as the maximum in [trg_p]
        alloc_new_index_entry(s_p, trg_hdr_p,
                              get_start_array(s_p, trg_p),
                              key_p, &addr);
        shuffle_insert_key(trg_hdr_p, trg_hdr_p->num_used_entries-1);
        
        // 3. remove E from [src_p]
        shuffle_remove_key(src_hdr_p, 0);
    }
}

/**********************************************************************/

void oc_xt_nd_swap_root_ref(Oc_xt_state *s_p,
                          Oc_wu *trg_wu_p,
                          Oc_wu *src_wu_p)
{
    uint64 root_addr = s_p->root_node_p->disk_addr;

    // swap references between work-units
    s_p->cfg_p->node_get(trg_wu_p, root_addr);
    s_p->cfg_p->node_release(src_wu_p, s_p->root_node_p);
}

/**********************************************************************/
