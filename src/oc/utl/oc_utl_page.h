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
#ifndef OC_UTL_PAGE_H
#define OC_UTL_PAGE_H

#include "oc_wu_s.h"
#include "oc_utl.h"

// page stuff

// Initialize the page-pool etc. 
void oc_utl_page_init(int num_pages);
    
void oc_utl_page_free_resources();

// Allocate a page from the page-pool atomically
Oc_utl_page *oc_utl_page_alloc_b(Oc_wu *wu_p);

/* Allocate a page -now-, assert if the
 * pool is empty
 */
Oc_utl_page *oc_utl_page_alloc_strict(Oc_wu *wu_p);

// Increment the ref-count for the page
void oc_utl_page_inc(Oc_wu *wu_p, Oc_utl_page *pg_p);

/* Decrement the ref-count for the page.
 * If the ref-count reaches zero then the page
 * is returned to the page-pool.
 */
void oc_utl_page_dec(Oc_wu *wu_p, Oc_utl_page *pg_p);

const char *oc_utl_page_string_of_state(Oc_utl_page_state state);

// Get the current number of free pages
int oc_utl_page_num_free(void);

#endif
