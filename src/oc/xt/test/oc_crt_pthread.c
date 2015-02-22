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
/******************************************************************/
// OC_XT_TEST_DUMMY_CRT.C
// dummy implementation of CRT
/******************************************************************/
#include "oc_crt_int.h"

#include <pthread.h>
#include <semaphore.h>
#include <sched.h>

static Oc_crt_config config;
/******************************************************************/

// pthread-based implementation

void oc_crt_init_rw_lock(Oc_crt_rw_lock * lock)
{
    int ern;
    pthread_rwlock_t *rw_p = (pthread_rwlock_t*) lock;

    if (ern = pthread_rwlock_init(rw_p, NULL))
        ERR(("pthread_rwlock_init: (%d) %s", ern, strerror(ern))) ;
}

void oc_crt_lock_read(Oc_crt_rw_lock * lock)
{
    int ern;
    pthread_rwlock_t *rw_p = (pthread_rwlock_t*) lock;

    if (ern = pthread_rwlock_rdlock (rw_p))
        ERR(("pthread_rwlock_rdlock: (%d) %s", ern, strerror(ern))) ;
}

void oc_crt_lock_write(Oc_crt_rw_lock * lock)
{
    int ern;
    pthread_rwlock_t *rw_p = (pthread_rwlock_t*) lock;

    if (ern = pthread_rwlock_wrlock(rw_p))
        ERR(("pthread_rwlock_wrlock: (%d) %s", ern, strerror(ern))) ;
}

void oc_crt_unlock(Oc_crt_rw_lock * lock)
{
    int ern;
    pthread_rwlock_t *rw_p = (pthread_rwlock_t*) lock;

    if (ern = pthread_rwlock_unlock(rw_p))
        ERR(("pthread_rwlock_unlock: (%d) %s", ern, strerror(ern))) ;
}

void oc_crt_init_full(Oc_crt_config * config_p)
{
    int id;
    
    config = *config_p;

    printf("sizeof(oc_crt_rwlock) = %d\n", sizeof(Oc_crt_rw_lock));
    printf("sizeof(pthread_rwlock_t) = %d\n", sizeof(pthread_rwlock_t));
    printf("sizeof(oc_crt_sema) = %d\n", sizeof(Oc_crt_sema));
    printf("sizeof(sem_t) = %d\n", sizeof(sem_t));

    oc_utl_assert(sizeof(Oc_crt_rw_lock) >= sizeof(pthread_rwlock_t));
    oc_utl_assert(sizeof(Oc_crt_sema) >= sizeof(sem_t));
    
    if (pthread_create((pthread_t*)&id,
                       NULL,
                       (void *(*)(void *))config.init_fun, NULL) != 0)
        ERR(("could not open main thread"));
}

void oc_crt_default_config(Oc_crt_config *config_p)
{
    memset(config_p, 0, sizeof(Oc_crt_config));
}

void oc_crt_init(void)
{
    oc_crt_init_full(&config);
}

void oc_crt_yield_task(void)
{
    sched_yield ();
}

void oc_crt_assert(void)
{
}

int oc_crt_get_thread(void)
{
    return (int)pthread_self ();
}


void oc_crt_sema_init(Oc_crt_sema * sema_p, int value)
{
    int ern;
    sem_t *sem_p = (sem_t *) sema_p;
    
    if (ern = sem_init(sem_p, 0, value)) 
        ERR(("sem_init: (%d) %s", ern, strerror(ern))) ;
}

void oc_crt_sema_post(Oc_crt_sema * sema_p)
{
    int ern;
    sem_t *sem_p = (sem_t *) sema_p;

    if (ern = sem_post (sem_p))
        ERR(("sem_post: (%d) %s", ern, strerror(ern))) ;
}

void oc_crt_sema_wait(Oc_crt_sema * sema_p)
{
    int ern;
    sem_t *sem_p = (sem_t *) sema_p;

    if (ern = sem_wait(sem_p))
        ERR(("sem_wait: (%d) %s", ern, strerror(ern))) ;
}

void oc_crt_create_task(const char * name_p, void (*run_p) (void *), void * arg_p)
{
    int id;

    if (pthread_create((pthread_t *)&id,
                       NULL,
                       (void *(*)(void *)) run_p,
                       arg_p) != 0)
        ERR(("could not create a thread"));
}

/******************************************************************/
