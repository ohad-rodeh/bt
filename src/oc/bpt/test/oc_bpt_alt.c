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
// OC_BPT_ALT.C
// An alternate implementation of a b-tree. Uses a linked-list.
/******************************************************************/
#include <string.h>
#include <malloc.h>

#include "oc_utl.h"
#include "oc_bpt_alt.h"
/******************************************************************/
// A record includes a key-data pair
typedef struct Oc_bpt_alt_rec {
    Ss_dlist_node link;
    struct Oc_bpt_key *key_p;
    struct Oc_bpt_data *data_p;
} Oc_bpt_alt_rec;

/******************************************************************/
// utility functions

void *wrap_malloc(int size)
{
    void *p;
    
    p= malloc(size);
    if (NULL == p) ERR(("out of memory"));
    return p;
}

static Oc_bpt_alt_rec *rec_alloc(
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p)
{
    Oc_bpt_alt_rec *rec_p;
    
    rec_p = (Oc_bpt_alt_rec*) wrap_malloc(sizeof(Oc_bpt_alt_rec));
    memset(rec_p, 0, sizeof(Oc_bpt_alt_rec));
    
    rec_p->key_p = (struct Oc_bpt_key*) wrap_malloc(s_p->cfg_p->key_size);
    memcpy(rec_p->key_p, key_p, s_p->cfg_p->key_size);
    
    rec_p->data_p = (struct Oc_bpt_data*) wrap_malloc(s_p->cfg_p->data_size);
    memcpy(rec_p->data_p, data_p, s_p->cfg_p->data_size);
    
    return rec_p;
}

static void rec_free(Oc_bpt_alt_rec *rec_p)
{
    free(rec_p->key_p);
    free(rec_p->data_p);
    free(rec_p);
}
    
/******************************************************************/

void oc_bpt_alt_init_state_b(
    struct Oc_wu *wu_pi,
    struct Oc_bpt_alt_state *s_p,
    Oc_bpt_alt_cfg *cfg_p)
{
    memset(s_p, 0, sizeof(Oc_bpt_alt_state));
    s_p->cfg_p = cfg_p;
}

void oc_bpt_alt_create_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p)
{
    // do nothing;
}

bool oc_bpt_alt_insert_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p)
{
    Oc_bpt_alt_rec *rec_p, *tmp_p;

    for (tmp_p = (Oc_bpt_alt_rec*)ssdlist_head(&s_p->list);
         tmp_p != NULL;
         tmp_p = (Oc_bpt_alt_rec*)ssdlist_next(&tmp_p->link))
    {
        switch ((s_p->cfg_p->key_compare)(key_p, tmp_p->key_p)) {
        case 0:
            // the key is already here, replace the data
            memcpy(tmp_p->data_p, data_p, s_p->cfg_p->data_size);
            return TRUE;
        case -1:
            // need to continue searching
            continue;
        case 1:
            // insert it here.
            rec_p = rec_alloc(s_p, key_p, data_p);
            ssdlist_add_before(&s_p->list, &tmp_p->link, &rec_p->link);
            return FALSE;
        }
    }

    // The new key is the highest, insert it last
    // this also takes care of the empty-list case
    rec_p = rec_alloc(s_p, key_p, data_p);
    ssdlist_add_tail(&s_p->list, &rec_p->link);
    return FALSE;
}

bool oc_bpt_alt_lookup_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_po)
{
    Oc_bpt_alt_rec *tmp_p;

    for (tmp_p = (Oc_bpt_alt_rec*)ssdlist_head(&s_p->list);
         tmp_p != NULL;
         tmp_p = (Oc_bpt_alt_rec*)ssdlist_next(&tmp_p->link))
    {
        switch ((s_p->cfg_p->key_compare)(key_p, tmp_p->key_p)) {
        case 0:
            memcpy((char*)data_po, (char*)tmp_p->data_p, s_p->cfg_p->key_size);
            return TRUE;
        case -1:
            // need to continue searching
            continue;
        case 1:
            // did not find the key
            return FALSE;
        }
    }

    return FALSE;
}

bool oc_bpt_alt_remove_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *key_p)
{
    Oc_bpt_alt_rec *tmp_p;

    for (tmp_p = (Oc_bpt_alt_rec*)ssdlist_head(&s_p->list);
         tmp_p != NULL;
         tmp_p = (Oc_bpt_alt_rec*)ssdlist_next(&tmp_p->link))
    {
        switch ((s_p->cfg_p->key_compare)(key_p, tmp_p->key_p)) {
        case 0:
            ssdlist_remove(&s_p->list, &tmp_p->link);
            rec_free(tmp_p);
            return TRUE;
        case -1:
            // need to continue searching
            continue;
        case 1:
            // did not find the key
            return FALSE;
        }
    }

    return FALSE;
}

