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
/* OC_XT_TEST_UTL.H
 *
 * Utilities for testing
 */
/**********************************************************************/
#include <malloc.h>
#include <string.h>
#include <stdlib.h>

#include "pl_trace.h"
#include "pl_trace_base.h"
#include "oc_utl.h"
#include "oc_crt_int.h"
#include "oc_xt_int.h"
#include "oc_xt_alt.h"
#include "oc_xt_test_fs.h"
#include "oc_xt_test_nd.h"
#include "oc_xt_test_utl.h"
#include "oc_xt_alt.h"

/**********************************************************************/
typedef enum Fs_type {
    FS_REAL,
    FS_ALT,
} Fs_type;

typedef struct Oc_xt_test_rcrd {
    uint32 len;
    uint32 data;

    /* which FS are we using?
     *
     * 0 - real
     * 1 - simple (alt) implementation
     */
    Fs_type fs_impl; 
} Oc_xt_test_rcrd;


static int key_compare(struct Oc_xt_key *key1_p,
                       struct Oc_xt_key *key2_p);
static void key_inc(struct Oc_xt_key *_key_p, struct Oc_xt_key *_result_p);
static void key_to_string(struct Oc_xt_key *key_p, char *str_p, int max_len);

static Oc_xt_cmp rcrd_compare(
    struct Oc_xt_key *key1_p,
    struct Oc_xt_rcrd *rcrd1_p,
    struct Oc_xt_key *key2_p,
    struct Oc_xt_rcrd *rcrd2_p);

static int rcrd_compare0(
    struct Oc_xt_key *key1_p,
    struct Oc_xt_key *key2_p,
    struct Oc_xt_rcrd *rcrd2_p);

static Oc_xt_cmp rcrd_bound_split(
    // record+key to chop
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p,
    
    // boundary keys
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    
    // resulting sub-extents
    struct Oc_xt_key *key_array_p[3],
    struct Oc_xt_rcrd *rcrd_array_p[3]);

static void rcrd_chop_length(
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p,
    uint64 len);

static void rcrd_split_into_sub(struct Oc_xt_key *_key_p,
                                struct Oc_xt_rcrd *_rcrd_p,
                                int num,
                                struct Oc_xt_key *_key_array,
                                struct Oc_xt_rcrd *_rcrd_array);

static void rcrd_chop_top(
    struct Oc_xt_key *key_po,
    struct Oc_xt_rcrd *rcrd_po,
    struct Oc_xt_key *hi_key_p);

static Oc_xt_cmp rcrd_split(
    // record+key to chop
    struct Oc_xt_key *key1_p,
    struct Oc_xt_rcrd *rcrd1_p,
    
    // record to provide boundary keys
    struct Oc_xt_key *key2_p,
    struct Oc_xt_rcrd *rcrd2_p,
    
    // resulting sub-extents
    struct Oc_xt_key *key_array_p[3],
    struct Oc_xt_rcrd *rcrd_array_p[3]);

static void rcrd_end_offset(struct Oc_xt_key *key_p,
                              struct Oc_xt_rcrd *rcrd_p,
                              
                              // returned key 
                              struct Oc_xt_key *end_key_po);
static uint64 rcrd_length(struct Oc_xt_key *key_p, struct Oc_xt_rcrd *rcrd_pi);

static void rcrd_release(struct Oc_wu *wu_p,
                         struct Oc_xt_key *key_p,
                         struct Oc_xt_rcrd *rcrd_p);
static void rcrd_to_string(struct Oc_xt_key *key_p,
                           struct Oc_xt_rcrd *rcrd_p,
                           char *str_p,
                           int max_len);
static void fs_query_dealloc( struct Oc_rm_resource *r_p, int n_pages);
static void fs_query_alloc( struct Oc_rm_resource *r_p, int n_pages);

static bool ext_array_compare(
    int nkeys_found_A,
    struct Oc_xt_key *key_array_A,
    struct Oc_xt_rcrd *rcrd_array_A, 
    int nkeys_found_B,
    struct Oc_xt_key *key_array_B,
    struct Oc_xt_rcrd *rcrd_array_B,
    bool allow_partial_match);

static void create_rcrd_from_start_and_end_ofs(
    uint32 key,
    uint32 end_key,
    uint32 ext_start_key,
    Oc_xt_test_rcrd *rcrd_p,
    struct Oc_xt_key *trg_key_p,
    struct Oc_xt_rcrd *trg_rcrd_p);
static void copy_ext(
    struct Oc_xt_key *trg_key_p,
    struct Oc_xt_rcrd *trg_rcrd_p,
    struct Oc_xt_key *src_key_p,
    struct Oc_xt_rcrd *src_rcrd_p);

/**********************************************************************/
static Oc_xt_cfg cfg;
static Oc_xt_state state;

static Oc_xt_alt_state alt_state;

/* two free-space instances, one for the real implementation, the other
 * for the dummy one.
 */
static struct Oc_xt_test_fs_ctx *fs_ctx_p, *fs_ctx_alt_p;

typedef uint32 Oc_xt_test_key;

int num_rounds = 100;
int max_int = 200;
int num_tasks = 10;
bool verbose = FALSE;
bool statistics = FALSE;
int max_root_fanout = 5;
int max_non_root_fanout = 5;
int min_fanout = 2;
int total_ops = 0;
Oc_xt_test_utl_type test_type = OC_XT_TEST_UTL_ANY;

// work-unit used for validate and display
static Oc_wu utl_wu;
static Oc_rm_ticket utl_rm_ticket;

