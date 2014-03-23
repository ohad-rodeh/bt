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
// All the data structures used by the page-manager

#ifndef OC_PM_S_H
#define OC_PM_S_H

#include "oc_io_s.h"
#include "oc_utl_s.h"
#include "oc_wu_s.h"
#include "oc_utl_htbl_s.h"
#include "oc_crt_int.h"
#include "pl_mm_int.h"

// This should be removed later, it is here just for backward compatibility
typedef struct Oc_pm_config {
    // The number of pages managed by the PM
    int num_pages;
} Oc_pm_config;


typedef struct Oc_pm_page_cb {
    Ss_slist_node       hash_chain_link; // hash chain link
    Ss_dlist_node       lru_link;

    int                 pg_ref_count;    
    Oc_utl_page        *base_pg_p;       // The basic page struct from UTL
    Oc_meta_data_page_hndl hndl;

    struct {
        unsigned        invalid:1;       // An invalid page, no valid data
        unsigned        dirty:1;         // A dirty page
        unsigned        force:1;         // Force this page to disk 
    } flags;
} Oc_pm_page_cb;

// All these counters count the number of occurences of certain events
typedef struct Oc_pm_stats {
    uint32 get_page_cnt;      // #get_page calls
    uint32 new_page_cnt;      // #new_page calls
    uint32 get_page_hit_cnt;  // get_page found the page already in-cache
    uint32 new_page_hit_cnt;  // new_page found the page already in-cache
} Oc_pm_stats;


typedef struct Oc_pm_ca {
    int htbl_size;        // size of the hash-table
    Oc_utl_htbl htbl;     // Hash-table that points to ref-counted pages
    Ss_dlist lru_l;       // LRU list, the head is where latest pages are located.
                          // The tail is where the oldest pages are located. 
} Oc_pm_ca;

typedef struct Oc_pm_global {
    Oc_pm_config     config;
    Oc_pm_stats      stats;
    Oc_pm_ca         ca;                 // area for cache
    Pl_mm_op         *pcb_pool_p;         // The pool of PCBs
    Oc_crt_atom_cnt  reserve_cnt;        // counter for making reservations atomic
} Oc_pm_global;

typedef struct Oc_pm_ref_set {
    /* if [account] is true then account for all pages taken and
     * and released
     */
    bool account;  
    Oc_pm_page_cb *pages[OC_PM_MAX_REFS+1];
    int cursor;   // The first empty location
    int sum;      // The total number of pages referenced. 
} Oc_pm_ref_set;

typedef struct Oc_pm_rm {
    int   num_reserved;   // The number of pages reserved by this work-unit
    Oc_pm_ref_set refs;    // The set of pages referenced by this work-unit
} Oc_pm_rm;

#endif
