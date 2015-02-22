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
/* 
 * A self test for the lock-tracking code
 */
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <semaphore.h>

#include "pl_int.h"
#include "oc_wu_s.h"
#include "oc_rm_s.h"
#include "oc_crt_int.h"
#include "oc_utl.h"
#include "oc_utl_trk.h"

/********************************************************************/
static void run_tests(void* dummy)
{
    Oc_crt_rw_lock lock1, lock2, lock3;
    Oc_wu wu;
    Oc_rm_ticket rm_ticket;

    memset(&wu, 0, sizeof(wu));
    memset(&rm_ticket, 0, sizeof(rm_ticket));
    wu.rm_p = &rm_ticket;

    oc_crt_init_rw_lock(&lock1);
    oc_crt_init_rw_lock(&lock2);
    oc_crt_init_rw_lock(&lock3);

    oc_utl_trk_crt_lock_read(&wu, &lock1);
    oc_utl_trk_crt_lock_read(&wu, &lock2);
    oc_utl_trk_crt_lock_write(&wu, &lock3);
    
    oc_utl_trk_crt_unlock(&wu, &lock1);
    oc_utl_trk_crt_lock_write(&wu, &lock1);
    oc_utl_trk_crt_unlock(&wu, &lock2);
    oc_utl_trk_abort(&wu);

    printf("\nSuccess\n"); 
    exit(0);
}

static void exec_test(void)
{
    // Open a task to run all the tests
    oc_crt_create_task("run-tests", run_tests, NULL);
}

int main (int argc, char *argv[])
{
    sem_t sema;
    Oc_crt_config crt_conf;
    
    pl_init();
    oc_utl_init();

    oc_crt_default_config(&crt_conf);
    crt_conf.init_fun = exec_test;
    oc_crt_init_full(&crt_conf);
        
    sem_init(&sema,0,0);
    sem_wait(&sema);

    return 0;
}

/********************************************************************/