/**********************************************************************/
static inline struct Oc_xt_key* key_array_kth(
    struct Oc_xt_state *s_p,
    struct Oc_xt_key* key_array,
    int k)
{
    return (struct Oc_xt_key*) ((char*)key_array + s_p->cfg_p->key_size * k);
}

static inline struct Oc_xt_rcrd* rcrd_array_kth(
    struct Oc_xt_state *s_p,
    struct Oc_xt_rcrd* rcrd_array,
    int k)
{
    return (struct Oc_xt_rcrd*) ((char*)rcrd_array + s_p->cfg_p->rcrd_size * k);
}
/**********************************************************************/
static int key_compare(struct Oc_xt_key *key1_p,
                       struct Oc_xt_key *key2_p)
{
    Oc_xt_test_key *key1 = (uint32*) key1_p;
    Oc_xt_test_key *key2 = (uint32*) key2_p;
    
    if (*key1 == *key2) return 0;
    else if (*key1 > *key2) return -1;
    else return 1;
}

static void key_inc(struct Oc_xt_key *_key_p, struct Oc_xt_key *_result_p)
{
    Oc_xt_test_key *key_p = (uint32*) _key_p;
    Oc_xt_test_key *result_p = (uint32*) _result_p;
    *result_p = *key_p;
    (*result_p)++;
}


static void key_to_string(struct Oc_xt_key *key_p, char *str_p, int max_len)
{
    if (max_len < 10)
        ERR(("key_to_string: %d is not enough", max_len));
    sprintf(str_p, "%lu", *((uint32*)key_p));
}

// compare a key to an extent
static int rcrd_compare0(
    struct Oc_xt_key *_key1_p,
    struct Oc_xt_key *_key2_p,
    struct Oc_xt_rcrd *_rcrd2_p)
{
    uint32 key1 = *(uint32*)_key1_p;
    Oc_xt_test_rcrd *rcrd2_p = (Oc_xt_test_rcrd *)_rcrd2_p;
    uint32 key2 = *(uint32*)_key2_p;
    uint32 end2 = key2 + rcrd2_p->len -1;

    if (key1 < key2) return 1;
    else if (key1 > end2) return -1;
    else return 0;
}

// compare two extents 
static Oc_xt_cmp rcrd_compare(
    struct Oc_xt_key *_key1_p,
    struct Oc_xt_rcrd *_rcrd1_p,
    struct Oc_xt_key *_key2_p,
    struct Oc_xt_rcrd *_rcrd2_p)
{
    uint32 key1 = *(uint32*)_key1_p;
    Oc_xt_test_rcrd *rcrd1_p = (Oc_xt_test_rcrd *)_rcrd1_p;
    uint32 end1 = key1 + rcrd1_p->len - 1;
    uint32 key2 = *(uint32*)_key2_p;
    Oc_xt_test_rcrd *rcrd2_p = (Oc_xt_test_rcrd *)_rcrd2_p;
    uint32 end2 = key2 + rcrd2_p->len -1;

    oc_utl_assert(rcrd1_p->len > 0);
    if (rcrd2_p->len ==  0)
        oc_utl_assert(0);

    // we need to compare the two extents and decide which of the cases
    // applies
    
    if ( end1 < key2) {
        // A is strictly smaller than B. For example A=[10-13] B=[15-16].
        return OC_XT_CMP_SML;
    }
    else if (key1 > end2) {
        // A is strictly larger than B. For example A=[10-13]  B=[8-9].
        return OC_XT_CMP_GRT;
    }
    else if (key1 == key2 &&
             end1 == end2) {
        // A and B cover exactly the same extent. For example A=[10-13] B=[10-13].
        return OC_XT_CMP_EQUAL;
    }
    else if (key1 >= key2 &&
             end1 <= end2) {
        // A is (strictly) covered by B. For example A=[10-13]  B=[9-15].
        return OC_XT_CMP_COVERED;
    }
    else if (key1 <= key2 &&
             end1 >= end2) {
        // A (strictly) fully covers B. For example A=[10-13]  B=[11-12].
        return OC_XT_CMP_FULLY_COVERS;
    }
    else if (key1 < key2 &&
             end1 >= key2 &&
             end1 <= end2) {
        // A and B (strictly) partially overlap. A is less than B,
        // For example A=[10-13] B=[12-20].
        return OC_XT_CMP_PART_OVERLAP_SML;
    }
    else if (key1 >= key2 &&
             key1 <= end2 &&
             end1 > end2) {
        // A and B (strictly) partially overlap. A is greater than B,
        // For example A=[10-13] B=[5-10].
        return OC_XT_CMP_PART_OVERLAP_GRT;
    }
    else {
        // ERR(("sanity, all comparison cases should have been covered"));
        printf("sanity, all comparison cases should have been covered\n");
        oc_utl_assert(0);
    }
}


// copy from source into target if the target pointers are non-null
static void copy_ext(
    struct Oc_xt_key *trg_key_p,
    struct Oc_xt_rcrd *trg_rcrd_p,
    struct Oc_xt_key *src_key_p,
    struct Oc_xt_rcrd *src_rcrd_p)
{
    if (trg_key_p != NULL)
        memcpy((char*)trg_key_p, (char*)src_key_p, sizeof(Oc_xt_test_key));
    if (trg_rcrd_p != NULL)
        memcpy((char*)trg_rcrd_p, (char*)src_rcrd_p, sizeof(Oc_xt_test_rcrd));
}

// create an extent from a start and end offset
static void create_rcrd_from_start_and_end_ofs(
    uint32 key,
    uint32 end_key,
    uint32 ext_start_key,
    Oc_xt_test_rcrd *rcrd_p,
    struct Oc_xt_key *trg_key_p,
    struct Oc_xt_rcrd *trg_rcrd_p)
{
    Oc_xt_test_rcrd rcrd;

