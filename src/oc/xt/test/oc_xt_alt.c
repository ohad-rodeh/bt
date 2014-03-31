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
/******************************************************************/
/* OC_XT_ALT.C
 * An alternate implementation of an x-tree. Uses a linked-list.
 */
/******************************************************************/
#include <string.h>
#include <malloc.h>
#include <alloca.h>

#include "oc_xt_alt.h"
/******************************************************************/
// An entry includes a key-record pair
typedef struct Oc_xt_alt_ext {
    Ss_dlist_node link;
    struct Oc_xt_key *key_p;
    struct Oc_xt_rcrd *rcrd_p;
} Oc_xt_alt_ext;

/******************************************************************/
// utility functions

void *wrap_malloc(int size)
{
    void *p;
    
    p= malloc(size);
    if (NULL == p) ERR(("out of memory"));
    return p;
}

static Oc_xt_alt_ext *ext_alloc(
    struct Oc_xt_alt_state *s_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p)
{
    Oc_xt_alt_ext *ext_p;

    ext_p = (Oc_xt_alt_ext*) wrap_malloc(sizeof(Oc_xt_alt_ext));
    memset(ext_p, 0, sizeof(Oc_xt_alt_ext));
    
    ext_p->key_p = (struct Oc_xt_key*) wrap_malloc(s_p->cfg_p->key_size);
    memcpy(ext_p->key_p, key_p, s_p->cfg_p->key_size);
    
    ext_p->rcrd_p = (struct Oc_xt_rcrd*) wrap_malloc(s_p->cfg_p->rcrd_size);
    memcpy(ext_p->rcrd_p, rcrd_p, s_p->cfg_p->rcrd_size);
    
    return ext_p;
}

static void ext_free(Oc_xt_alt_ext *ext_p)
{
    free(ext_p->key_p);
    free(ext_p->rcrd_p);
    free(ext_p);
}

void ext_print(struct Oc_xt_alt_state *s_p,
               Oc_xt_alt_ext *ext_p)
{
    char s[40];

    s_p->cfg_p->rcrd_to_string(ext_p->key_p, ext_p->rcrd_p, s, 40);        

    printf("ext=%s\n", s);
}

/******************************************************************/

void oc_xt_alt_init_state_b(
    struct Oc_wu *wu_pi,
    struct Oc_xt_alt_state *s_p,
    Oc_xt_cfg *cfg_p)
{
    memset(s_p, 0, sizeof(Oc_xt_alt_state));
    s_p->cfg_p = cfg_p;
}

void oc_xt_alt_create_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_alt_state *s_p)
{
    // do nothing;
}

void oc_xt_alt_delete_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_alt_state *s_p)
{
    Oc_xt_alt_ext *tmp_p;

    while (!ssdlist_empty(&s_p->list))
    {
        tmp_p = (Oc_xt_alt_ext*)ssdlist_remove_head(&s_p->list);
        s_p->cfg_p->rcrd_release(wu_p, tmp_p->key_p, tmp_p->rcrd_p);
        ext_free(tmp_p);
    }

    ssdlist_init(&s_p->list);
}

/******************************************************************/
bool oc_xt_alt_dbg_validate_b(struct Oc_wu *wu_p,
                           Oc_xt_alt_state *s_p)
{
    Oc_xt_alt_ext *crnt_p, *prev_p;
    
//    printf("// alt_validate len=%d\n", ssdlist_get_length(&s_p->list));
    if (ssdlist_get_length(&s_p->list) == 0)
        return TRUE;

    prev_p = (Oc_xt_alt_ext*)ssdlist_head(&s_p->list);

    if (ssdlist_get_length(&s_p->list) == 1) {
        if (NULL == prev_p->key_p ||
            NULL == prev_p->rcrd_p)
            return FALSE;
    }


    crnt_p = (Oc_xt_alt_ext*)ssdlist_next(&prev_p->link);
    while (crnt_p != NULL)
    {
        if (NULL == crnt_p->key_p ||
            NULL == crnt_p->rcrd_p)
            return FALSE;

        if (s_p->cfg_p->rcrd_compare(
                prev_p->key_p, prev_p->rcrd_p,
                crnt_p->key_p, crnt_p->rcrd_p) != OC_XT_CMP_SML)
            return FALSE;
        
        prev_p = crnt_p;
        crnt_p = (Oc_xt_alt_ext*)ssdlist_next(&prev_p->link);
    }

    return TRUE;
}
/******************************************************************/
void oc_xt_alt_dbg_output_b(struct Oc_wu *wu_p,
                             Oc_xt_alt_state *s_p)
{
    char s[40];
    Oc_xt_alt_ext *crnt_p;

//    printf("// %s   ", (oc_xt_alt_validate_b(wu_p, s_p) ? "VALID" : "INVALID" ));
    printf("// list=");
    for (crnt_p = (Oc_xt_alt_ext*)ssdlist_head(&s_p->list);
         crnt_p != NULL;
         crnt_p = (Oc_xt_alt_ext*)ssdlist_next(&crnt_p->link))
    {
        s_p->cfg_p->rcrd_to_string(crnt_p->key_p, crnt_p->rcrd_p, s, 40);
        
        printf("(rcrd=%s) ", s);
    }
    printf("\n ");
}

