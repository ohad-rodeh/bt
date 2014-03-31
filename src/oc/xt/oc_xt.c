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
/* OC_XT.C  
 *
 * A second version of the b-tree
 */
/**********************************************************************/
#include <string.h>

#include "oc_utl_trk.h"

#include "oc_xt_int.h"
#include "oc_xt_nd.h"
#include "oc_xt_trace.h"

#include "oc_xt_op_delete.h"

#include "oc_xt_op_lookup_range.h"
#include "oc_xt_op_insert_range.h"
#include "oc_xt_op_remove_range.h"

#include "oc_xt_op_validate.h"
#include "oc_xt_op_output_dot.h"
#include "oc_xt_op_stat.h"
// needed for the query function
#include "oc_rm_s.h"
/**********************************************************************/
// initialization functions
void oc_xt_init(void)
{
    // do nothing
}

void oc_xt_free_resources(void)
{
    // do nothing
}

void oc_xt_init_state_b(struct Oc_wu *wu_pi, Oc_xt_state *state_po, Oc_xt_cfg *cfg_p)
{
    memset(state_po, 0, sizeof(Oc_xt_state));
    oc_crt_init_rw_lock(&state_po->lock);
    state_po->cfg_p = cfg_p;
}

void oc_xt_destroy_state(struct Oc_wu *wu_pi, Oc_xt_state *s_p)
{
    oc_xt_trace_wu_lvl(3, OC_EV_XT_DESTROY_STATE, wu_pi, "root_node=%Lu",
                        s_p->root_node_p);
    oc_utl_debugassert(s_p->root_node_p);

    // release the root node
    s_p->cfg_p->node_release(wu_pi, s_p->root_node_p);
    s_p->root_node_p = NULL;
}

/**********************************************************************/

void oc_xt_init_config(Oc_xt_cfg *cfg_p)
{
    int max_num_ent_root_leaf_node, max_num_ent_root_index_node;

    oc_utl_assert(cfg_p->node_alloc);
    oc_utl_assert(cfg_p->node_dealloc);
    oc_utl_assert(cfg_p->node_get);
    oc_utl_assert(cfg_p->node_release);
    oc_utl_assert(cfg_p->node_mark_dirty);
    oc_utl_assert(cfg_p->key_compare);
    oc_utl_assert(cfg_p->key_inc);
    oc_utl_assert(cfg_p->key_to_string);
    oc_utl_assert(cfg_p->rcrd_compare);
    oc_utl_assert(cfg_p->rcrd_compare0);
    oc_utl_assert(cfg_p->rcrd_bound_split);
    oc_utl_assert(cfg_p->rcrd_split);
    oc_utl_assert(cfg_p->rcrd_end_offset);
    oc_utl_assert(cfg_p->rcrd_chop_length);
    oc_utl_assert(cfg_p->rcrd_chop_top);
    oc_utl_assert(cfg_p->rcrd_length);
    oc_utl_assert(cfg_p->rcrd_release);
    oc_utl_assert(cfg_p->rcrd_to_string);
    oc_utl_assert(cfg_p->fs_query_alloc);
    oc_utl_assert(cfg_p->fs_query_dealloc);
    
    // The data-release function can be null.
    // oc_utl_assert(cfg_p->data_release);
    oc_utl_assert(cfg_p->rcrd_to_string);
    
    if (cfg_p->key_size % 4 != 0 ||
        cfg_p->rcrd_size % 4 != 0 ||
        cfg_p->node_size % 4 != 0 )
        ERR(("key/data/node size are misaligned; not divisible by 4"));
        
    if ( cfg_p->node_size < (int)sizeof(Oc_xt_nd_hdr) )
        ERR(("node size is too tmall. smaller then Oc_xt_nd_hdr"));
    if ( cfg_p->node_size < (int)sizeof(Oc_xt_nd_hdr_root) )
        ERR(("node size is too tmall. smaller then Oc_xt_nd_hdr_root"));

    cfg_p->leaf_ent_size = cfg_p->key_size + cfg_p->rcrd_size;
    cfg_p->index_ent_size = sizeof(uint64) + cfg_p->key_size;
    
    cfg_p->max_num_ent_leaf_node =
        (cfg_p->node_size - sizeof(Oc_xt_nd_hdr)) /
        cfg_p->leaf_ent_size;
    
    cfg_p->max_num_ent_index_node =
        (cfg_p->node_size - sizeof(Oc_xt_nd_hdr)) /
        cfg_p->index_ent_size;
        
    max_num_ent_root_leaf_node =
        (cfg_p->node_size - sizeof(Oc_xt_nd_hdr_root)) /
        cfg_p->leaf_ent_size;
        
    max_num_ent_root_index_node =
        (cfg_p->node_size - sizeof(Oc_xt_nd_hdr_root)) /
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
        WRN(("Fanout of a b-tree node is larger than 256. The code will not be able to use these many entries; it is limited to 256."));
    
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
    
    oc_xt_trace_wu_lvl(
        1, OC_EV_XT_INIT_CFG, NULL,
        "max_num_ent_leaf_node=%lu  max_num_ent_index_node=%lu max_num_ent_root=%lu\n",
        cfg_p->max_num_ent_leaf_node,
        cfg_p->max_num_ent_index_node,  
        cfg_p->max_num_ent_root_node);

    cfg_p->initialized = TRUE;
}


