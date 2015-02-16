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
/* OC_BPT.C  
 *
 * A second version of the b-tree
 */
/**********************************************************************/
#include <string.h>

#include "oc_utl_trk.h"

#include "oc_bpt_int.h"
#include "oc_bpt_nd.h"
#include "oc_bpt_utl.h"
#include "oc_bpt_trace.h"

#include "oc_bpt_op_insert.h"
#include "oc_bpt_op_lookup.h"
#include "oc_bpt_op_remove_key.h"

#include "oc_bpt_op_lookup_range.h"
#include "oc_bpt_op_insert_range.h"
#include "oc_bpt_op_remove_range.h"

#include "oc_bpt_op_validate.h"
#include "oc_bpt_op_validate_clones.h"
#include "oc_bpt_op_output_dot.h"
#include "oc_bpt_op_output_clones_dot.h"
#include "oc_bpt_op_stat.h"

// needed for the query function
#include "oc_rm_s.h"
/**********************************************************************/
// initialization functions
void oc_bpt_init(void)
{
    // do nothing
}

void oc_bpt_free_resources()
{
    // do nothing
}

void oc_bpt_init_state_b(struct Oc_wu *wu_pi,
                         Oc_bpt_state *state_po,
                         Oc_bpt_cfg *cfg_p,
                         uint64 tid)                         
{
    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_INIT_STATE, wu_pi, "tid=%Lu", tid);
    memset(state_po, 0, sizeof(Oc_bpt_state));
    oc_crt_init_rw_lock(&state_po->lock);
    state_po->cfg_p = cfg_p;
    state_po->tid = tid;
}
    
void oc_bpt_destroy_state(struct Oc_wu *wu_pi, Oc_bpt_state *s_p)
{
    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_DESTROY_STATE, wu_pi, "root_node=%Lu",
                        s_p->root_node_p);
    oc_utl_debugassert(s_p->root_node_p);

    // release the root node
    s_p->cfg_p->node_release(wu_pi, s_p->root_node_p);
    s_p->root_node_p = NULL;
}

/**********************************************************************/

