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
/* Author: Ohad Rodeh 10/2002
 * Description: A generic hashtable.
 * Very minor adaptation for utl use by GC 1/2003.
 *
 * Implements a hashtable of a fixed number of buckets. All operations
 * do not consume the cell memory.
 */

#include "oc_utl_htbl.h"
#include "oc_utl.h"
#include <memory.h>
#include <stdio.h>


/* Create a hashtable. User needs to provide four functions: hash, compare, get_next,
 * and set_next.
 */
void oc_utl_htbl_create(
    Oc_utl_htbl *htbl,
    int size,
    char *slot_array_pi,
    Oc_utl_htbl_hash      hash,
    Oc_utl_htbl_compare   compare)
{
    memset(htbl, 0, sizeof(Oc_utl_htbl));
    if (NULL == slot_array_pi) {
        htbl->arr = (Ss_slist_node**) pl_mm_malloc(size * sizeof(int*));
        if (htbl->arr) {
            htbl->arr_malloced_on_create = TRUE;
        }
    }
    else
        htbl->arr = (Ss_slist_node**) slot_array_pi;

    memset(htbl->arr, 0, size * sizeof(int*));
    htbl->len=size;
    htbl->size = 0;
    htbl->hash = hash;
    htbl->compare = compare;
}

// abruptly free up memory of htbl
// for a clean release, user may want to use oc_utl_htbl_iter_discard
// before calling this api.
void oc_utl_htbl_free(Oc_utl_htbl *htbl)
{
    memset(htbl->arr, 0, htbl->len * sizeof(int));
    if (htbl->arr_malloced_on_create) {
        pl_mm_free((char*)htbl->arr);
    }
    memset(htbl, 0, sizeof(Oc_utl_htbl));
}

/** Check existence in the table. Return 1 if true, 0 if false.
 * @h - hash table
 * @o - object name to look for
 */
int oc_utl_htbl_exists(Oc_utl_htbl *h_i, void *key_i)
{
    uint32 slot;
    Ss_slist_node *tmp;
    
    slot = h_i->hash(key_i, h_i->len, 0);
    oc_utl_debugassert(slot < h_i->len && 0 <= slot);
    tmp = h_i->arr[slot];
    
    while (tmp != NULL) {
        if (h_i->compare((void*)tmp, key_i) == TRUE)
            return TRUE;
        tmp = tmp->next;
    }
    return FALSE;
}

/** lookup in the table.
 */
void* oc_utl_htbl_lookup(Oc_utl_htbl *h_i, void *key_i)
{
    uint32 slot;
    Ss_slist_node *tmp;
//    int j = 0;
    
    slot = h_i->hash(key_i,h_i->len, 0);
    oc_utl_debugassert(slot < h_i->len && 0 <= slot);
    tmp = h_i->arr[slot];
    
    while (tmp) {
        if (h_i->compare(tmp, key_i) == TRUE)
            return tmp;
        tmp = tmp->next;
        // j++;
    }

/*    if (j>5)
        WRN(("more than 5 elements in the same bucket! (htbl=%p slot=%lu #elem=%d)",
        h_i, slot, j));*/
    
    return NULL;
}

/* lookup in the table, and extract the item if it exists
 */
void* oc_utl_htbl_extract(Oc_utl_htbl *h_i, void *key_i)
{
    uint32 slot;
    Ss_slist_node *tmp, *rc;
    
    slot = h_i->hash(key_i,h_i->len, 0);
    oc_utl_debugassert(slot < h_i->len && 0 <= slot);
    tmp = h_i->arr[slot];

    if (tmp == NULL) return NULL;

    if (h_i->compare(tmp, key_i) == TRUE) {
        rc = tmp;
        h_i->arr[slot] = tmp->next;
        h_i->size--;
        return rc;
    }

    while (tmp) {
        if (tmp->next == NULL) return NULL;
        if (h_i->compare(tmp->next, key_i) == TRUE) {
            rc = tmp->next;
            tmp->next = tmp->next->next;
            h_i->size--;
            return rc;
        }
        tmp = tmp->next;
    }

    return NULL;    
}

/*
 *  insert: Insert an item into the hash table
 * first check if the item already exists. In this case, return NO.
 * Otherwise, insert the item at the start of the list, and return
 * TRUE;
 *
 */
void oc_utl_htbl_insert(Oc_utl_htbl *h_i, void *key_i, void* _elem)
{
    uint32 slot;
    Ss_slist_node *head = NULL;
    Ss_slist_node *elem = (Ss_slist_node*) _elem;

#if OC_DEBUG
    if (oc_utl_htbl_exists(h_i, key_i) == TRUE)
        ERR(("Trying to add an element to the hash-table twice"));
#endif
        
    h_i->size++;
    slot = h_i->hash(key_i, h_i->len, 0);
    oc_utl_assert(slot < h_i->len && 0 <= slot);
    head = h_i->arr[slot];
    elem->next = head;
    h_i->arr[slot] = elem;
}