/**********************************************************************/

uint64 oc_xt_create_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p)
{
    oc_xt_trace_wu_lvl(2, OC_EV_XT_CREATE, wu_p, "");
    oc_utl_assert(NULL == s_p->root_node_p);
    oc_utl_debugassert(s_p->cfg_p->initialized);

    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    s_p->root_node_p = s_p->cfg_p->node_alloc(wu_p);
    oc_xt_nd_create_root(wu_p, s_p, s_p->root_node_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    return s_p->root_node_p->disk_addr;
}


void oc_xt_delete_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p)
{
    oc_xt_trace_wu_lvl(2, OC_EV_XT_DELETE, wu_p, "");
    oc_utl_debugassert(s_p->cfg_p->initialized);
    
    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    oc_xt_op_delete_b(wu_p, s_p);    
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
}

void oc_xt_cow_root_and_update_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    struct Oc_bpt_data *father_data_p,
    int size)
{
    // Old address stored as data in father
    uint64 prev_addr = *((uint64*)father_data_p); 
    Oc_xt_node *node_p;   
    
    oc_xt_trace_wu_lvl(2, OC_EV_XT_COW_ROOT_AND_UPDATE, wu_p,
                       "addr of root as appers in father (before update):%Lu",
                       prev_addr);
    oc_utl_assert(NULL != s_p->root_node_p);
    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);

    node_p = s_p->cfg_p->node_get(wu_p, s_p->root_node_p->disk_addr);
    oc_utl_debugassert(node_p == s_p->root_node_p);

    // Dalit: may not be needed    
    oc_utl_trk_crt_lock_write(wu_p, &s_p->root_node_p->lock);

    
    s_p->cfg_p->node_mark_dirty(wu_p, s_p->root_node_p);
    if (s_p->root_node_p->disk_addr != prev_addr)
    {
        uint64 new_addr = s_p->root_node_p->disk_addr;
        memcpy((char*)father_data_p, &new_addr, size);
    }
    oc_xt_trace_wu_lvl(2, OC_EV_XT_COW_ROOT_AND_UPDATE, wu_p,
                        "addr of root as appers in father (AFTER update):%Lu",
                        *((uint64*)father_data_p));
    
    oc_xt_nd_release(wu_p, s_p, s_p->root_node_p);// Dalit: may not be needed
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);   // Dalit: may not be needed

}


bool oc_xt_dbg_validate_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p)
{
    bool rc;
    
    oc_utl_debugassert(s_p->cfg_p->initialized);

    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    rc = oc_xt_op_validate_b(wu_p, s_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    return rc;
}

void oc_xt_statistics_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p)
{
    Oc_xt_statistics statistics;
    
    oc_utl_debugassert(s_p->cfg_p->initialized);
    return oc_xt_op_statistics_b(wu_p,s_p, &statistics);
}
 