    oc_utl_debugassert(end_key >= key);
    oc_utl_debugassert(key >= ext_start_key);    
    
    rcrd.len = end_key - key + 1;
    rcrd.data = rcrd_p->data + (key - ext_start_key);
    rcrd.fs_impl = rcrd_p->fs_impl;
    
    copy_ext(
        trg_key_p,
        trg_rcrd_p,
        (struct Oc_xt_key*)&key,
        (struct Oc_xt_rcrd*)&rcrd);
}


static void rcrd_chop_length(
    struct Oc_xt_key *_key_po,
    struct Oc_xt_rcrd *_rcrd_po,
    uint64 _len)
{
    uint32 *key_po = (uint32*)_key_po;
    Oc_xt_test_rcrd *rcrd_po = (Oc_xt_test_rcrd *) _rcrd_po;
    uint32 len = (uint32) _len;

    (*key_po) += len;
    rcrd_po->len -= len;
    rcrd_po->data += len;
}

static void rcrd_split_into_sub(struct Oc_xt_key *_key_p,
                                struct Oc_xt_rcrd *_rcrd_p,
                                int num,
                                struct Oc_xt_key *_key_array,
                                struct Oc_xt_rcrd *_rcrd_array)
{
    uint32 key = *(uint32*)_key_p;
    Oc_xt_test_rcrd *rcrd_p = (Oc_xt_test_rcrd *) _rcrd_p;
    uint32 *key_array = (uint32*) _key_array;
    Oc_xt_test_rcrd *rcrd_array = (Oc_xt_test_rcrd*)_rcrd_array;
    int i, remain_len, sub_len;

    /* make sure the request is to split into -more- than one sub-extent and
     * that the sub-extents will be -non-empty-.
     */
    oc_utl_assert(rcrd_p->len >= (uint32)num);
    oc_utl_assert(num > 1);

    sub_len = rcrd_p->len/num;
    remain_len = rcrd_p->len;
    
    for (i=0; i<num; i++) {
        key_array[i] = key + i * sub_len;
        rcrd_array[i].fs_impl = rcrd_p->fs_impl;

        if (i < num-1) {
            rcrd_array[i].len = sub_len;
            rcrd_array[i].data = rcrd_p->data + i * sub_len;
            remain_len -= sub_len;
        }
        else {
            rcrd_array[i].data = rcrd_p->data + (num-1) * sub_len;            
            rcrd_array[i].len = remain_len;
        }
    }
}

static void rcrd_chop_top(
    struct Oc_xt_key *_key_po,
    struct Oc_xt_rcrd *_rcrd_po,
    struct Oc_xt_key *hi_key_p)
{
    uint32 key = *(uint32*)_key_po;
    Oc_xt_test_rcrd *rcrd_po = (Oc_xt_test_rcrd *) _rcrd_po;
    uint32 top_key = *(uint32*) hi_key_p;

    oc_utl_assert(key < top_key);
    rcrd_po->len = MIN((top_key - key), rcrd_po->len);
}

static Oc_xt_cmp rcrd_split(
    // record+key to chop
    struct Oc_xt_key *_keyA_p,
    struct Oc_xt_rcrd *_rcrdA_p,
    
    // record to provide boundary keys
    struct Oc_xt_key *_keyB_p,
    struct Oc_xt_rcrd *_rcrdB_p,
    
    // resulting sub-extents
    struct Oc_xt_key *key_array_p[3],
    struct Oc_xt_rcrd *rcrd_array_p[3])
{
    uint32 keyA = *(uint32*)_keyA_p;
    Oc_xt_test_rcrd *rcrdA_p = (Oc_xt_test_rcrd *)_rcrdA_p;
    uint32 endA = keyA + rcrdA_p->len - 1;
    uint32 keyB = *(uint32*)_keyB_p;
    Oc_xt_test_rcrd *rcrdB_p = (Oc_xt_test_rcrd *)_rcrdB_p;
    uint32 endB = keyB + rcrdB_p->len - 1;
    Oc_xt_cmp rc;

    oc_utl_debugassert(rcrdA_p->fs_impl == FS_REAL ||
                       rcrdA_p->fs_impl == FS_ALT);
    rc = rcrd_compare(_keyA_p, _rcrdA_p, _keyB_p, _rcrdB_p);