void oc_bpt_init_config(Oc_bpt_cfg *cfg_p)
{
    int max_num_ent_root_leaf_node, max_num_ent_root_index_node;

    oc_utl_assert(cfg_p->node_alloc);
    oc_utl_assert(cfg_p->node_dealloc);
    oc_utl_assert(cfg_p->node_get_sl);
    oc_utl_assert(cfg_p->node_get_xl);
    oc_utl_assert(cfg_p->node_release);
    oc_utl_assert(cfg_p->node_mark_dirty);
    oc_utl_assert(cfg_p->fs_inc_refcount);
    oc_utl_assert(cfg_p->fs_get_refcount);
    oc_utl_assert(cfg_p->key_compare);
    oc_utl_assert(cfg_p->key_inc);
    oc_utl_assert(cfg_p->key_to_string);

    
    // The data-release function can be null.
    // oc_utl_assert(cfg_p->data_release);
    oc_utl_assert(cfg_p->data_to_string);
    
    if (cfg_p->key_size % 4 != 0 ||
        cfg_p->data_size % 4 != 0 ||
        cfg_p->node_size % 4 != 0 )
        ERR(("key/data/node size are misaligned; not divisible by 4"));

    if ( cfg_p->node_size < (int)sizeof(Oc_bpt_nd_hdr) )
        ERR(("node size is too tmall. smaller then Oc_bpt_nd_hdr"));
    if ( cfg_p->node_size < (int)sizeof(Oc_bpt_nd_hdr_root) )
        ERR(("node size is too tmall. smaller then Oc_bpt_nd_hdr_root"));
        
    cfg_p->leaf_ent_size = cfg_p->key_size + cfg_p->data_size;
    cfg_p->index_ent_size = sizeof(uint64) + cfg_p->key_size;
    
    cfg_p->max_num_ent_leaf_node =
        (cfg_p->node_size - sizeof(Oc_bpt_nd_hdr)) /
        cfg_p->leaf_ent_size;
    
    cfg_p->max_num_ent_index_node =
        (cfg_p->node_size - sizeof(Oc_bpt_nd_hdr)) /
        cfg_p->index_ent_size;
        
    max_num_ent_root_leaf_node =
        (cfg_p->node_size - sizeof(Oc_bpt_nd_hdr_root)) /
        cfg_p->leaf_ent_size;
        
    max_num_ent_root_index_node =
        (cfg_p->node_size - sizeof(Oc_bpt_nd_hdr_root)) /
        cfg_p->index_ent_size;

    cfg_p->max_num_ent_root_node =
        MIN(max_num_ent_root_leaf_node, max_num_ent_root_index_node);
        
    if (cfg_p->root_fanout > cfg_p->non_root_fanout)
        ERR(("The root-fanout cannot be larger than the node fanout"));
        
    if (cfg_p->root_fanout > 0)
        cfg_p->max_num_ent_root_node = MIN( cfg_p->root_fanout, 
                                            cfg_p->max_num_ent_root_node );

    if (cfg_p->non_root_fanout > 0) {    
        cfg_p->max_num_ent_leaf_node =
            MIN(cfg_p->max_num_ent_leaf_node, cfg_p->non_root_fanout);
        cfg_p->max_num_ent_index_node =
            MIN(cfg_p->max_num_ent_index_node, cfg_p->non_root_fanout);
    }

    if (cfg_p->max_num_ent_leaf_node > 256 ||
        cfg_p->max_num_ent_index_node > 256 ||
        cfg_p->max_num_ent_root_node > 256)
        printf("Warning: Fanout of a b-tree node is larger than 256."
               "The code will not be able to use these many entries;"
               "it is limited to 256.\n");
    
    cfg_p->max_num_ent_leaf_node =
        MIN(cfg_p->max_num_ent_leaf_node, 256);
    cfg_p->max_num_ent_index_node =
        MIN(cfg_p->max_num_ent_index_node, 256);
    cfg_p->max_num_ent_root_node =
        MIN(cfg_p->max_num_ent_root_node, 256);
    
    if (cfg_p->max_num_ent_leaf_node < 5 ||
        cfg_p->max_num_ent_index_node < 5 ||
        cfg_p->max_num_ent_root_node < 5)
        ERR(("Fanout of a b-tree node cannot be smaller than 5 in any of its nodes"));
    
    // We want the number of entries in a node to be between b and 3b.
    if (0 == cfg_p->min_num_ent)
        cfg_p->min_num_ent = MIN(
            MIN(cfg_p->max_num_ent_leaf_node/3,
                cfg_p->max_num_ent_index_node/3),
            cfg_p->max_num_ent_root_node/3
            );

    if (cfg_p->min_num_ent < 2) {
        // we didn't manage to create a range of [b .. 3b]. Now try [b .. 2b+1].
        cfg_p->min_num_ent = MIN(
            MIN((cfg_p->max_num_ent_leaf_node-1)/2,
                (cfg_p->max_num_ent_index_node-1)/2),
            (cfg_p->max_num_ent_root_node-1)/2
            );
    }
    
    if (cfg_p->min_num_ent < 2)
        ERR(("The minimal number of entries cannot be smaller than 2"));

    /* For correctness, the number of entries in the btree has to be between
     * b and 2b+1.
     */
    if (cfg_p->min_num_ent * 2 + 1 > cfg_p->max_num_ent_leaf_node ||
        cfg_p->min_num_ent * 2 + 1 > cfg_p->max_num_ent_index_node ||
        cfg_p->min_num_ent * 2 + 1 > cfg_p->max_num_ent_root_node)
        ERR(("The number of entries in a node is not (even) between b and 2b+1"));
    
    oc_bpt_trace_wu_lvl(
        1, OC_EV_BPT_INIT_CFG, NULL,
        "max_num_ent_leaf_node=%lu  max_num_ent_index_node=%lu max_num_ent_root=%lu\n",
        cfg_p->max_num_ent_leaf_node,
        cfg_p->max_num_ent_index_node,  
        cfg_p->max_num_ent_root_node);

    cfg_p->initialized = TRUE;
}

uint64 oc_bpt_create_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p)
{
    oc_utl_assert(NULL == s_p->root_node_p);
    oc_utl_debugassert(s_p->cfg_p->initialized);
        
    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    {
        s_p->root_node_p = s_p->cfg_p->node_alloc(wu_p);
        oc_bpt_nd_create_root(wu_p, s_p, s_p->root_node_p);
        oc_utl_trk_crt_unlock(wu_p, &s_p->root_node_p->lock);
    }
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    return s_p->root_node_p->disk_addr;
}

uint64 oc_bpt_get_tid(struct Oc_bpt_state *s_p)
{
    return s_p->tid;
}

/// Create a b-tree whose root is in address [addr]
void oc_bpt_init_map(
    struct Oc_wu *wu_p,
    Oc_bpt_cfg *cfg_p, 
    uint64 addr)
{
    oc_utl_debugassert(cfg_p->initialized);
    oc_bpt_nd_init_map(wu_p, cfg_p, addr);
}

