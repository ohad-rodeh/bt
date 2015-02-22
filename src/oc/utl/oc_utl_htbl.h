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
/* Author: Ohad Rodeh 10/2002
 * Description: A generic hashtable.
 *
 * Implements a hashtable of a fixed number of buckets. All operations
 * do not consume the cell memory.
 */

#ifndef OC_UTL_HTBL_H
#define OC_UTL_HTBL_H

#include "oc_utl_htbl_s.h"

/* Create a hashtable. User needs to provide two functions: hash, and
 * compare.  The [slot_array_pi] parameter may be NULL in which case
 * the fuction will allocate space for slot-array internally. If it isn't NULL
 * then it -has- to be of length at least [size * sizeof(int)].
 */
void oc_utl_htbl_create(
    Oc_utl_htbl *htbl,
    int size,
    char *slot_array_pi,
    Oc_utl_htbl_hash      hash,
    Oc_utl_htbl_compare   compare
    );

// abruptly free up memory of htbl
// for a clean release, user may want to use oc_utl_htbl_iter_discard
// before calling this api.
void oc_utl_htbl_free(Oc_utl_htbl *htbl);

/** lookup in the table.
 */
void* oc_utl_htbl_lookup(Oc_utl_htbl *h_i, void *key_i);

int oc_utl_htbl_exists(Oc_utl_htbl *h_i, void *key_i);

/*
 *  insert: Insert an item into the hash table. In a debugging build,
 * first check if the item already exists. 
 *
 */
void oc_utl_htbl_insert(Oc_utl_htbl *h_i, void *key_i, void* _elem);

bool oc_utl_htbl_remove(Oc_utl_htbl *h_i, void *key_i);

void oc_utl_htbl_iter(Oc_utl_htbl *h_i,
                      void (*fun)(void *elem, void *ctx),
                      void *ctx);

/* Same as above, but throw out any items that [fun] returns TRUE for.
 */
void oc_utl_htbl_iter_discard( Oc_utl_htbl *h_i,
                           bool (*fun)(void *elem, void *data),
                           void *additional_data );

void oc_utl_htbl_iter_mv_to_list( Oc_utl_htbl *h_i,
                              bool (*fun)(void *elem, void *data),
                              void *additional_data,
                                  Ss_slist *list_pi );

/* lookup in the table, and extract the item if it exists
 */
void* oc_utl_htbl_extract(Oc_utl_htbl *h_i, void *key_i);

#endif 