/******************************************************************/

static inline struct Oc_xt_key* key_array_kth(
    struct Oc_xt_alt_state *s_p,
    struct Oc_xt_key* key_array,
    int k)
{
    return (struct Oc_xt_key*) ((char*)key_array + s_p->cfg_p->key_size * k);
}

static inline struct Oc_xt_rcrd* rcrd_array_kth(
    struct Oc_xt_alt_state *s_p,
    struct Oc_xt_rcrd* rcrd_array,
    int k)
{
    return (struct Oc_xt_rcrd*) ((char*)rcrd_array + s_p->cfg_p->rcrd_size * k);
}

// lookup a range of extents
void oc_xt_alt_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_alt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    int max_num_keys_i,
    struct Oc_xt_key *key_array_po,
    struct Oc_xt_rcrd *rcrd_array_po,
    int *n_found_po)
{
    Oc_xt_alt_ext *crnt_p;
    int i, n_found;
    struct Oc_xt_key *res_key_array_p[3];
    struct Oc_xt_rcrd *res_rcrd_array_p[3];

    if (s_p->cfg_p->key_compare(min_key_p, max_key_p) == -1) {
        // the range does not make sense
        *n_found_po = 0;
        return;  
    }
    
    n_found = 0;
    for (crnt_p = (Oc_xt_alt_ext*)ssdlist_head(&s_p->list);
         crnt_p != NULL && n_found < max_num_keys_i;
         crnt_p = (Oc_xt_alt_ext*)ssdlist_next(&crnt_p->link))
    {
        // setup result array. All we are interested in is the intersection
        // between the range and this extent
        for (i=0; i<3; i++) {
            res_key_array_p[i] = NULL;
            res_rcrd_array_p[i] = NULL;
        }
        res_key_array_p[1] = key_array_kth(s_p, key_array_po, n_found);
        res_rcrd_array_p[1] = rcrd_array_kth(s_p, rcrd_array_po, n_found);
            
        // check if the extent is inside the range
        s_p->cfg_p->rcrd_bound_split(
            crnt_p->key_p,
            crnt_p->rcrd_p,
            
            // boundary keys
            min_key_p, 
            max_key_p,
            
            // store results in the result array
            res_key_array_p,
            res_rcrd_array_p);
        
        if (res_key_array_p[1] != NULL) {
            // There is a non-empty intersection. The sub-extent has been copied
            // into the result array.
            n_found++;
        }
    }

    *n_found_po = n_found;
}


/******************************************************************/