void oc_bpt_alt_delete_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p)
{
    Oc_bpt_alt_rec *tmp_p;

    while (!ssdlist_empty(&s_p->list))
    {
        tmp_p = (Oc_bpt_alt_rec*)ssdlist_remove_head(&s_p->list);
        rec_free(tmp_p);
    }

    ssdlist_init(&s_p->list);
}

/******************************************************************/
bool oc_bpt_alt_dbg_validate_b(struct Oc_wu *wu_p,
                           Oc_bpt_alt_state *s_p)
{
    Oc_bpt_alt_rec *crnt_p, *prev_p;
    
//    printf("// alt_validate len=%d\n", ssdlist_get_length(&s_p->list));
    if (ssdlist_get_length(&s_p->list) == 0)
        return TRUE;

    prev_p = (Oc_bpt_alt_rec*)ssdlist_head(&s_p->list);

    if (ssdlist_get_length(&s_p->list) == 1) {
        if (NULL == prev_p->key_p ||
            NULL == prev_p->data_p)
            return FALSE;
    }


    crnt_p = (Oc_bpt_alt_rec*)ssdlist_next(&prev_p->link);
    while (crnt_p != NULL)
    {
        if (NULL == crnt_p->key_p ||
            NULL == crnt_p->data_p)
            return FALSE;

        if (s_p->cfg_p->key_compare(prev_p->key_p, crnt_p->key_p) != 1)
            return FALSE;
        
        prev_p = crnt_p;
        crnt_p = (Oc_bpt_alt_rec*)ssdlist_next(&prev_p->link);
    }

    return TRUE;
}
/******************************************************************/
void oc_bpt_alt_dbg_output_b(struct Oc_wu *wu_p,
                             Oc_bpt_alt_state *s_p)
{
    char s1[20];
    char s2[20];
    Oc_bpt_alt_rec *crnt_p;

//    printf("// %s   ", (oc_bpt_alt_validate_b(wu_p, s_p) ? "VALID" : "INVALID" ));
    printf("// list=");
    for (crnt_p = (Oc_bpt_alt_rec*)ssdlist_head(&s_p->list);
         crnt_p != NULL;
         crnt_p = (Oc_bpt_alt_rec*)ssdlist_next(&crnt_p->link))
    {
        s_p->cfg_p->key_to_string(crnt_p->key_p, s1, 20);
        s_p->cfg_p->data_to_string(crnt_p->data_p, s2, 20);
    
        printf("(key=%s, data=%s) ", s1, s2);
    }
    printf("\n ");
}

/******************************************************************/
// lookup a range of keys
void oc_bpt_alt_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p,
    int max_num_keys_i,
    struct Oc_bpt_key *key_array_po,
    struct Oc_bpt_data *data_array_po,
    int *nkeys_found_po)
{
    Oc_bpt_alt_rec *crnt_p;
    bool rc;
    int cursor_keys, cursor_data;

    *nkeys_found_po = 0;
    cursor_keys = 0;
    cursor_data = 0;
    for (crnt_p = (Oc_bpt_alt_rec*)ssdlist_head(&s_p->list);
         crnt_p != NULL && *nkeys_found_po < max_num_keys_i;
         crnt_p = (Oc_bpt_alt_rec*)ssdlist_next(&crnt_p->link))
    {
        switch ((s_p->cfg_p->key_compare)(min_key_p, crnt_p->key_p)) {
        case 0:
            rc = TRUE;
            break;
        case -1:
            rc = FALSE;
            break;
        case 1:
            rc = TRUE;
            break;
        }

        if (!rc) continue;

        switch ((s_p->cfg_p->key_compare)(crnt_p->key_p, max_key_p)) {
        case 0:
            rc = TRUE;
            break;
        case -1:
            rc = FALSE;
            break;
        case 1:
            rc = TRUE;
            break;
        }

        if (!rc) continue;

        // the key is in the range, copy it.
        memcpy((char*)key_array_po + cursor_keys,
               (char*)crnt_p->key_p,
               s_p->cfg_p->key_size);
        if (data_array_po)
            memcpy((char*)data_array_po + cursor_data,
                   (char*)crnt_p->data_p,
                   s_p->cfg_p->data_size);

        (*nkeys_found_po)++;
        cursor_keys += s_p->cfg_p->key_size;
        cursor_data += s_p->cfg_p->data_size;
    }
}