    if (OC_XT_CMP_SML == rc) {
        // A is strictly smaller than B. For example A=[10-13] B=[15-16].
        copy_ext(
            key_array_p[0], rcrd_array_p[0],
            _keyA_p, _rcrdA_p );
        key_array_p[1] = NULL;
        rcrd_array_p[1] = NULL;
        key_array_p[2] = NULL;
        rcrd_array_p[2] = NULL;
    }
    else if (OC_XT_CMP_GRT == rc) {
        // A is strictly larger than B. For example A=[10-13]  B=[8-9].
        key_array_p[0] = NULL;
        rcrd_array_p[0] = NULL;
        key_array_p[1] = NULL;
        rcrd_array_p[1] = NULL;
        copy_ext(
            key_array_p[2], rcrd_array_p[2],
            _keyA_p, _rcrdA_p);
    }
    else if (OC_XT_CMP_EQUAL == rc) {
        // A and B cover exactly the same extent. For example A=[10-13] B=[10-13].
        key_array_p[0] = NULL;
        rcrd_array_p[0] = NULL;
        copy_ext(
            key_array_p[1], rcrd_array_p[1],
            _keyA_p, _rcrdA_p);
        key_array_p[2] = NULL;
        rcrd_array_p[2] = NULL;
    }
    else if (OC_XT_CMP_COVERED == rc) {
        // A is (strictly) covered by B. For example A=[10-13]  B=[9-15].
        key_array_p[0] = NULL;
        rcrd_array_p[0] = NULL;
        copy_ext(
            key_array_p[1], rcrd_array_p[1],
            _keyA_p, _rcrdA_p);
        key_array_p[2] = NULL;
        rcrd_array_p[2] = NULL;
    }
    else if (OC_XT_CMP_FULLY_COVERS == rc) {
        // A (strictly) fully covers B. For example A=[10-13]  B=[11-12].

        // A is split into three extents: A1,A2,A3.
        // Either A1 or A3 may be empty

        if (keyA < keyB) {
            // A1 is non-empty
            create_rcrd_from_start_and_end_ofs(
                keyA, keyB - 1,
                keyA, rcrdA_p,
                key_array_p[0], rcrd_array_p[0]
                );
        } else {
            // A1 is empty
            key_array_p[0] = NULL;
            rcrd_array_p[0] = NULL;
        }

        // Add the middle extent
        create_rcrd_from_start_and_end_ofs(
            keyB, endB,
            keyA, rcrdA_p,
            key_array_p[1], rcrd_array_p[1]);
        
        if ( endA > endB) {
            // A3 is non-empty
            create_rcrd_from_start_and_end_ofs(
                endB+1, endA,
                keyA, rcrdA_p,
                key_array_p[2], rcrd_array_p[2]);                
        } else {
            // A3 is empty
            key_array_p[2] = NULL;
            rcrd_array_p[2] = NULL;
        }
    }
    else if (OC_XT_CMP_PART_OVERLAP_SML == rc) {
        // A and B (strictly) partially overlap. A is less than B,
        // For example A=[10-13] B=[12-20].
        // A is split into two extents: A1,A2

        // A1 is non-empty
        oc_utl_debugassert(keyA < keyB);
        create_rcrd_from_start_and_end_ofs(
            keyA, keyB - 1,
            keyA, rcrdA_p,
            key_array_p[0], rcrd_array_p[0]
            );
        
        // Add the middle extent
        create_rcrd_from_start_and_end_ofs(
            keyB, endA,
            keyA, rcrdA_p,
            key_array_p[1], rcrd_array_p[1]
            );

        // last extent is empty
        key_array_p[2] = NULL;
        rcrd_array_p[2] = NULL;
    }
    else if (OC_XT_CMP_PART_OVERLAP_GRT == rc) {
        // A and B (strictly) partially overlap. A is greater than B,
        // For example A=[10-13] B=[5-10].
        // A is split into two extents: A2,A3

        // first extent is empty
        key_array_p[0] = NULL;
        rcrd_array_p[0] = NULL;

        // Add the middle extent
        create_rcrd_from_start_and_end_ofs(
            keyA, endB,
            keyA, rcrdA_p,
            key_array_p[1], rcrd_array_p[1]
            );        
        
        // A3 is non-empty
        oc_utl_debugassert(endA > endB);
        create_rcrd_from_start_and_end_ofs(        
            endB+1, endA,
            keyA, rcrdA_p,
            key_array_p[2], rcrd_array_p[2]
            );
    }
    
    // print results
    if (0) 
    {
        int i;
        
        printf("\n -- split [%lu-%lu] fs_impl=%d with bounds=[%lu-%lu] into:\n",
               keyA, endA,
               rcrdA_p->fs_impl,
               keyB, endB); 
        for (i=0; i<=2; i++) {
            if (NULL == key_array_p[i]) {
                printf("[%d]=NULL\n" , i);
            } else {
                uint32 end_key;

                rcrd_end_offset(key_array_p[i],
                                rcrd_array_p[i],
                                (struct Oc_xt_key *)& end_key);
                printf("[%d]=[%lu-%lu data=%lu fs_impl=%d]\n",
                       i, *(uint32*)key_array_p[i], end_key,
                       ((Oc_xt_test_rcrd*)rcrd_array_p[i])->data,
                       ((Oc_xt_test_rcrd*)rcrd_array_p[i])->fs_impl);
            }
        }
        fflush(stdout);
    }
    
    return rc;
}


static Oc_xt_cmp rcrd_bound_split(
    // record+key to chop
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p,
    
    // boundary keys
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    
    // resulting sub-extents
    struct Oc_xt_key *key_array_p[3],
    struct Oc_xt_rcrd *rcrd_array_p[3])
{
    uint32 min_key = *(uint32*)min_key_p;
    uint32 max_key = *(uint32*)max_key_p;
    Oc_xt_test_rcrd rcrd2;
    
    // convert the min/max keys into an extent
    memset((char*)&rcrd2, 0, sizeof(rcrd2));
    rcrd2.len = max_key - min_key + 1;
    
    return rcrd_split(key_p, rcrd_p,
                      min_key_p,
                      (struct Oc_xt_rcrd*)&rcrd2,
                      key_array_p,
                      rcrd_array_p);
}

/* return the end offset of an extent. The end offset is part
 * of the extent. For example, if E=[10-13] then 13 is the end-offset
 * and it is still -in- E.
 */
static void rcrd_end_offset(struct Oc_xt_key *_key_p,
                            struct Oc_xt_rcrd *_rcrd_p,
                            
                            // returned key 
                            struct Oc_xt_key *_end_key_po)
{
    uint32 key = *(uint32*)_key_p;
    uint32 *key_po = (uint32*)_end_key_po;
    Oc_xt_test_rcrd *rcrd_p = (Oc_xt_test_rcrd*) _rcrd_p;

    oc_utl_debugassert(rcrd_p->len > 0);
    (*key_po) = key + rcrd_p->len - 1;
}