/**********************************************************************/
    
void oc_xt_dbg_output_init(struct Oc_utl_file *_out_p)
{
    FILE *out_p = (FILE *) _out_p;

    fprintf(out_p, "digraph G {\n");
    fprintf(out_p, "    node [shape=record, color=yellow]\n");
}

void oc_xt_dbg_output_end(struct Oc_utl_file *_out_p)
{
    FILE *out_p = (FILE *) _out_p;
    
    fprintf(out_p, "}\n");
}

void oc_xt_dbg_output_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_state *s_p,
    struct Oc_utl_file *out_p,
    char *tag_p)
{
    oc_utl_debugassert(s_p->cfg_p->initialized);

    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    oc_xt_op_output_dot_b(wu_p, s_p, out_p, tag_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
}

/**********************************************************************/
// range operations

void oc_xt_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    int max_num_keys_i,
    struct Oc_xt_key *key_array_po,
    struct Oc_xt_rcrd *rcrd_array_po,
    int *nx_found_po)
{
    struct Oc_xt_op_lookup_range lkr;
    
    oc_xt_trace_wu_lvl(2, OC_EV_XT_LOOKUP_RANGE, wu_p, "[%s]",
                        oc_xt_nd_string_of_2key(s_p, min_key_p, max_key_p));
    oc_utl_debugassert(s_p->cfg_p->initialized);
    oc_utl_debugassert(key_array_po != NULL);
    oc_utl_debugassert(rcrd_array_po != NULL);
    
    lkr.min_key_p = min_key_p;
    lkr.max_key_p = max_key_p;
    lkr.max_num_keys_i = max_num_keys_i;
    lkr.key_array_po = key_array_po;
    lkr.rcrd_array_po = rcrd_array_po;
    lkr.nx_found_po = nx_found_po;

    // There are too many arguments, we stuff them into a single structure
    oc_utl_trk_crt_lock_read(wu_p, &s_p->lock);
    oc_xt_op_lookup_range_b(wu_p, s_p, &lkr);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
}


uint64 oc_xt_insert_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p)
{
    uint64 rc;
    
    oc_xt_trace_wu_lvl(
        2, OC_EV_XT_INSERT_RANGE, wu_p, "[%s]",
        oc_xt_nd_string_of_rcrd(
            s_p,
            key_p, rcrd_p));
    
    oc_utl_debugassert(s_p->cfg_p->initialized);

    oc_utl_trk_crt_lock_read(wu_p, &s_p->lock);
    rc = oc_xt_op_insert_range_b(wu_p, s_p, key_p, rcrd_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);

    oc_xt_trace_wu_lvl(3, OC_EV_XT_INSERT_RANGE, wu_p, "rc=%d", rc);
    return rc;
}

uint64 oc_xt_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p)
{
    int rc;
    
    oc_xt_trace_wu_lvl(2, OC_EV_XT_REMOVE_RANGE, wu_p, "[%s]",
                       oc_xt_nd_string_of_2key(s_p, min_key_p, max_key_p));
    oc_utl_debugassert(s_p->cfg_p->initialized);
    
    oc_utl_trk_crt_lock_write(wu_p, &s_p->lock);
    rc = oc_xt_op_remove_range_b(wu_p, s_p, min_key_p, max_key_p);
    oc_utl_trk_crt_unlock(wu_p, &s_p->lock);
    
    return rc;
}

void oc_xt_swap_root_ref(Oc_xt_state *s_p,
                         Oc_wu *trg_wu_p,
                         Oc_wu *src_wu_p)
{
    oc_xt_nd_swap_root_ref(s_p, trg_wu_p, src_wu_p);
}

/**********************************************************************/

#define CASE(s) case s: return #s ; break

