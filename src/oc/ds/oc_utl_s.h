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
#ifndef OC_UTL_S_H
#define OC_UTL_S_H

#include "oc_config.h"
#include "oc_crt_s.h"
#include <sys/time.h>

#define OC_PACKED __attribute__ ((__packed__))

// size of attributes buffer inside a btree root node.
#define OC_UTL_ATTRIBUTES_BUF_SIZE 128

/**
 * The structure of an ObS meta data page.
 */
typedef struct Oc_meta_data_page_hdr {
    char eye_catcher[4];
    uint32 lrc;
    uint16 component_id; // same as Oc_subcomponent_id
    uint16 version;

    char reserved[12];    // reserved for future use
} OC_PACKED Oc_meta_data_page_hdr;

// Minimal data-structure for handling a meta-data page
typedef struct Oc_meta_data_page_hndl {
    // read-write lock for the page    
    Oc_crt_rw_lock lock; 

    // pointer to an actual data buffer of size SS_PAGE_SIZE    
    char *data;

    // address of this node on disk, in bytes (not sectors).
    uint64 disk_addr;          
} Oc_meta_data_page_hndl;

typedef struct Oc_utl_config {
    uint64 lun_size;                // LUN size in bytes
    //    int    num_pages;               // The number of pages
    uint16 version;                 // Version number of ObjectStone
    //    int    max_num_pages_in_io;     // Maximum number of pages in a single IO
    char   data_dev[60];            // Device name to store data
    char   ljl_dev[60];             // Device name to store journal

} Oc_utl_config;

#define OC_UTL_TRK_MAX_REFS (30)

struct Oc_crt_rw_lock;

typedef struct Oc_utl_trk_ref_set {
    struct Oc_crt_rw_lock *locks[OC_UTL_TRK_MAX_REFS+1];
    int cursor;   // The first empty location
    int sum;      // The total number of locks referenced. 
} Oc_utl_trk_ref_set;

typedef struct Oc_utl_rm {
    Oc_utl_trk_ref_set refs;
} Oc_utl_rm;

#endif
