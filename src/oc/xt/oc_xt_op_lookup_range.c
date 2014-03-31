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
/* OC_XT_OP_LOOKUP_RANGE.C
 *
 * Search for a range of keys in a b-tree
 */
/**********************************************************************/
/*
 * Search for all keys inside a particular range. The implementation is a
 * more general form of lookup. The implementation uses a 
 * mini-lookup(min,max) function that can descend through a tree and search for
 * the range in a leaf or two. A loop around mini-lookup ensures
 * that long ranges are supported.
 * 
 * The basic operation of mini-lookup(min,max) is to search for
 * the range around [min] using an upper and lower limit. An
 * iterative descent is performed on the tree with two tree nodes: one
 * holding an upper bound and another holding a lower bound. At the end
 * of the recursion one or two leaf nodes are found. 
 * 
 */
/**********************************************************************/
#include <string.h>
#include <alloca.h>

#include "oc_utl.h"
#include "oc_xt_int.h"
#include "oc_xt_trace.h"
#include "oc_xt_nd.h"
#include "oc_xt_op_lookup_range.h"
/**********************************************************************/
static bool check_in_bounds(Oc_xt_state *s_p,
                            Oc_xt_node *node_p,
                            struct Oc_xt_key *key_p);

static bool search_in_leaf(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    Oc_xt_op_lookup_range *lkr_p );        
static bool simple_descent_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    Oc_xt_op_lookup_range *lkr_p );    
static bool mini_lookup_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_op_lookup_range *lkr_p );

/**********************************************************************/
/* copy the intersection of [key_p, rcrd_p] by bounded [min_key_p, max_key_p] 
 * into [lkr_p]
 */
static void copy_partial_ext(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    struct Oc_xt_key *key_p, 
    struct Oc_xt_rcrd *rcrd_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    Oc_xt_op_lookup_range *lkr_p)
{
    struct Oc_xt_key *key_array_p[3];
    struct Oc_xt_rcrd *rcrd_array_p[3];
    
    // make sure there is room to copy into 
    if (*(lkr_p->nx_found_po) == lkr_p->max_num_keys_i) return;

    /* setup space for returned values; we are going to split extent
     * [key_p, rcrd_p]
     */
    key_array_p[0] = NULL;
    rcrd_array_p[0] = NULL;
    key_array_p[1] =
        oc_xt_nd_key_array_kth(s_p, lkr_p->key_array_po, *(lkr_p->nx_found_po));
    rcrd_array_p[1] =
        oc_xt_nd_rcrd_array_kth(s_p, lkr_p->rcrd_array_po, *(lkr_p->nx_found_po)); 
    key_array_p[2] = NULL;
    rcrd_array_p[2] = NULL;
    
    // split the record and copy the intersection to the output array.
    s_p->cfg_p->rcrd_bound_split(key_p, rcrd_p,
                                 min_key_p, max_key_p,
                                 key_array_p, rcrd_array_p);

    *lkr_p->nx_found_po = *lkr_p->nx_found_po + 1;
}

/**********************************************************************/
/* Check that in [node_p] there are keys greater or equal to [key_p].
 */ 
static bool check_in_bounds(Oc_xt_state *s_p,
                            Oc_xt_node *node_p,
                            struct Oc_xt_key *key_p)
{
    struct Oc_xt_key *max_key_p;

    max_key_p = (struct Oc_xt_key *) alloca(s_p->cfg_p->key_size);
    oc_xt_nd_max_ofs(s_p, node_p, max_key_p);
    
    switch (s_p->cfg_p->key_compare(key_p, max_key_p)) {
    case -1:
        return FALSE;
        break;
    case 0:
        break;
    case 1:
        break;                    
    }
    
    return TRUE;
}

/* search in [node_p] for keys in the range.
 * copy extents into the output arrays. 
 * update the total count of keys found [nx_found_po].
 *
 * return TRUE if any entries were found. Return FALSE otherwise.
 *
 * note: the caller may specify if the minimum-key is included or not. 
 */