// A string representation of the comparison return code
const char *oc_xt_string_of_cmp_rc(Oc_xt_cmp rc)
{
    switch (rc) {
        CASE(OC_XT_CMP_SML);
        CASE(OC_XT_CMP_GRT);
        CASE(OC_XT_CMP_EQUAL);
        CASE(OC_XT_CMP_COVERED);
        CASE(OC_XT_CMP_FULLY_COVERS);
        CASE(OC_XT_CMP_PART_OVERLAP_SML);
        CASE(OC_XT_CMP_PART_OVERLAP_GRT);

    default:
        ERR(("sanity"));
    }
}
#undef CASE

/**********************************************************************/
// query function
void oc_xt_query_b(
    struct Oc_wu *wu_p,
    Oc_xt_cfg *cfg_p, 
    struct Oc_rm_resource *r_p,
    Oc_xt_fid fid_i,
    void *param)
{
    int nkeys, num_iters;
    
    switch (fid_i) {
    case OC_XT_FN_CREATE:
        // the root is allocated
        r_p->pm_write_pages_i++;
        r_p->fs_pages++;
        cfg_p->fs_query_alloc(r_p, 1);  
        break;

    case OC_XT_FN_DELETE:
        // A b-tree path is kept in memory. 
        r_p->pm_read_pages_i += OC_XT_MAX_HEIGHT;

        // deallocation is performed one at a time
        cfg_p->fs_query_dealloc(r_p, 1);

        // no disk pages are used here
        break;

    case OC_XT_FN_DBG_OUTPUT:
        // A b-tree path is kept in memory. 
        r_p->pm_read_pages_i += OC_XT_MAX_HEIGHT;
        break;
        
    case OC_XT_FN_DBG_VALIDATE:
        // A b-tree path is kept in memory. 
        r_p->pm_read_pages_i += OC_XT_MAX_HEIGHT;
        break;

    case OC_XT_FN_LOOKUP_RANGE:
        /* Mostly, two pages are used. The worst case is three; this 
         * occurs when the range overlaps a page-boundary.
         */
        r_p->pm_read_pages_i += 3;
        break;

    case OC_XT_FN_INSERT_RANGE:
        /* we always crawl on a single tree path. The possiblity for splits
         * increases the worst case from two to three modified pages. 
         */
        r_p->pm_write_pages_i += 3;   

        // the number of disk-pages needed depends on the length of the extent
        nkeys = ((int) param)/ SS_PAGE_SIZE;
        num_iters = 1 + (nkeys / (cfg_p->max_num_ent_leaf_node/2));
        r_p->fs_pages += num_iters * OC_XT_MAX_HEIGHT;
        cfg_p->fs_query_alloc(r_p, 1);
        break;
        
    case OC_XT_FN_REMOVE_RANGE:
        // We might hold a page, its father, and its two siblings. 
        r_p->pm_write_pages_i += 4;

        // per tree level we might remove two disk pages and mark another page dirty
        r_p->fs_pages += 3 * OC_XT_MAX_HEIGHT;
        
        cfg_p->fs_query_dealloc(r_p, 1);
        
        /* in addition, there is a case where removing an extent causes
         * the creation of another extent. Account for the insertion of
         * an extent.
         */
        r_p->fs_pages += OC_XT_MAX_HEIGHT;
        
        // cfg_p->fs_query_alloc(r_p, 1);
        break;
        
    case OC_XT_FN_GET_ATTR:
        // The root page is already held; therefore, no more resources are required
        break;
        
    case OC_XT_FN_SET_ATTR:
        r_p->pm_write_pages_i++;
        r_p->fs_pages++;
        break;
        
    default:
        oc_utl_assert(0);
    }
    
    oc_xt_trace_wu_lvl(3, OC_EV_XT_QUERY, wu_p, 
                        "pm_write_pages=%d  pm_read_pages=%d  disk_pages=%d", 
                        r_p->pm_read_pages_i, 
                        r_p->pm_write_pages_i,
                        r_p->fs_pages);
}


/**********************************************************************/