// remove a range 
uint64 oc_xt_alt_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_alt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p)
{
    Oc_xt_alt_ext *crnt_p, *next_p, *before_p, *after_p;
    uint64 tot_len = 0;
    struct Oc_xt_key *res_key_ap[3];
    struct Oc_xt_rcrd *res_rcrd_ap[3];
    struct Oc_xt_key *res_key_a;
    struct Oc_xt_rcrd *res_rcrd_a;
    Oc_xt_cmp rc;
    int i;
    
    // temporary space to store results
    res_key_a = (struct Oc_xt_key *) alloca(s_p->cfg_p->key_size * 3);
    res_rcrd_a = (struct Oc_xt_rcrd *) alloca(s_p->cfg_p->rcrd_size * 3);

    for (crnt_p = (Oc_xt_alt_ext*)ssdlist_head(&s_p->list);
         crnt_p != NULL;
         /* */ )
    {
        // setup the result pointers. This needs to be done each iteration because
        // the [rcrd_bound_split] call modifies the pointers. 
        for (i=0; i<3; i++) {
            res_key_ap[i] = key_array_kth(s_p, res_key_a, i);
            res_rcrd_ap[i] = rcrd_array_kth(s_p, res_rcrd_a, i);
        }
        
        // check if the extent is inside the range
        rc = s_p->cfg_p->rcrd_bound_split(
            crnt_p->key_p,
            crnt_p->rcrd_p,
            
            // boundary keys
            min_key_p, 
            max_key_p,
            
            // store results in the result array
            res_key_ap,
            res_rcrd_ap);

        // release the storage in the middle extent
        if (res_key_ap[1]) {
            tot_len += s_p->cfg_p->rcrd_length(res_key_ap[1], res_rcrd_ap[1]);
            s_p->cfg_p->rcrd_release(wu_p, res_key_ap[1], res_rcrd_ap[1]);
        }

        switch (rc) {
        case OC_XT_CMP_SML:
            // haven't gotten to the area to remove yet. 
            crnt_p = (Oc_xt_alt_ext *)ssdlist_next(&crnt_p->link);
            break;

        case OC_XT_CMP_GRT:
            // we are done. 
            return tot_len;

        case OC_XT_CMP_EQUAL:
            // remove this extent and return 
            ssdlist_remove(&s_p->list, &crnt_p->link);
            ext_free(crnt_p);
            return tot_len;
            
        case OC_XT_CMP_COVERED:
            // [crnt_p] is completely within the boundaries, remove it
            next_p = (Oc_xt_alt_ext *)ssdlist_next(&crnt_p->link);
            ssdlist_remove(&s_p->list, &crnt_p->link);
            ext_free(crnt_p);
            crnt_p = next_p;
            break;

        case OC_XT_CMP_FULLY_COVERS:
            // The area to remove is completely covered by [crnt_p].
            // remove the middle of [crnt_p] and return.

            // add the before + after extents in-front of [crnt_p]
            if (res_key_ap[0]) {
                before_p = ext_alloc(s_p, res_key_ap[0], res_rcrd_ap[0]); 
                ssdlist_add_before(&s_p->list, &crnt_p->link, &before_p->link);
            }
            if (res_key_ap[2]) {
                after_p = ext_alloc(s_p, res_key_ap[2], res_rcrd_ap[2]); 
                ssdlist_add_before(&s_p->list, &crnt_p->link, &after_p->link);
            }
            
            // remove crnt_p from the list
            next_p = (Oc_xt_alt_ext *)ssdlist_next(&crnt_p->link);
            ssdlist_remove(&s_p->list, &crnt_p->link);
            ext_free(crnt_p);
            crnt_p = next_p;
            
            // we are done
            return tot_len;

        case OC_XT_CMP_PART_OVERLAP_SML:
            // The current extent is split into E1,E2 where E2 needs to be removed
            
            // add the before extent in-front of [crnt_p]
            before_p = ext_alloc(s_p, res_key_ap[0], res_rcrd_ap[0]);
            ssdlist_add_before(&s_p->list, &crnt_p->link, &before_p->link);            

            // remove crnt_p from the list
            ssdlist_remove(&s_p->list, &crnt_p->link);
            ext_free(crnt_p);
            crnt_p = (Oc_xt_alt_ext *)ssdlist_next(&before_p->link);

            break;
            
        case OC_XT_CMP_PART_OVERLAP_GRT:
            // The current extent is split into E2,E3 where E2 needs to be removed
            
            // add the after extent in-front of [crnt_p]
            after_p = ext_alloc(s_p, res_key_ap[2], res_rcrd_ap[2]); 
            ssdlist_add_before(&s_p->list, &crnt_p->link, &after_p->link);            

            // remove crnt_p from the list
            ssdlist_remove(&s_p->list, &crnt_p->link);
            ext_free(crnt_p);
            crnt_p = (Oc_xt_alt_ext *)ssdlist_next(&after_p->link);
            break;

        default:
            ERR(("sanity"));
        }
    }
    
    return tot_len;
}
    
/******************************************************************/
    
uint64 oc_xt_alt_insert_b(
    struct Oc_wu *wu_p,
    Oc_xt_alt_state *s_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p)
{
    struct Oc_xt_key *end_key_p;
    uint64 tot_len = 0;
    Oc_xt_alt_ext *e_p, *ext_p, *post_p = NULL; 
    
    // remove the range between the start of the extent and the end of it
    end_key_p = (struct Oc_xt_key*) alloca(s_p->cfg_p->key_size);
    s_p->cfg_p->rcrd_end_offset( key_p, rcrd_p, end_key_p);

    tot_len = oc_xt_alt_remove_range_b( wu_p, s_p, key_p, end_key_p);
    
    // find out where to insert the set of extents
    // iterate until you find the first extent that is larger than [fst_p]
    for (e_p = (Oc_xt_alt_ext*)ssdlist_head(&s_p->list);
         e_p != NULL;
         e_p = (Oc_xt_alt_ext*)ssdlist_next(&e_p->link))
    {
        Oc_xt_cmp rc = s_p->cfg_p->rcrd_compare(key_p, rcrd_p,
                                                e_p->key_p, e_p->rcrd_p);

        switch (rc) {
        case OC_XT_CMP_SML:
            // N is smaller than E, 
            // insert it before E, we are done.
            post_p = e_p;
            goto insert;
            
        case OC_XT_CMP_GRT:
            // N is strictly larger than E, continue searching
            break;
            
        default:
        {
            char s1[30];
            char s2[30];
            char s3[30];
            
            s_p->cfg_p->rcrd_to_string(key_p, rcrd_p, s1, 30);
            s_p->cfg_p->rcrd_to_string(e_p->key_p, e_p->rcrd_p, s2, 30);
            s_p->cfg_p->key_to_string(end_key_p, s3, 30);

            printf("sanity, insert ext=[%s], intersection with [%s] rc=%s end_ofs=%s\n",
                   s1, s2, oc_xt_string_of_cmp_rc(rc), s3);

            oc_xt_alt_dbg_output_b(wu_p, s_p);
            assert(0);
        }
        }
    }
        
    insert:
    if (NULL == post_p) {
        // Reached the end of the list. Insert at the tail
        ext_p = ext_alloc(s_p, key_p, rcrd_p);
        ssdlist_add_tail(&s_p->list, &ext_p->link);
    }
    else {
        // insert before [post_p]
        ext_p = ext_alloc(s_p, key_p, rcrd_p);
        ssdlist_add_before(&s_p->list, &post_p->link, &ext_p->link);
    }
    
    return tot_len;
}