static bool search_in_leaf(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p, 
    Oc_xt_op_lookup_range *lkr_p )    
{
    int loc_lo, loc_hi, i, cursor_keys, cursor_rcrd;    
    struct Oc_xt_key *key_p;
    struct Oc_xt_rcrd *rcrd_p;
    
    oc_utl_debugassert(oc_xt_nd_is_leaf(s_p, node_p));
    oc_utl_debugassert(*(lkr_p->nx_found_po) < lkr_p->max_num_keys_i);
    
    oc_xt_trace_wu_lvl(
        3, OC_EV_XT_LOOKUP_RNG_SEARCH_LEAF, wu_p, "leaf=[%s] range=[%s] found_so_far=%d",
        oc_xt_nd_string_of_node(s_p, node_p),
        oc_xt_nd_string_of_2key(s_p, lkr_p->min_key_p, lkr_p->max_key_p),
        *lkr_p->nx_found_po);
    
    // find the first extent that is greater or equal than [min_key_p]
    loc_lo = oc_xt_nd_leaf_lookup_ge_key(wu_p, s_p, node_p, lkr_p->min_key_p);
    if (-1 == loc_lo) 
        // nothing matching
        return FALSE;
    
    // find the first extent that is smaller or equal than [max_key_p]
    loc_hi = oc_xt_nd_leaf_lookup_le_key(wu_p, s_p, node_p, lkr_p->max_key_p);
    if (-1 == loc_hi) 
        // nothing matching
        return FALSE;
    
    // make sure that there is something in the range.
    if (loc_lo > loc_hi)
        return FALSE;
    
    oc_utl_debugassert(loc_lo < oc_xt_nd_num_entries(s_p, node_p));
    oc_utl_debugassert(0 <= loc_lo);
    oc_utl_debugassert(loc_hi < oc_xt_nd_num_entries(s_p, node_p));
    
    oc_xt_trace_wu_lvl(3, OC_EV_XT_LOOKUP_RNG, wu_p, "loc_lo=%d loc_hi=%d sum=%d",
                       loc_lo, loc_hi, loc_hi-loc_lo+1);
    
    // Copy all entries between [loc_lo] and [loc_hi]
    
    /* The first and last extents may be a partial match, they require
     * special attention.
     */
    oc_xt_nd_leaf_get_kth(s_p, node_p, loc_lo, &key_p, &rcrd_p);
    copy_partial_ext(wu_p, s_p,
                     key_p, rcrd_p,
                     lkr_p->min_key_p, lkr_p->max_key_p,
                     lkr_p);

    // return early if we found only a single extent
    if (loc_lo == loc_hi)
        return TRUE;
    
    /* copy the set of middle extents, they are completely between lo-key and
     * hi-key.
     */
    cursor_keys = (*lkr_p->nx_found_po) * s_p->cfg_p->key_size;
    cursor_rcrd = (*lkr_p->nx_found_po) * s_p->cfg_p->rcrd_size;
    for (i=loc_lo+1;
         i<=loc_hi-1 && *lkr_p->nx_found_po < lkr_p->max_num_keys_i;
         i++)
    {
        // get the next entry in the page
        oc_xt_nd_leaf_get_kth(s_p, node_p, i, &key_p, &rcrd_p);
        
        memcpy((char*)lkr_p->key_array_po + cursor_keys, 
               (char*)key_p,
               s_p->cfg_p->key_size);
        memcpy((char*)lkr_p->rcrd_array_po + cursor_rcrd, 
               (char*)rcrd_p,
               s_p->cfg_p->rcrd_size);
        *lkr_p->nx_found_po = *lkr_p->nx_found_po + 1;
        cursor_keys += s_p->cfg_p->key_size;
        cursor_rcrd += s_p->cfg_p->rcrd_size;
    }
    
    // copy the last extent
    oc_xt_nd_leaf_get_kth(s_p, node_p, loc_hi, &key_p, &rcrd_p);
    copy_partial_ext(wu_p, s_p,
                     key_p, rcrd_p,
                     lkr_p->min_key_p, lkr_p->max_key_p,
                     lkr_p);
    return TRUE;
}

