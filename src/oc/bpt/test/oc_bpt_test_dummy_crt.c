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
/* OC_BPT_TEST_DUMMY_CRT.C
 * Dummy implementation of THE co-routine module.
 */
/******************************************************************/
#include <string.h>

#include "pl_base.h"
#include "oc_crt_s.h"
#include "oc_utl.h"
/******************************************************************/

void oc_crt_init_rw_lock(Oc_crt_rw_lock * lock_p)
{
    memset(lock_p, 0, sizeof(Oc_crt_rw_lock));
    lock_p-> mode = CRT_RWSTATE_NONE;
}

void oc_crt_lock_read(Oc_crt_rw_lock *lock_p)
{
    oc_utl_assert(CRT_RWSTATE_NONE == lock_p->mode);
    lock_p->mode = CRT_RWSTATE_READ;
}

void oc_crt_lock_write(Oc_crt_rw_lock *lock_p)
{
    oc_utl_assert(CRT_RWSTATE_NONE == lock_p->mode);
    lock_p->mode = CRT_RWSTATE_WRITE;
}

void oc_crt_unlock(Oc_crt_rw_lock * lock_p)
{
    oc_utl_assert(CRT_RWSTATE_NONE != lock_p->mode);
    lock_p-> mode = CRT_RWSTATE_NONE;
}

bool oc_crt_rw_is_locked_write(Oc_crt_rw_lock *lock_p)
{
    return (CRT_RWSTATE_WRITE == lock_p->mode);
}

bool oc_crt_rw_is_locked_read(Oc_crt_rw_lock *lock_p)
{
    return (CRT_RWSTATE_READ == lock_p->mode);
}

bool oc_crt_rw_is_unlocked(Oc_crt_rw_lock *lock_p)
{
    return (CRT_RWSTATE_NONE == lock_p->mode);
}

bool oc_crt_rw_is_locked_read(Oc_crt_rw_lock *lock_p);
bool oc_crt_rw_is_unlocked(Oc_crt_rw_lock *lock_p);


void oc_crt_init_full(struct Oc_crt_config * config_p)
{
}

void oc_crt_default_config(struct Oc_crt_config *config_p)
{
}

void oc_crt_init(void)
{
}

void oc_crt_yield_task(void)
{
}

void oc_crt_assert(void)
{
}

int oc_crt_get_thread(void)
{
    return 1;
}

int oc_crt_atom_cnt_get_val(struct Oc_crt_atom_cnt * acnt_p)
{
    return 1;
}

void oc_crt_sleep(uint32 milisec)
{
}

void oc_crt_create_task(const char * name_p, 
                        void (*run_p) (void *), 
                        void * arg_p)
{
}

void oc_crt_create_task_b(const char * name_p, void (*run_p) (void *), void * arg_p)
{
}

uint64 oc_crt_get_time_millis(void)
{
    return 1;
}

void oc_crt_atom_cnt_inc(struct Oc_crt_atom_cnt * acnt_p, int num_i)
{
}

void oc_crt_wake_up_all(struct Oc_crt_wait_q * wq)
{
}

void oc_crt_atom_cnt_dec(struct Oc_crt_atom_cnt * acnt_p, int num_i)
{
}

void oc_crt_init_wait_q(struct Oc_crt_wait_q * wq)
{
}

void oc_crt_atom_cnt_init(struct Oc_crt_atom_cnt * acnt_p, int init_num_i)
{
}

/******************************************************************/