static uint64 rcrd_length(struct Oc_xt_key *key_p, struct Oc_xt_rcrd *rcrd_pi)
{
    Oc_xt_test_rcrd *rcrd_p = (Oc_xt_test_rcrd*) rcrd_pi;

    return rcrd_p->len;
}

static void rcrd_release(Oc_wu *wu_p,
                         struct Oc_xt_key *_key_p,
                         struct Oc_xt_rcrd *_rcrd_p)
{
    struct Oc_xt_test_fs_ctx *fs_p;
    Oc_xt_test_rcrd *rcrd_p = (Oc_xt_test_rcrd *) _rcrd_p;

    switch (rcrd_p->fs_impl) {
    case FS_REAL:
        if (wu_p->po_id != 0)
            /* For the real implementation we try to test the 
             * multi-threading aspect.
             */
            if (oc_xt_test_utl_random_number(3) == 0)
                oc_crt_yield_task(); 
        
        fs_p = fs_ctx_p;
        break;
    case FS_ALT:
        fs_p = fs_ctx_alt_p;
        break;
    default:
        ERR(("sanity, fs-class=%d", rcrd_p->fs_impl));
    }
    
    oc_xt_test_fs_dealloc(fs_p, rcrd_p->data, rcrd_p->len);
}

static void rcrd_to_string(struct Oc_xt_key *key_pi,
                           struct Oc_xt_rcrd *rcrd_pi,
                           char *str_p,
                           int max_len)
{
    uint32 key = *((uint32*)key_pi);
    Oc_xt_test_rcrd *rcrd_p = (Oc_xt_test_rcrd*) rcrd_pi;
    
    if (max_len < 15)
        ERR(("data_to_string: %d is not enough", max_len));
    
    sprintf(str_p, "%lu-%lu", key, key + rcrd_p->len -1);
}

static void fs_query_alloc( struct Oc_rm_resource *r_p, int n_pages)
{
    return;
}

static void fs_query_dealloc( struct Oc_rm_resource *r_p, int n_pages)
{
    return;
}

void oc_xt_test_utl_finalize(int refcnt)
{
    oc_utl_assert(refcnt == oc_xt_test_nd_get_refcount());
}

/******************************************************************/
void oc_xt_test_utl_setup_wu(Oc_wu *wu_p, Oc_rm_ticket *rm_p)
{
    static int po_id =0;
    
    memset(wu_p, 0, sizeof(Oc_wu));
    memset(rm_p, 0, sizeof(Oc_rm_ticket));
    wu_p->po_id = po_id++;
    wu_p->rm_p = rm_p;
}

void oc_xt_test_utl_alt_print(void)
{
    printf("\n // -------------------------------------------------\n");
    printf("// Linked-list:\n");
    oc_xt_alt_dbg_output_b(&utl_wu, &alt_state);
    printf("\n // -------------------------------------------------\n");
}

void oc_xt_test_utl_init(struct Oc_wu *wu_p)
{
    oc_xt_init_state_b(NULL, &state, &cfg);
    oc_xt_alt_init_state_b(NULL, &alt_state, &cfg);
}

void oc_xt_test_utl_create(Oc_wu *wu_p)
{
    if (verbose) {
        printf("// btree create\n");
        fflush(stdout);
    }
    oc_xt_create_b(wu_p, &state);
    oc_xt_alt_create_b(wu_p, &alt_state);
}

void oc_xt_test_utl_display(bool with_linked_list)
{
    char tag[10];
    static int cnt =0;

    cnt++;
    sprintf(tag, "Tree_%d", cnt);
    oc_xt_dbg_output_b(&utl_wu, &state, (struct Oc_utl_file*)stdout, tag);
    
    if (with_linked_list) {
        printf("// Linked-list:\n");
        oc_xt_alt_dbg_output_b(&utl_wu, &alt_state);

        if (!oc_xt_alt_dbg_validate_b(&utl_wu, &alt_state))
            ERR(("bad list"));
    }
    
    fflush(stdout);
}
    
bool oc_xt_test_utl_validate(void)
{
    bool rc1, rc2, rc3;
    
    rc1 = oc_xt_dbg_validate_b(&utl_wu, &state);
    rc2 = oc_xt_alt_dbg_validate_b(&utl_wu, &alt_state);
    rc3 = oc_xt_test_fs_compare(fs_ctx_alt_p, fs_ctx_p);
    
    return rc1 && rc2 && rc3;
}

void oc_xt_test_utl_statistics(void)
{
    oc_xt_statistics_b(&utl_wu, &state); 
}

void oc_xt_test_utl_delete(Oc_wu *wu_p)
{
    if (verbose) {
        printf("// btree delete\n");
        fflush(stdout);
    }
    oc_xt_delete_b(wu_p, &state);
    oc_xt_alt_delete_b(wu_p, &alt_state);
}

// create a random number between 0 and [top]
uint32 oc_xt_test_utl_random_number(uint32 top)
{
    if (0 == top) return 0;
    return (uint32) (rand() % top);
}

/* Compare between two extent arrays, check if they are
 * equivalent.
 *
 * if [allow_partial_match] is TRUE then allow the case
 * where the array-A is a sub-match for array-B.
 */