/* Start searching from [node_p] down.
 * prerequisit:
 *    [node_p] is locked for read. 
 */
static bool simple_descent_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_node *node_p,
    Oc_xt_op_lookup_range *lkr_p )    
{
    bool rc;
    Oc_xt_node *child_p, *father_p;
    uint64 addr;

    oc_xt_trace_wu_lvl(
        3, OC_EV_XT_LOOKUP_RNG_SIMPLE_DECENT, wu_p, "range=[%s]",
        oc_xt_nd_string_of_2key (s_p, lkr_p->min_key_p, lkr_p->max_key_p));
    
    if (oc_xt_nd_is_leaf(s_p, node_p)) {
        // [node_p] is a leaf
        rc = search_in_leaf(wu_p, s_p, node_p, lkr_p);
        oc_xt_nd_release(wu_p, s_p, node_p);
        return rc;
    }

    father_p = node_p;

    // descend down the tree
    while (1) {
        addr = oc_xt_nd_index_lookup_key(wu_p, s_p, father_p, lkr_p->min_key_p,
                                         NULL, NULL);
        if (0 == addr)
            addr = oc_xt_nd_index_lookup_min_key(wu_p, s_p, father_p);
        child_p = oc_xt_nd_get_for_read(wu_p, s_p, addr);
        
        if (oc_xt_nd_is_leaf(s_p, child_p)) {
            rc = search_in_leaf(wu_p, s_p, child_p, lkr_p);
            oc_xt_nd_release(wu_p, s_p, child_p);
            oc_xt_nd_release(wu_p, s_p, father_p);
            return rc;
        }
        else {
            // release the father
            oc_xt_nd_release(wu_p, s_p, father_p);
            
            // switch places between father and son.
            father_p = child_p;
        }
    }
}

/* A limited lookup that searches in a tree of height 2.
 * 
 * prerequisites:
 *   - a tree of height more than 1
 *
 * Decend in the tree using lock coupling with locks taken in
 * read-mode. Stop when you reach a leaf C. Mark the father as
 * F. Search for matches in C then continue into the
 * other children of F. 
 *
 */