void oc_bpt_cow_root_and_update_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_data *father_data_p,
    int size)
{
    // Old disk-addr stored as data in father
    uint64 prev_addr = *((uint64*)father_data_p); 
    Oc_bpt_node *node_p;   
    int fs_refcnt;
    
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_COW_ROOT_AND_UPDATE, wu_p,
                        "lba of root as appers in father (before update):%llu",
                        prev_addr);
    oc_utl_assert(NULL != s_p->root_node_p);
    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);

    node_p = s_p->cfg_p->node_get_xl(wu_p, s_p->root_node_p->disk_addr);
    oc_utl_debugassert(node_p == s_p->root_node_p);

    fs_refcnt = s_p->cfg_p->fs_get_refcount(
        wu_p,
        s_p->root_node_p->disk_addr);
    s_p->cfg_p->node_mark_dirty(wu_p, s_p->root_node_p, (fs_refcnt > 1));

    if (s_p->root_node_p->disk_addr != prev_addr)
    {
        uint64 new_addr = s_p->root_node_p->disk_addr;
        memcpy((char*)father_data_p, &new_addr, size);
    }
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_COW_ROOT_AND_UPDATE, wu_p,
                        "lba of root as appers in father (AFTER update):%llu",
                        *((uint64*)father_data_p));
    
    oc_bpt_nd_release(wu_p, s_p, s_p->root_node_p);// Dalit: may not be needed
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);   // Dalit: may not be needed

}


/**********************************************************************/

bool oc_bpt_insert_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p)
{
    bool rc;
    
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_INSERT, wu_p,
                        "tid=%Lu key=%s",
                        s_p->tid, 
                        oc_bpt_nd_string_of_key(s_p, key_p));    
    oc_utl_debugassert(s_p->cfg_p->initialized);

    oc_utl_trk_crt_lock_read(wu_p, &s_p->lock);
    rc = oc_bpt_op_insert_b(wu_p, s_p, key_p, data_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    return rc;
}
/**********************************************************************/

bool oc_bpt_lookup_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_po)
{
    bool rc;
    
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_LOOKUP_KEY, wu_p, "tid=%Lu key=%s",
                        s_p->tid, oc_bpt_nd_string_of_key(s_p, key_p));        
    oc_utl_debugassert(s_p->cfg_p->initialized);
    
    oc_utl_trk_crt_lock_read(wu_p, &s_p->lock);
    rc = oc_bpt_op_lookup_b(wu_p, s_p, key_p, data_po);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    return rc;
}

bool oc_bpt_remove_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    struct Oc_bpt_key *key_p)
{
    bool rc;
    
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_REMOVE_KEY, wu_p, "tid=%Lu %s",
                        s_p->tid, oc_bpt_nd_string_of_key(s_p, key_p));
    oc_utl_debugassert(s_p->cfg_p->initialized);
    
    oc_utl_trk_crt_lock_read(wu_p, &s_p->lock);
    rc = oc_bpt_op_remove_key_b(wu_p, s_p, key_p);    
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    return rc;
}

void oc_bpt_delete_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p)
{
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_DELETE, wu_p, "tid=%Lu", s_p->tid);
    oc_utl_debugassert(s_p->cfg_p->initialized);
    
    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);

    /* We need to upgrade the root-lock to a shared-lock
     * because the delete-code unlocks all the pages after
     * deleting them. This means that the root cannot remain
     * unlocked, as usual. 
     */
    oc_utl_trk_crt_lock_read(wu_p, &s_p->root_node_p->lock);
    oc_bpt_utl_delete_subtree_b(wu_p, s_p, s_p->root_node_p);

    s_p->root_node_p = NULL;
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
}

bool oc_bpt_dbg_validate_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p)
{
    bool rc;
    
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_VALIDATE, wu_p, "tid=%Lu", s_p->tid);
    oc_utl_debugassert(s_p->cfg_p->initialized);

    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    rc = oc_bpt_op_validate_b(wu_p, s_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    return rc;
}


bool oc_bpt_dbg_validate_clones_b(
    struct Oc_wu *wu_p,
    int n_clones,
    Oc_bpt_state *st_array[])
{
    bool rc;
    int i;

    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_VALIDATE_CLONES, wu_p, "");

    // lock all the clones
    for (i=0; i<n_clones; i++) {
        Oc_bpt_state *s_p = st_array[i];

        oc_utl_debugassert(s_p->cfg_p->initialized);
        oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    }
    
    rc = oc_bpt_op_validate_clones_b(wu_p, n_clones, st_array);

    // unlock all the clones    
    for (i=0; i<n_clones; i++) {
        Oc_bpt_state *s_p = st_array[i];        

        oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
    }

    return rc;
}