static bool ext_array_compare(
    int nkeys_found_A,
    struct Oc_xt_key *key_array_A,
    struct Oc_xt_rcrd *rcrd_array_A, 
    int nkeys_found_B,
    struct Oc_xt_key *key_array_B,
    struct Oc_xt_rcrd *rcrd_array_B,
    bool allow_partial_match)
{
    int i,j,k;
    Oc_xt_cmp rc;
    struct Oc_xt_key *res_key_array;
    struct Oc_xt_rcrd *res_rcrd_array;
    struct Oc_xt_key *res_key_array_p[3];
    struct Oc_xt_rcrd *res_rcrd_array_p[3];
    struct Oc_xt_key *keyA, *keyB;
    struct Oc_xt_rcrd *rcrdA, *rcrdB;
    uint64 len;

    // corner case: there are no extents
    if (nkeys_found_B == nkeys_found_A &&
        nkeys_found_A == 0)
        return TRUE;

    // an array to store split results
    res_key_array = (struct Oc_xt_key *)alloca(state.cfg_p->key_size * 3);
    res_rcrd_array = (struct Oc_xt_rcrd *)alloca(state.cfg_p->rcrd_size * 3);

    keyA = (struct Oc_xt_key*) alloca(state.cfg_p->key_size);
    keyB = (struct Oc_xt_key*) alloca(state.cfg_p->key_size);    
    rcrdA = (struct Oc_xt_rcrd*) alloca(state.cfg_p->rcrd_size);
    rcrdB = (struct Oc_xt_rcrd*) alloca(state.cfg_p->rcrd_size);   
    copy_ext( keyA, rcrdA,
              key_array_kth(&state, key_array_A, 0),
              rcrd_array_kth(&state, rcrd_array_A, 0));
    copy_ext( keyB, rcrdB,
              key_array_kth(&state, key_array_B,0),
              rcrd_array_kth(&state, rcrd_array_B,0));
    
    i=0, j=0;
    while (1) 
    {
        oc_utl_assert (i <= nkeys_found_A);
        oc_utl_assert (j <= nkeys_found_B);
                
        // setup a place to receive extent-split results
        for (k=0; k<3; k++) {
            res_key_array_p[k] = key_array_kth(&state, res_key_array,k);
            res_rcrd_array_p[k] =rcrd_array_kth(&state, res_rcrd_array,k);
        }
        
        rc = rcrd_split(
            // record+key to chop
            keyA, rcrdA,

            // record to provide boundary keys
            keyB, rcrdB,
            
            // resulting sub-extents
            res_key_array_p,
            res_rcrd_array_p);
        
        //printf("comparison, rc=%s\n", oc_xt_string_of_cmp_rc(rc));
        //fflush(stdout);
        
        switch (rc) {
        case OC_XT_CMP_SML:
        case OC_XT_CMP_GRT:
            return FALSE;

        case OC_XT_CMP_EQUAL:
            i++;
            j++;
            if (i == nkeys_found_A && j == nkeys_found_B) {
                // we are done
                return TRUE;
            }
            else if (i < nkeys_found_A && j < nkeys_found_B) {
                copy_ext( keyA, rcrdA,
                          key_array_kth(&state, key_array_A, i),
                          rcrd_array_kth(&state, rcrd_array_A, i));
                copy_ext( keyB, rcrdB,
                          key_array_kth(&state, key_array_B, j),
                          rcrd_array_kth(&state, rcrd_array_B, j));
            }
            else {
                // the extent-arrays only partially match
                if (allow_partial_match &&
                    i == nkeys_found_A &&
                    j < nkeys_found_B)
                    return TRUE;
                else
                    return FALSE;
            }
            break;

        case OC_XT_CMP_COVERED:
            len = state.cfg_p->rcrd_length(keyA, rcrdA);
            state.cfg_p->rcrd_chop_length(keyB, rcrdB, len);
            i++;
            if (i == nkeys_found_A) {
                if (allow_partial_match)
                    return TRUE;
                else
                    return FALSE;
            }
            copy_ext( keyA, rcrdA,
                      key_array_kth(&state, key_array_A, i),
                      rcrd_array_kth(&state, rcrd_array_A, i));
            break;
            
        case OC_XT_CMP_FULLY_COVERS:
            /* Remove the beginning of extent1 and continue with rest
             * of it.
             */
            if (res_key_array_p[0] != NULL)
                return FALSE;
            copy_ext(keyA, rcrdA, res_key_array_p[2], res_rcrd_array_p[2]);
            j++;
            if (j == nkeys_found_B)
                return FALSE;
            copy_ext( keyB, rcrdB,
                      key_array_kth(&state, key_array_B, j),
                      rcrd_array_kth(&state, rcrd_array_B, j));
            break;
            
        case OC_XT_CMP_PART_OVERLAP_SML:
        case OC_XT_CMP_PART_OVERLAP_GRT:
            return FALSE;

        default:
            ERR(("sanity"));
        }
    }

    return TRUE;
}