static bool mini_lookup_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_op_lookup_range *lkr_p )    
{
    Oc_xt_node *father_p, *child_p, *hi_p = NULL;
    bool rc;
    
    oc_xt_trace_wu_lvl(
        3, OC_EV_XT_LOOKUP_RNG_MINI, wu_p, "range=[%s]",
        oc_xt_nd_string_of_2key (s_p, lkr_p->min_key_p, lkr_p->max_key_p));

    oc_xt_nd_get_for_read(wu_p, s_p, s_p->root_node_p->disk_addr);

    // 1. base case, this is the root node and the root is a leaf
    if (oc_xt_nd_is_leaf(s_p, s_p->root_node_p))
    {
        rc = search_in_leaf(wu_p, s_p, s_p->root_node_p, lkr_p);
        oc_xt_nd_release(wu_p, s_p, s_p->root_node_p);
        return rc;
    }

    father_p = s_p->root_node_p;

    // iteration
    while (1) {
        int loc_lo, loc_hi;
        uint64 addr;
        struct Oc_xt_key *dummy_key_p;
        
        oc_xt_trace_wu_lvl(3, OC_EV_XT_LOOKUP_RNG_MINI_ITER, wu_p, "");

        // compute the upper and lower bounds
        loc_lo = oc_xt_nd_index_lookup_le_key(wu_p, s_p, father_p, lkr_p->min_key_p);
        loc_hi = oc_xt_nd_index_lookup_ge_key(wu_p, s_p, father_p, lkr_p->min_key_p);
        oc_utl_assert(loc_lo != -1 ||
                      loc_hi != -1);
        if (-1 == loc_lo)
            loc_lo = loc_hi;
        if (-1 == loc_hi)
            loc_hi = loc_lo;

        // start by assuming that the lower-bound is the right direction
        oc_xt_nd_index_get_kth(s_p, father_p, loc_lo, &dummy_key_p, &addr);
        child_p = oc_xt_nd_get_for_read(wu_p, s_p, addr);

        /* If [child_p] contains [min_key_p] then don't bother with the 
         * high bound.
         */
        if (check_in_bounds(s_p, child_p, lkr_p->min_key_p))
            if (hi_p) {
                oc_xt_nd_release(wu_p, s_p, hi_p);
                hi_p = NULL;
            }
        
        if (loc_hi != loc_lo &&
            !check_in_bounds(s_p, child_p, lkr_p->min_key_p))
        {
            /* we are running the risk of missing some search hits.
             * Take a read-lock on the page with the high-bound.
             * Hold on to this page until further notice. 
             */
            oc_xt_trace_wu_lvl(3, OC_EV_XT_LOOKUP_RNG_MINI_SPLIT, wu_p, "");
            if (hi_p)
                oc_xt_nd_release(wu_p, s_p, hi_p);
            oc_xt_nd_index_get_kth(s_p, father_p, loc_hi, &dummy_key_p, &addr);
            hi_p = oc_xt_nd_get_for_read(wu_p, s_p, addr);
        }
        
        // continue descent with [child_p] only

        if (oc_xt_nd_is_leaf(s_p, child_p)) {
            // found a leaf with some keys
            rc = search_in_leaf(wu_p, s_p, child_p, lkr_p);
            oc_xt_nd_release(wu_p, s_p, father_p);
            oc_xt_nd_release(wu_p, s_p, child_p);
            break;
        }

        // release the father
        oc_xt_nd_release(wu_p, s_p, father_p);
        
        // switch places between father and son.
        father_p = child_p;
    }

    if (hi_p) {
        // We might have missed the search so we try the high bound as well.
        rc |= simple_descent_b(wu_p, s_p, hi_p, lkr_p);
    }
    
    return rc;
}

/**********************************************************************/
void oc_xt_op_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_state *s_p,
    Oc_xt_op_lookup_range *lkr_p )
{
    bool rc;
    struct Oc_xt_key *cursor_p;

    *lkr_p->nx_found_po = 0;
    if (0 == lkr_p->max_num_keys_i ||
        s_p->cfg_p->key_compare(lkr_p->min_key_p, lkr_p->max_key_p) == -1)
        return;
    
    if (oc_xt_nd_num_entries(s_p, s_p->root_node_p) == 0) {
        return;
    }
    
    cursor_p = (struct Oc_xt_key*)alloca(s_p->cfg_p->key_size);
    memcpy((char*)cursor_p, (char*)lkr_p->min_key_p, s_p->cfg_p->key_size);
    
    while (*lkr_p->nx_found_po < lkr_p->max_num_keys_i)
    {
        struct Oc_xt_key *max_key_so_far_p;
        struct Oc_xt_rcrd *max_rcrd_so_far_p;
        
        rc = mini_lookup_b(wu_p, s_p, lkr_p);
        
        if (!rc)
            // no more keys found in the range, we're done
            return;
        
        /* Update the minimal key searched for.
         * After the first search we move the minimal-key
         * forward.
         */
        oc_utl_debugassert(*lkr_p->nx_found_po > 0);
        max_key_so_far_p = oc_xt_nd_key_array_kth(
            s_p, lkr_p->key_array_po, *lkr_p->nx_found_po - 1);
        max_rcrd_so_far_p = oc_xt_nd_rcrd_array_kth(
            s_p, lkr_p->rcrd_array_po, *lkr_p->nx_found_po - 1);
        s_p->cfg_p->rcrd_end_offset(max_key_so_far_p, max_rcrd_so_far_p,
                                    cursor_p);
        s_p->cfg_p->key_inc(cursor_p, cursor_p);

        if (s_p->cfg_p->key_compare(cursor_p, lkr_p->max_key_p) == -1)
            // We have found all there is to find
            return;

        lkr_p->min_key_p = cursor_p;
    }
}