void oc_bpt_statistics_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p)
{
    Oc_bpt_statistics statistics;
    
    oc_utl_debugassert(s_p->cfg_p->initialized);
    return oc_bpt_op_statistics_b(wu_p,s_p, &statistics);
}
 
/**********************************************************************/
    
void oc_bpt_dbg_output_init(void)
{
    printf("digraph G {\n");
    printf("    node [shape=record, color=yellow]\n");
}

void oc_bpt_dbg_output_end(void)
{
    printf("}\n");
}

void oc_bpt_dbg_output_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    char *tag_p)
{
    oc_utl_debugassert(s_p->cfg_p->initialized);

    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    oc_bpt_op_output_dot_b(wu_p, s_p, tag_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
}

void oc_bpt_dbg_output_clones_b(
    struct Oc_wu *wu_p,
    int n_clones,
    struct Oc_bpt_state *st_array[],
    char *tag_p)
{
    int i;

    for (i=0 ; i < n_clones; i++) {
        struct Oc_bpt_state *s_p = st_array[i];

        oc_utl_debugassert(s_p->cfg_p->initialized);
        oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    }
    
    oc_bpt_op_output_clones_dot_b(wu_p, n_clones,
                                  st_array,
                                  tag_p);
    
    for (i=0 ; i < n_clones; i++) {
        struct Oc_bpt_state *s_p = st_array[i];
        oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
    }
}

/**********************************************************************/
// range operations

void oc_bpt_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p,
    int max_num_keys_i,
    struct Oc_bpt_key *key_array_po,
    struct Oc_bpt_data *data_array_po,
    int *nkeys_found_po)
{
    struct Oc_bpt_op_lookup_range lkr;
    
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_LOOKUP_RANGE, wu_p, "tid=%Lu [%s]",
                        s_p->tid,
                        oc_bpt_nd_string_of_2key(s_p, min_key_p, max_key_p));
    oc_utl_debugassert(s_p->cfg_p->initialized);
    
    lkr.min_key_p = min_key_p;
    lkr.max_key_p = max_key_p;
    lkr.max_num_keys_i = max_num_keys_i;
    lkr.key_array_po = key_array_po;
    lkr.data_array_po = data_array_po;
    lkr.nkeys_found_po = nkeys_found_po;

    // There are too many arguments, we stuff them into a single structure
    oc_utl_trk_crt_lock_read(wu_p, &s_p->lock);
    oc_bpt_op_lookup_range_b(wu_p, s_p, &lkr);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
}


int oc_bpt_insert_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_data *data_array)
{
    int rc;
    
    if (0 == length) return 0;

    oc_bpt_trace_wu_lvl(
        2, OC_EV_BPT_INSERT_RANGE, wu_p, "tid=%Lu [%s]",
        s_p->tid,
        oc_bpt_nd_string_of_2key(
            s_p,
            oc_bpt_nd_key_array_kth(s_p, key_array, 0),
            oc_bpt_nd_key_array_kth(s_p, key_array, length-1)));
    
    oc_utl_debugassert(s_p->cfg_p->initialized);

    oc_utl_trk_crt_lock_read(wu_p, &s_p->lock);
    rc = oc_bpt_op_insert_range_b(wu_p, s_p, length, key_array, data_array);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_INSERT_RANGE, wu_p, "rc=%d", rc);
    return rc;
}

int oc_bpt_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p)
{
    int rc;
    
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_REMOVE_RANGE, wu_p, "tid=%Lu [%s]",
                        s_p->tid,
                        oc_bpt_nd_string_of_2key(s_p, min_key_p, max_key_p));
    oc_utl_debugassert(s_p->cfg_p->initialized);
    
    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    rc = oc_bpt_op_remove_range_b(wu_p, s_p, min_key_p, max_key_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    return rc;
}

