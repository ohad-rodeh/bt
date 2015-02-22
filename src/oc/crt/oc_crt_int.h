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
#ifndef OC_CRT_INT_H
#define OC_CRT_INT_H

#include "pl_dstru.h"
#include "oc_utl.h"
#include "oc_crt_s.h"

void oc_crt_init_full(Oc_crt_config * config_p);
void oc_crt_default_config(Oc_crt_config *config_p);
void oc_crt_init(void);
Oc_crt_config *oc_crt_get_config(void);

// assert that the caller is on a co-routine stack
void oc_crt_assert(void);

// Return the pthread-id used by the co-routines
int oc_crt_get_thread (void);

/* The CRT uses virtual time in Harness mode, therefore,
 * all uses of time should go through this function.
 *
 * Return time in milliseconds since initialization
 */
uint64 oc_crt_get_time_millis(void);

/* Fork a new task to run function [run_p]. 
 *
 * can -only- be called from the pthread running co-routines.
 */
int oc_crt_create_task(const char * name_p,
                        void *(*start_routine) (void *),
                        void * arg_p);

void oc_crt_yield_task();

//Oc_crt_task *oc_crt_get_current_task(void);

// kills all tasks except current one
//void oc_crt_kill_all();

// RW locks
void oc_crt_init_rw_lock(Oc_crt_rw_lock * lock);
void oc_crt_lock_read(Oc_crt_rw_lock * lock);
void oc_crt_lock_write(Oc_crt_rw_lock * lock);
void oc_crt_unlock(Oc_crt_rw_lock * lock);

// return TRUE if lock is taken for write, FALSE otherwise
bool oc_crt_rw_is_locked_write(Oc_crt_rw_lock * lock);
bool oc_crt_rw_is_locked_read(Oc_crt_rw_lock * lock_p);
bool oc_crt_lock_check(Oc_crt_rw_lock * lock_p);

// Semaphores
void oc_crt_sema_init(Oc_crt_sema * sema_p, int value);
void oc_crt_sema_post(Oc_crt_sema * sema_p);
void oc_crt_sema_wait(Oc_crt_sema * sema_p);
int  oc_crt_sema_get_val(Oc_crt_sema * sema_p);

#endif

