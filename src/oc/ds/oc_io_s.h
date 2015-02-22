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
#ifndef OC_IO_S_H
#define OC_IO_S_H

#include "oc_config.h"
#include "oc_wu_s.h"
#include "oc_crt_s.h"
#include "oc_utl_s.h"

// The type of scheduler
typedef enum Oc_io_sched_type {
    // For internal purposes
    OC_IO_SCHED_FIRST, 

    /* A back-end that uses a pool of worker threads that perform
     * synchronous IO. 
     */
    OC_IO_SCHED_THRD,

    // A deterministic way of running the backend
    OC_IO_SCHED_DET,
    
    // For internal purposes
    OC_IO_SCHED_LAST,  
} Oc_io_sched_type;

typedef enum Oc_io_cache_policy {
    OC_IO_CACHE_NONE,      // do not cache anything
    OC_IO_CACHE_ALL,       // Cache data and meta-data
    OC_IO_CACHE_META_DATA, // Cache only meta-data
} Oc_io_cache_policy;

typedef struct Oc_io_config {
    bool               direct_io;
    Oc_io_sched_type   sched;

    /* In case this is the threaded back-end, determine the
     * number of threads used. 
     */
    int              thrd_bkend_num_threads;    
    int              thrd_bkend_size_coalesce_buf;    
} Oc_io_config;

typedef struct Oc_io_stats_container {
    uint64 read_meta;
    uint64 write_meta;
    uint64 read_pg;
    uint64 write_pg;
    uint64 read_pg_misses;
} Oc_io_stats_container;

typedef struct Oc_io_stats {
    Oc_io_stats_container roll;
    Oc_io_stats_container cuml;

    uint32 free_pages_task;
    uint32 slow_writes;
    uint32 io_submit;
    uint32 io_read_page;
    uint32 io_write_page;

    uint32 ca_destage;
    uint32 ca_write_len[10];
} Oc_io_stats;

#endif
