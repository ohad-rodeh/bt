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
#ifndef OC_CRT_S_H
#define OC_CRT_S_H

#include <setjmp.h>
#include "pl_dstru.h"
#include "pl_utl.h"

typedef struct Oc_crt_config {
    uint32 num_tasks;
    uint32 max_cycle_count;
    uint32 stack_page_size;  // how many 4K pages to use for a stack? 
    uint32 stack_guard_size;
    uint32 stack_size;       // computed internally
    bool deterministic_run;

    void (*init_fun) (void);  // function to execute after start-up
} Oc_crt_config;

typedef struct Oc_crt_wait_q {
    Ss_slist q;
} Oc_crt_wait_q;

typedef enum Oc_crt_rw_mode {
    CRT_RWSTATE_NONE,
    CRT_RWSTATE_READ,
    CRT_RWSTATE_WRITE,
} Oc_crt_rw_mode;

typedef struct Oc_crt_rw_lock {
    uint32 counter;
    Oc_crt_rw_mode mode;
    Oc_crt_wait_q wq;
} Oc_crt_rw_lock;

/* The type of time events.
 */
struct Oc_crt_task;

typedef uint32 Oc_crt_time;

#define MAX_TASK_NAME_LEN 200

typedef struct Oc_crt_tm_ev {
    Ss_dlist_node       event_node;    
    Oc_crt_time		time;
    struct Oc_crt_task  * task_p;
} Oc_crt_tm_ev;


typedef struct Oc_crt_task {
    Ss_slist_node node;
    Ss_dlist_node used_link;
    Oc_crt_rw_mode rwlock_mode;
    uint32 time_to_wake;
    Oc_crt_tm_ev timer;
    
    void (* run_p) (void *);
    void * arg_p;

    char name_p[MAX_TASK_NAME_LEN];

    int * stack;
    int * stack_low;
    uint32 stack_size;
    jmp_buf state;
    jmp_buf exception;
    bool exception_is_set;
    unsigned char exception_caught;
} Oc_crt_task;


// semaphore
typedef struct Oc_crt_sema {
    int magic;  // for sanity checking
    Ss_slist q;
    int counter;
} Oc_crt_sema;

// binary semaphore
typedef struct Oc_crt_bn_sema {
    int magic;  // for sanity checking
    Ss_slist q;
    int counter;
} Oc_crt_bn_sema;

// atomic-counter
typedef struct Oc_crt_atom_cnt {
    int magic ;
    Ss_slist q;
    int counter;
} Oc_crt_atom_cnt;

// A thread-safe implementation of a semaphore
typedef struct Oc_crt_ts_sema {
    int magic ;
    Ss_slist q;
    int counter;
} Oc_crt_ts_sema;

#endif