void oc_xt_test_utl_lookup_range(Oc_wu *wu_p,
                                 uint32 lo_key,
                                 uint32 hi_key,
                                 bool check )
{
    int j;
    uint32 key_array1[30], key_array2[30];
    Oc_xt_test_rcrd rcrd_array1[30], rcrd_array2[30];
    int nkeys_found1, nkeys_found2;
    bool allow_sub_match = FALSE;
    
    total_ops++;
    if (verbose) {
        printf("// lookup_range [lo_key=%lu, hi_key=%lu]\n", lo_key, hi_key);
        fflush(stdout);
    }
    
    oc_xt_lookup_range_b(
        wu_p, &state,
        (struct Oc_xt_key*)&lo_key,
        (struct Oc_xt_key*)&hi_key,
        30,
        (struct Oc_xt_key*)key_array1,
        (struct Oc_xt_rcrd*)rcrd_array1, 
        &nkeys_found1);
    
    if (!check) return;
    
    oc_xt_alt_lookup_range_b(
        wu_p, &alt_state,
        (struct Oc_xt_key*)&lo_key,
        (struct Oc_xt_key*)&hi_key,
        30,
        (struct Oc_xt_key*)key_array2,
        (struct Oc_xt_rcrd*)rcrd_array2, 
        &nkeys_found2);

    if (30 == nkeys_found1)
        allow_sub_match = TRUE;
    if (ext_array_compare(
            nkeys_found1,
            (struct Oc_xt_key*)key_array1, (struct Oc_xt_rcrd*)rcrd_array1, 
            nkeys_found2,
            (struct Oc_xt_key*)key_array2, (struct Oc_xt_rcrd*)rcrd_array2,
            allow_sub_match))
        return;

    // error case
    for (j=0; j<nkeys_found1; j++) {
        char s[30];

        state.cfg_p->rcrd_to_string((struct Oc_xt_key*) &key_array1[j],
                                    (struct Oc_xt_rcrd*) &rcrd_array1[j],
                                    s, 30);
        printf("  // xt] ext=%s \n", s);
    }

    for (j=0; j<nkeys_found2; j++) {
        char s[30];

        state.cfg_p->rcrd_to_string((struct Oc_xt_key*) &key_array2[j],
                                    (struct Oc_xt_rcrd*) &rcrd_array2[j],
                                    s, 30);        
        printf("  // alt] ext=%s\n", s);
    }
    
    oc_xt_test_utl_display(TRUE);
    oc_xt_dbg_output_end((struct Oc_utl_file*)stdout);
    printf("#entries found btree = %d\n", nkeys_found1);
    printf("#entries found linked-list = %d\n", nkeys_found2);
    
    ERR(("mismatch in lookup_range(%lu, %lu)", lo_key, hi_key));

    if (verbose) oc_xt_test_utl_display(FALSE);
}

void oc_xt_test_utl_insert( Oc_wu *wu_p,
                            uint32 key,
                            uint32 length,
                            bool check)
{
    uint64 rc1, rc2;
    Oc_xt_test_rcrd rcrd1, rcrd2;
    
    // we handle limited sized arrays
    total_ops++;
    
    oc_utl_assert(length > 0);
    if (verbose) {
        printf("// insert_range key=%lu len=%lu\n", key, length);
        fflush(stdout);
    }
    
    rcrd1.len = length;
    rcrd1.data = oc_xt_test_fs_alloc(fs_ctx_p, length);
    rcrd1.fs_impl = FS_REAL;
    rc1 = oc_xt_insert_range_b(wu_p, &state, 
                               (struct Oc_xt_key*)&key,
                               (struct Oc_xt_rcrd*)&rcrd1); 

    rcrd2.len = length;
    rcrd2.data = oc_xt_test_fs_alloc(fs_ctx_alt_p, length);
    rcrd2.fs_impl = FS_ALT;
    rc2 = oc_xt_alt_insert_b(wu_p, &alt_state, 
                             (struct Oc_xt_key*)&key,
                             (struct Oc_xt_rcrd*)&rcrd2);
    
    if (check) {
        if (!oc_xt_test_utl_validate()) {
            printf("    // invalid b-tree\n");
            oc_xt_test_utl_display(TRUE);
            oc_xt_dbg_output_end( (struct Oc_utl_file*)stdout );
            exit(1);
        }
        if (rc1 != rc2) {
            oc_xt_test_utl_display(TRUE);
            oc_xt_dbg_output_end( (struct Oc_utl_file*)stdout );
            ERR(("mismatch in insert_range key=%lu len=%lu, rc1=%Lu rc2=%Lu\n",
                 key, length, rc1, rc2));
        }
    }
    
    if (verbose) oc_xt_test_utl_display(FALSE);
}

void oc_xt_test_utl_remove_range(Oc_wu *wu_p, uint32 lo_key, uint32 hi_key,
                                 bool check)
{
    int rc1, rc2;
    
    total_ops++;
    if (verbose) {
        printf("// remove_range lo_key=%lu hi_key=%lu\n", lo_key, hi_key);
        fflush(stdout);
    }
    
    rc1 = oc_xt_remove_range_b(wu_p, &state,
                               (struct Oc_xt_key*)&lo_key, 
                               (struct Oc_xt_key*)&hi_key); 
    rc2 = oc_xt_alt_remove_range_b(wu_p, &alt_state, 
                                   (struct Oc_xt_key*)&lo_key, 
                                   (struct Oc_xt_key*)&hi_key);
    
    if (check) {
        if (!oc_xt_test_utl_validate()) {
            printf("    // invalid b-tree\n");
            oc_xt_test_utl_display(TRUE);
            oc_xt_dbg_output_end((struct Oc_utl_file*)stdout);
            exit(1);
        }
        if (rc1 != rc2) {
            oc_xt_test_utl_display(TRUE);
            oc_xt_dbg_output_end((struct Oc_utl_file*)stdout);
            ERR(("mismatch in remove_range lo_key=%lu hi_key=%lu",lo_key, hi_key));
        }
    }
    
    if (verbose) oc_xt_test_utl_display(FALSE);
}

void oc_xt_test_utl_compare_and_verify(Oc_wu *wu_p, int max_int)
{
    // TODO: need to reimplement this
#if 0
    int i;
    
    if (verbose) {
        printf("// compare and verify\n");
        fflush(stdout);
    }

    for (i=0; i<max_int; i++) {
        oc_xt_test_utl_lookup(wu_p, i, TRUE);
    }
#endif
}