/**********************************************************************/
// query function
void oc_bpt_query_b(
    struct Oc_wu *wu_p,
    Oc_bpt_cfg *cfg_p, 
    struct Oc_rm_resource *r_p,
    Oc_bpt_fid fid_i,
    void *param)
{
    int nkeys, num_iters;
    
    switch (fid_i) {
    case OC_BPT_FN_CREATE:
        // the root is allocated
        r_p->pm_pages++;
        r_p->fs_pages++;
        break;

    case OC_BPT_FN_CREATE_AT:
        // the root is pre-allocated
        r_p->pm_pages++;
        break;

    case OC_BPT_FN_DELETE:
        // A b-tree path is kept in memory. 
        r_p->pm_pages += OC_BPT_MAX_HEIGHT;
        break;

    case OC_BPT_FN_INSERT_KEY:
        /* During an insert we might hold three PM pages in case of split.
         * The worst case for the number of allocated disk pages is that the
         * whole tree path will be split.
         */
        r_p->pm_pages += 3;
        r_p->fs_pages += OC_BPT_MAX_HEIGHT;
        break;
        
    case OC_BPT_FN_LOOKUP_KEY:
        // Since crabbing is used only two pages are ever held simultaneous
        r_p->pm_pages += 2;
        break;
        
    case OC_BPT_FN_LOOKUP_KEY_WITH_COW:
        /* During a lookup_withcow we might hold two PM pages due to the 
         * Write lock.
         * The worst case for the number of allocated disk pages is that the
         * whole tree path will be COWed.
         */
        r_p->pm_pages += 2;
        r_p->fs_pages += OC_BPT_MAX_HEIGHT;
        break;
                
    case OC_BPT_FN_REMOVE_KEY:
        /* During remove-key we might hold three PM pages in case of merge.
         * The worst case for the number of allocated disk pages is that the
         * whole tree path will be merged. We take into account here snapshots. 
         */
        r_p->pm_pages += 3;
        r_p->fs_pages += OC_BPT_MAX_HEIGHT;
        break;

    case OC_BPT_FN_DBG_OUTPUT:
        // A b-tree path is kept in memory. 
        r_p->pm_pages += OC_BPT_MAX_HEIGHT;
        break;
        
    case OC_BPT_FN_DBG_VALIDATE:
        // A b-tree path is kept in memory. 
        r_p->pm_pages += OC_BPT_MAX_HEIGHT;
        break;

    case OC_BPT_FN_LOOKUP_RANGE:
        /* Mostly, two pages are used. The worst case is three; this 
         * occurs when the range overlaps a page-boundary.
         */
        r_p->pm_pages += 3;
        break;

    case OC_BPT_FN_INSERT_RANGE:
        /* we always crawl on a single tree path. The possiblity for splits
         * increases the worst case from two to three modified pages. 
         */
        r_p->pm_pages += 3;   

        // the number of disk-pages needed depends on the number of inserted keys
        nkeys = *(int*) ((void*)param);
        num_iters = 1 + (nkeys / (cfg_p->max_num_ent_leaf_node/2));
        r_p->fs_pages += num_iters * OC_BPT_MAX_HEIGHT;
        break;
        
    case OC_BPT_FN_REMOVE_RANGE:
        // We might hold a page, its father, and its two siblings. 
        r_p->pm_pages += 4;

        // per tree level we might remove two disk pages and mark another page dirty
        r_p->fs_pages += 3 * OC_BPT_MAX_HEIGHT;
        break;
        
    default:
        oc_utl_assert(0);
    }
    
    oc_bpt_trace_wu_lvl(3, OC_EV_BPT_QUERY, wu_p, 
                        "pm_write_pages=%d  pm_read_pages=%d  disk_pages=%d", 
                        r_p->pm_pages, 
                        r_p->pm_pages,
                        r_p->fs_pages);
}


/**********************************************************************/
// snapshot and clone section

uint64 oc_bpt_clone_b(
    struct Oc_wu *wu_p,
    Oc_bpt_state *src_p,
    Oc_bpt_state *trg_p)
{
    // make sure the configurations are equivalent
    oc_utl_assert(trg_p->cfg_p == src_p->cfg_p);

    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_CLONE, wu_p,
                        "tid=%Lu -> tid=%Lu",
                        src_p->tid, trg_p->tid); 
        
    oc_utl_debugassert(src_p->cfg_p->initialized);
    oc_utl_debugassert(trg_p->cfg_p->initialized);
    oc_utl_assert(NULL == trg_p->root_node_p);
        
    oc_utl_trk_crt_lock_write(wu_p, &src_p->lock);
    oc_bpt_nd_clone_root(wu_p, src_p, trg_p);
    oc_utl_trk_crt_unlock(wu_p, &src_p->lock);

    return trg_p->root_node_p->disk_addr;
}

/**********************************************************************/

void oc_bpt_iter_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_state *s_p,
    void (*iter_f)(struct Oc_wu *, Oc_bpt_node*))
{
    oc_bpt_trace_wu_lvl(2, OC_EV_BPT_ITER, wu_p, "tid=%Lu", s_p->tid);    

    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    oc_bpt_utl_iter_b(wu_p, s_p, iter_f, s_p->root_node_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
}

/**********************************************************************/
