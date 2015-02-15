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

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

#include "pl_base.h"

typedef struct Oc_crt_config {
    uint32 num_tasks;
    uint32 max_cycle_count;
    uint32 stack_page_size;  // how many 4K pages to use for a stack? 
    uint32 stack_guard_size;
    uint32 stack_size;       // computed internally
    bool deterministic_run;

    void (*init_fun) (void);  // function to execute after start-up
} Oc_crt_config;

typedef enum Oc_crt_rw_mode {
    CRT_RWSTATE_NONE,
    CRT_RWSTATE_READ,
    CRT_RWSTATE_WRITE,
} Oc_crt_rw_mode;

// semaphore
typedef struct Oc_crt_sema {
    sem_t sem;
} Oc_crt_sema;

typedef struct Oc_crt_rw_lock {
    pthread_rwlock_t lock;
} Oc_crt_rw_lock;

#endif