bool oc_utl_htbl_remove(Oc_utl_htbl *h_i, void *key_i)
{
    uint32 slot;
    Ss_slist_node *tmp = NULL;
    
    slot = h_i->hash(key_i, h_i->len, 0);
    oc_utl_assert(slot < h_i->len && 0 <= slot);
    tmp = h_i->arr[slot];

    if (tmp == NULL) return FALSE;

    if (h_i->compare(tmp, key_i) == TRUE) {
        h_i->arr[slot] = tmp->next;
        h_i->size--;
        return TRUE;
    }

    while (tmp) {
        if (tmp->next == NULL) return FALSE;
        if (h_i->compare(tmp->next, key_i) == TRUE) {
            tmp->next = tmp->next->next;
            h_i->size--;
            return TRUE;
        }
        tmp = tmp->next;
    }

    return FALSE;
}


void oc_utl_htbl_iter(Oc_utl_htbl *h_i, void (*fun)(void *elem, void *ctx), void *ctx)
{
    uint32 i;
    Ss_slist_node *elem;
    
    for (i=0; i<h_i->len; i++) {
        elem = h_i->arr[i] ;
        while (elem) {
            fun(elem, ctx);
            elem = elem->next;
        }
    }
}

/* Same as above, but throw out any items that [fun] returns TRUE for.
 */
void oc_utl_htbl_iter_discard(Oc_utl_htbl *h_i,
                          bool (*fun)(void *elem, void *data),
                          void *additional_data )
{
    uint32 i;
    Ss_slist_node *elem_p;
    Ss_slist_node *next_elem_p;
    int cont;
        
    for (i=0; i<h_i->len; i++) {
        elem_p = h_i->arr[i] ;
        if (NULL == elem_p) continue;

        // If the first element(s) should be removed -- remove them
        cont = TRUE;
        while ( cont
                && elem_p ) {
            cont = FALSE;

            next_elem_p = elem_p->next;

            switch (fun(elem_p, additional_data)) {
            case FALSE:
                break;
            case TRUE:
                elem_p = next_elem_p;
                h_i->arr[i] = elem_p;
                h_i->size--;
                cont = TRUE;
                break;
            }
        }

        // No point to continue if there is nothing left in the list
        if ( NULL == elem_p
          || NULL == elem_p->next ) continue;
            
        // We know that [elem_p] exists
        while (elem_p->next) {
            next_elem_p = elem_p->next->next;
            switch (fun(elem_p->next, additional_data)){
            case FALSE:
                elem_p = elem_p->next;
                break;
            case TRUE:
                elem_p->next = next_elem_p;
                h_i->size--;
                break;
            }
        }
    }
}

/*********************************************************************************/

/* Same as above, but move any items that [fun] returns TRUE for into [list]
 */
void oc_utl_htbl_iter_mv_to_list(Oc_utl_htbl *h_i,
                             bool (*fun)(void *elem, void *data),
                             void *additional_data,
                             Ss_slist *list_pi)
{
    uint32 i;
    Ss_slist_node *elem_p, *rmv_p;
    int cont;

    for (i=0; i < h_i->len; i++) {
        elem_p = h_i->arr[i] ;
        if (NULL == elem_p) continue;

        // If the first element(s) should be removed -- remove them
        cont = TRUE;
        while ( cont
                && elem_p != NULL) {
            cont = FALSE;

            switch (fun(elem_p, additional_data)) {
            case FALSE:
                break;
            case TRUE:
                rmv_p = elem_p;
                elem_p = elem_p->next;
                h_i->arr[i] = elem_p;
                h_i->size--;
                cont = TRUE;
                rmv_p->next = NULL;
                ssslist_add_tail(list_pi, rmv_p );
                break;
            }
        }

        // No point to continue if there is nothing left in the list
        if ( NULL == elem_p
             || NULL == elem_p->next ) continue;
            
        // We know that [elem_p] exists
        while (elem_p->next) {
            switch (fun(elem_p->next, additional_data)){
            case FALSE:
                elem_p = elem_p->next;
                break;
            case TRUE:
                rmv_p = elem_p->next;
                elem_p->next = elem_p->next->next;
                h_i->size--;
                rmv_p->next = NULL;
                ssslist_add_tail(list_pi, rmv_p );
                break;
            }
        }
    }
}

/*********************************************************************************/