/******************************************************************/

static inline struct Oc_bpt_key* key_array_kth(
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key* key_array,
    int k)
{
    return (struct Oc_bpt_key*) ((char*)key_array + s_p->cfg_p->key_size * k);
}

static inline struct Oc_bpt_data* data_array_kth(
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_data* data_array,
    int k)
{
    return (struct Oc_bpt_data*) ((char*)data_array + s_p->cfg_p->data_size * k);
}

int oc_bpt_alt_insert_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_alt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_data *data_array)
{
    Oc_bpt_alt_rec *rec_p, *crnt_p;
    int rc, i;
    
    // double iteration over the linked-list and the array to insert
    for (i=0, rc=0, crnt_p = (Oc_bpt_alt_rec*)ssdlist_head(&s_p->list);
         i < length && crnt_p != NULL;
        )
    {
        struct Oc_bpt_key *key_i_p = key_array_kth(s_p, key_array, i);
        struct Oc_bpt_data *data_i_p = data_array_kth(s_p, data_array, i);
        
        switch ((s_p->cfg_p->key_compare)(key_i_p, crnt_p->key_p)) {
        case 0:
            // the key is already here, replace the data
            memcpy(crnt_p->data_p, data_i_p, s_p->cfg_p->data_size);
            rc++;
            i++;
            crnt_p = (Oc_bpt_alt_rec *)ssdlist_next(&crnt_p->link);
            break;
        case -1:
            // need to continue searching
            crnt_p = (Oc_bpt_alt_rec *)ssdlist_next(&crnt_p->link);
            break;
        case 1:
            // insert it here.
            rec_p = rec_alloc(s_p, key_i_p, data_i_p);
            ssdlist_add_before(&s_p->list, &crnt_p->link, &rec_p->link);
            i++;
            break;
        }
    }

    // Reached the end of the list. Insert the rest of the entries in 
    // the tail.
    for ( ; i<length; i++) {
        struct Oc_bpt_key *key_i_p = key_array_kth(s_p, key_array, i);
        struct Oc_bpt_data *data_i_p = data_array_kth(s_p, data_array, i);

        rec_p = rec_alloc(s_p, key_i_p, data_i_p);
        ssdlist_add_tail(&s_p->list, &rec_p->link);
    }

    return rc;
}

/******************************************************************/

// remove a range of keys
int oc_bpt_alt_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p)
{
    Oc_bpt_alt_rec *crnt_p, *to_rmv_p;
    int nkeys_found = 0;

    for (crnt_p = (Oc_bpt_alt_rec*)ssdlist_head(&s_p->list);
         crnt_p != NULL;
         /* */ )
    {
        if (((s_p->cfg_p->key_compare)(min_key_p, crnt_p->key_p) >= 0) &&
            ((s_p->cfg_p->key_compare)(max_key_p, crnt_p->key_p) <= 0))
        {
            // increment the counter
            nkeys_found++;
            
            to_rmv_p = crnt_p;
            crnt_p = (Oc_bpt_alt_rec *)ssdlist_next(&crnt_p->link);

            // free this entry
            ssdlist_remove(&s_p->list, &to_rmv_p->link);
            rec_free(to_rmv_p);
        }
        else
            crnt_p = (Oc_bpt_alt_rec *)ssdlist_next(&crnt_p->link);            
    }

    return nkeys_found;
}

/******************************************************************/

uint64 oc_bpt_alt_clone_b(
    struct Oc_wu *wu_p,
    Oc_bpt_alt_state *src_p,
    Oc_bpt_alt_state *trg_p)
{
   /* simply create a new copy of the linked-list in [src_p]
    * and link it onto [trg_p]
    */
    Oc_bpt_alt_rec *rec_p, *crnt_p;

    // make sure the configurations are equivalent
    oc_utl_assert(trg_p->cfg_p == src_p->cfg_p);
        
    for (crnt_p = (Oc_bpt_alt_rec*)ssdlist_head(&src_p->list);
         crnt_p != NULL;
         crnt_p = (Oc_bpt_alt_rec *)ssdlist_next(&crnt_p->link))
    {
        rec_p = rec_alloc(src_p, crnt_p->key_p, crnt_p->data_p);
        ssdlist_add_tail(&trg_p->list, &rec_p->link);
    }

    return 0;
}

/******************************************************************/