/**********************************************************************/
void oc_xt_test_utl_init_module(void)
{
    oc_xt_test_utl_setup_wu(&utl_wu, &utl_rm_ticket);
    if (utl_wu.po_id != 0)
        ERR(("the initial po_id has to be zero"));

    // setup the node functions
    oc_xt_test_nd_init();
    
    // setup the free-space instances
    fs_ctx_p = oc_xt_test_fs_create("real", verbose);
    fs_ctx_alt_p = oc_xt_test_fs_create("alt", verbose);

    oc_xt_init();
    
    // setup 
    memset(&cfg, 0, sizeof(cfg));
    cfg.key_size = sizeof(Oc_xt_test_key);
    cfg.rcrd_size = sizeof(Oc_xt_test_rcrd);
    cfg.node_size = OC_XT_TEST_ND_SIZE;
    cfg.root_fanout = max_root_fanout;
    cfg.non_root_fanout = max_non_root_fanout;
    cfg.min_num_ent = min_fanout;
    cfg.node_alloc = oc_xt_test_nd_alloc;
    cfg.node_dealloc = oc_xt_test_nd_dealloc;
    cfg.node_get = oc_xt_test_nd_get;
    cfg.node_release = oc_xt_test_nd_release;
    cfg.node_mark_dirty = oc_xt_test_nd_mark_dirty;
    cfg.key_compare = key_compare;
    cfg.key_inc = key_inc;
    cfg.key_to_string = key_to_string;
    cfg.rcrd_compare = rcrd_compare;
    cfg.rcrd_compare0 = rcrd_compare0;
    cfg.rcrd_bound_split = rcrd_bound_split;
    cfg.rcrd_split = rcrd_split;
    cfg.rcrd_chop_length = rcrd_chop_length;
    cfg.rcrd_chop_top = rcrd_chop_top;
    cfg.rcrd_end_offset = rcrd_end_offset;
    cfg.rcrd_split_into_sub = rcrd_split_into_sub;
    cfg.rcrd_length = rcrd_length;
    cfg.rcrd_release = rcrd_release;
    cfg.rcrd_to_string = rcrd_to_string;
    cfg.fs_query_alloc = fs_query_alloc;
    cfg.fs_query_dealloc = fs_query_dealloc;
        
    oc_xt_init_config(&cfg);
}

bool oc_xt_test_utl_parse_cmd_line(int argc, char *argv[])
{
    int i = 1;

    for (i=1;i<argc;i++) {
        if (strcmp(argv[i], "-trace") == 0) {
	    if (++i >= argc)
                return FALSE;
            pl_trace_base_add_string_tag_full(argv[i]);
        }
        else if (strcmp(argv[i], "-trace_level") == 0) {
	    if (++i >= argc)
                return FALSE;
            pl_trace_base_set_level(atoi(argv[i]));
        }
        else if (strcmp(argv[i], "-trace_tags") == 0) {
            pl_trace_base_print_tag_list();
            exit(0);
        }
        else if (strcmp(argv[i], "-verbose") == 0) {
            verbose = TRUE;
        }
        else if (strcmp(argv[i], "-stat") == 0) {
            statistics = TRUE;
        }       
        else if (strcmp(argv[i], "-max_int") == 0) {
	    if (++i >= argc)
                return FALSE;
            max_int = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-num_rounds") == 0) {
	    if (++i >= argc)
                return FALSE;
            num_rounds = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-num_tasks") == 0) {
	    if (++i >= argc)
                return FALSE;
            num_tasks = atoi(argv[i]);
        }        
        else if (strcmp(argv[i], "-max_root_fanout") == 0) {
	    if (++i >= argc)
                return FALSE;
            max_root_fanout = atoi(argv[i]);
        }        
        else if (strcmp(argv[i], "-max_non_root_fanout") == 0) {
	    if (++i >= argc)
                return FALSE;
            max_non_root_fanout = atoi(argv[i]);
        }        
        else if (strcmp(argv[i], "-min_fanout") == 0) {
	    if (++i >= argc)
                return FALSE;
            min_fanout = atoi(argv[i]);
        }        
        else if (strcmp(argv[i], "-test") == 0) {
	    if (++i >= argc)
                return FALSE;
            if (strcmp(argv[i], "large_trees") == 0)
                test_type = OC_XT_TEST_UTL_LARGE_TREES;
            else if (strcmp(argv[i], "small_trees") == 0)
                test_type = OC_XT_TEST_UTL_SMALL_TREES;
            else if (strcmp(argv[i], "small_trees_w_ranges") == 0)
                test_type = OC_XT_TEST_UTL_SMALL_TREES_W_RANGES;
            else if (strcmp(argv[i], "small_trees_mixed") == 0)
                test_type = OC_XT_TEST_UTL_SMALL_TREES_MIXED;
            else
                ERR(("no such test. valid tests={large_trees,small_trees,small_trees_w_ranges,small_trees_mixed}"));
        } 
        else
            return FALSE;
    }
    
    return TRUE;
}

void oc_xt_test_utl_help_msg (void)
{
    printf("Usage: oc_xt_test\n");
    printf("\t -max_int <range of keys is [0 ... max_int]>  [default=%d]\n",
           max_int);
    printf("\t -num_rounds <number of rounds to execute>  [default=%d]\n",
           num_rounds);
    printf("\t -num_tasks <number of concurrent tasks>  [default=%d]\n",
           num_tasks); 
    printf("\t -max_root_fanout <maximal number of entries in a root node>  [default=%d]\n",
           max_root_fanout);
    printf("\t -max_non_root_fanout <maximal number of entries in a non-root node>  [default=%d]\n",
           max_non_root_fanout);
    printf("\t -min_fanout <minimal number of entries in a node>  [default=%d]\n",
           min_fanout);
    printf("\t -verbose\n");
    printf("\t -stat\n");
    printf("\t -test <small_trees|large_trees|small_trees_w_ranges|small_trees_mixed>\n");    
    exit(1);
}


/**********************************************************************/
