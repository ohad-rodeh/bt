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
/**************************************************************/
/* PL_UTL_H
 *
 * A set of useful definitions that should be placed at the
 * bottom-most level of the system. 
 */
/**************************************************************/
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>

#include "pl_utl.h"
#include "pl_mm_int.h"
#include "pl_trace.h"
#include "pl_base.h"
#include "pl_base.h"
#include "pl_base.h"

/**************************************************************/

static int pthread_create_fun(void* (*main_loop_p)(void *), void* param);
static void iobuf_alloc(
    int len_i,
    struct Pl_utl_iobuf **iobuf_ppo,
    char **buf_ppo);
static void iobuf_free(struct Pl_utl_iobuf *_buf_p);
static Pl_utl_io_rc disk_rw(Pl_utl_disk_desc *disk_p,
                            struct Pl_utl_iobuf *_buf_p, 
                            Pl_utl_rw rw,
                            long long disk_addr,
                            int len,
                            int bufOffset);


static struct Pl_utl_sema *pl_utl_sema_i_create(int init_val);
static void pl_utl_sema_i_wait(struct Pl_utl_sema *_sema_p);
static void pl_utl_sema_i_post(struct Pl_utl_sema *_sema_p);
static void pl_utl_sema_i_destroy(struct Pl_utl_sema *_sema_p);

static struct Pl_utl_bsema *pl_utl_bsema_i_create(void);
static void pl_utl_bsema_i_wait(struct Pl_utl_bsema *_bsema_p);
static void pl_utl_bsema_i_post(struct Pl_utl_bsema *_bsema_p);
static void pl_utl_bsema_i_destroy(struct Pl_utl_bsema *_bsema_p);

static struct Pl_utl_mutex *pl_utl_mutex_i_create(void);
static void pl_utl_mutex_i_destroy(struct Pl_utl_mutex *m_p);
static void pl_utl_mutex_i_lock(struct Pl_utl_mutex *m_p);
static void pl_utl_mutex_i_unlock(struct Pl_utl_mutex *m_p);
static int pl_utl_mutex_i_test_lock(struct Pl_utl_mutex *_m_p);

/**************************************************************/

Pl_utl_config pl_utl_cfg = {0,
                            0,
                            0,                            
                            {pthread_create_fun,
                             disk_rw,
                             pl_utl_bsema_i_create,
                             pl_utl_bsema_i_destroy,
                             pl_utl_bsema_i_post,
                             pl_utl_bsema_i_wait,
                             pl_utl_sema_i_create,
                             pl_utl_sema_i_destroy,
                             pl_utl_sema_i_post,
                             pl_utl_sema_i_wait,
                             pl_utl_mutex_i_create,
                             pl_utl_mutex_i_destroy,
                             pl_utl_mutex_i_lock,
                             pl_utl_mutex_i_unlock,
                             pl_utl_mutex_i_test_lock,
                             iobuf_alloc,
                             iobuf_free}};

/**************************************************************/

static int pthread_create_fun(void* (*main_loop_p)(void *), void* param)
{
    pthread_t pid;
    int rc;
    
    pl_trace(1, PL_EV_CREATE_PTHREAD, "");

    rc = pthread_create(&pid, 
                        NULL,
                        main_loop_p, param);
    
    if (rc) {
        printf("Could not create a pthread\n");
        printf("error (rc=%d) (errno=%d) %s\n",
               rc, errno, strerror(errno));
        ss_assert(0);
    }

    return (int)pid;
}

/* Allocate memory. Intended for use only during initialization.
 * The memory is aligned for IO.
 *
 * In this simple implementation there is no difference between a
 * Pl_utl_iobuf is really a type (char*). In GPFS, the implementation
 * is completely different.
 */
static void iobuf_alloc(
    int len_i,
    struct Pl_utl_iobuf **iobuf_ppo,
    char **buf_ppo)
{
    void *buf_p;
    int rc;
    
    rc = posix_memalign((void**)&buf_p, SS_SECTOR_SIZE, len_i);
    ss_assert(buf_p);
    //ss_assert((long long)buf_p % SS_SECTOR_SIZE == 0);
    ss_assert(0 == rc);
    
    *iobuf_ppo = (struct Pl_utl_iobuf*) buf_p;
    *buf_ppo = (char*) buf_p;
}

// Free memory. Intended for use only during shutdown
static void iobuf_free(struct Pl_utl_iobuf *_buf_p)
{
    void *buf_p = (void*) _buf_p;
    
    ss_assert(buf_p);
    free(buf_p);
}


void pl_utl_vbuf_alloc(Pl_utl_vbuf *vbuf_p, int len)    
{
    pl_utl_cfg.funs.iobuf_alloc_f(
        len,
        &vbuf_p->iobuf_p, 
        &vbuf_p->data_p);
    vbuf_p->len = len;
    vbuf_p->orig_len = len;
    vbuf_p->orig_data_p = vbuf_p->data_p;
}

#if 0
// Special alignment of a v-buf
void pl_utl_iobuf_spec_align(Pl_utl_vbuf *vbuf_p,
                             int len_i,
                             int special_align_i)
{
    // This call cannot be made in a real GPFS configuration
    ss_assert(!(pl_utl_cfg.gpfs && !pl_utl_cfg.hns));
        
    // record the original data
    vbuf_p->orig_data_p = vbuf_p->data_p;
    vbuf_p->orig_len = vbuf_p->len;

    // offset things
    vbuf_p->data_p += special_align_i;
    vbuf_p->len = len_i;

    /* Here we actually touch the pointer used for IO.
     * This can be done only in a non-real GPFS configuration
     */
    vbuf_p->iobuf_p =
        (struct Pl_utl_iobuf*) (((int)vbuf_p->iobuf_p) + special_align_i);
}

void pl_utl_vbuf_reset(Pl_utl_vbuf *vbuf_p)
{
    ss_assert(!(pl_utl_cfg.gpfs && !pl_utl_cfg.hns));

    vbuf_p->data_p = vbuf_p->orig_data_p;
    vbuf_p->len = vbuf_p->orig_len;
    vbuf_p->iobuf_p = (struct Pl_utl_iobuf*) vbuf_p->orig_data_p;
}

void pl_utl_vbuf_free(Pl_utl_vbuf *vbuf_p)
{
    pl_utl_cfg.funs.iobuf_free_f(vbuf_p->iobuf_p);
}
#endif

#define CASE(s) case s: return #s ; break

const char* pl_utl_string_of_rw(Pl_utl_rw rw)
{
    switch(rw) {
        CASE(PL_UTL_READ);
        CASE(PL_UTL_WRITE);

    default:
        ERR(("bad value for [rw]"));
    }
}

#undef CASE

/* Perform read/write with buffer [_buf_p] of length [len] to/from
 * disk [disk_p] at offset [ofs]. 
 * The [rw] flag determines if this is
 * a read or a write.
 *
 * Return a status to say if the IO succeeded or failed. 
 */
static Pl_utl_io_rc disk_rw(Pl_utl_disk_desc *disk_p,
                            struct Pl_utl_iobuf *_buf_p, 
                            Pl_utl_rw rw,
                            long long disk_addr,
                            int len,
                            int buf_ofs)
{
#if 0
    int fd = disk_p->fd;
    char *buf_p = (char*) _buf_p;
    int rc, accu;
    
    pl_trace(3, PL_EV_IO,
             "%s fd=%d disk_addr=%Ld len=%d buf_ofs=%d buf_p=%p",
             pl_utl_string_of_rw(rw), fd,
             disk_addr, len, buf_ofs, buf_p);

    /* The [buf_ofs] argument can be non sector-aligned. This
     * can happen if the original IO request was on a byte-range.
     * For example, a write to object X in range [2214,32187]
     * will create a non-aligned [buf_ofs].
     */
    //ss_debugassert(0 == buf_ofs % SS_SECTOR_SIZE);
        
    /* A write may either fail or be short. We need a loop
     * to make sure it completes. The expectation is to complete the
     * write in the first iteration. 
     */
    for (accu = 0, rc = 0;  accu < len; accu += rc)
    {
        if (accu > 0)
            pl_trace(
                3, PL_EV_IO_SECOND,
                "disk_addr=%Ld len=%d buf_ofs=%d buf_p=%p accu=%d",
                disk_addr, len, buf_ofs, buf_p, accu);
        
        if (PL_UTL_WRITE == rw)
            rc = pwrite64(fd, buf_p + buf_ofs + accu, len - accu,
                          disk_addr + accu);
        else
            rc = pread64(fd, buf_p + buf_ofs + accu, len - accu,
                         disk_addr + accu);
        
        if (rc <= 0) {
            // There was an IO error

            switch (errno) {
            case EAGAIN: 
            case EINTR:
                rc = 0;
                continue;

            default:
                // Error handling code
                if (disk_p->flags.file_backend &&
                    PL_UTL_READ == rw)
                {
                    /* The actual back-end is a file, and
                     * we got a short-read. We may have read beyond the
                     * end of the file, in which case we need to fill
                     * in zeros. 
                     */
                    memset(buf_p + buf_ofs + accu, 0, len - accu);
                    return PL_UTL_IO_RC_SUCCESS;                
                }
                else {
                    printf("error (rc=%d) (errno=%d) %s\n",
                           rc, errno, strerror(errno));
                    if (EINVAL == errno &&
                        ((int)buf_p + buf_ofs + accu) % SS_SECTOR_SIZE != 0) {
                        printf(
                            "If the file is opened with O_DIRECT,"
                            "then [buf_p + buf_ofs] must be a multiple"
                            "of 512 bytes\n");
                        printf("buf_p=%d buf_ofs=%d\n", (int)buf_p, buf_ofs);
                    }
                    ss_assert(0);
                    
                    return PL_UTL_IO_RC_FAILED;
                }
            }
        }
    }
#endif

    ss_assert(0);
    return PL_UTL_IO_RC_SUCCESS;
}

/**************************************************************/
/**************************************************************/

// Implementation based on pthread-semaphores

typedef struct Pl_utl_sema_os {
    int magic;            // Magic number, for assertions
    sem_t os_semaphore;   // Actual semaphore provided by the OS
} Pl_utl_sema_os;

#define PL_UTL_SEMA_MAGIC (1618033)

static struct Pl_utl_sema *pl_utl_sema_i_create(int init_val)
{
    int rc;
    Pl_utl_sema_os *sema_p;
    
    sema_p = (Pl_utl_sema_os*) pl_mm_malloc(sizeof(Pl_utl_sema_os));
    ss_debugassert(sema_p);
    
    sema_p->magic = PL_UTL_SEMA_MAGIC;
    rc = sem_init(&sema_p->os_semaphore,
                  0,
                  init_val);
    
    if (rc != 0) {
        switch (errno) {
        case EINVAL:
            printf("value exceeds the maximal counter value SEM_VALUE_MAX\n");
            break;
        case ENOSYS:
            printf("pshared is not zero\n");
            break;
        }
        
        ERR(("sem_init failed (errno=%d) %s", rc, strerror(rc)));
    }
    
    return (struct Pl_utl_sema*) sema_p;
}

static void pl_utl_sema_i_wait(struct Pl_utl_sema *_sema_p)
{
    int rc = 0;
    Pl_utl_sema_os *sema_p = (Pl_utl_sema_os*) _sema_p;
    
    ss_debugassert(sema_p);
    ss_debugassert(PL_UTL_SEMA_MAGIC == sema_p->magic);

    rc = sem_wait(&sema_p->os_semaphore);
    if (rc != 0)
       ERR(("sem_wait failed (errno=%d) %s", rc, strerror(rc)));
}


static void pl_utl_sema_i_post(struct Pl_utl_sema *_sema_p)
{
    int rc;
    Pl_utl_sema_os *sema_p = (Pl_utl_sema_os*) _sema_p;
    
    ss_debugassert(sema_p);
    ss_debugassert(PL_UTL_SEMA_MAGIC == sema_p->magic);
    rc = sem_post(&sema_p->os_semaphore);
    if (rc != 0)
        ERR(("sem_wait failed (errno=%d) %s", rc, strerror(rc)));
}

static void pl_utl_sema_i_destroy(struct Pl_utl_sema *_sema_p)
{
    int rc;
    Pl_utl_sema_os *sema_p = (Pl_utl_sema_os*) _sema_p;
    
    ss_debugassert(sema_p);
    ss_debugassert(PL_UTL_SEMA_MAGIC == sema_p->magic);
    sema_p->magic = -1;
    rc = sem_destroy(&sema_p->os_semaphore);
    if (rc != 0)
        ERR(("sem_wait failed (errno=%d) %s", rc, strerror(rc)));

    // return the semaphore to the pool 
    // pl_mm_pool_free(sema_pool_p, (void*)sema_p);
    pl_mm_free(sema_p);
}


/**************************************************************/
/**************************************************************/
// Implementation of a binary-semaphore using a regular semaphore

static struct Pl_utl_bsema *pl_utl_bsema_i_create(void)
{
    return (struct Pl_utl_bsema*) pl_utl_sema_i_create(0);
}

static void pl_utl_bsema_i_wait(struct Pl_utl_bsema *_bsema_p)
{
    pl_utl_sema_i_wait((struct Pl_utl_sema*) _bsema_p);
}

static void pl_utl_bsema_i_post(struct Pl_utl_bsema *_bsema_p)
{
    pl_utl_sema_i_post((struct Pl_utl_sema*) _bsema_p);    
}

static void pl_utl_bsema_i_destroy(struct Pl_utl_bsema *_bsema_p)
{
    pl_utl_sema_i_destroy((struct Pl_utl_sema*) _bsema_p);    
}

/**************************************************************/
/**************************************************************/
// Mutex implementation based on pthread-mutexes

typedef struct Pl_utl_mutex_os {
    int magic;              // Magic number, for assertions
    pthread_mutex_t mutex;  // Actual mutex provided by pthreads
} Pl_utl_mutex_os;

#define PL_UTL_MUTEX_MAGIC (21778899)

// Initialize a pthread lock
static struct Pl_utl_mutex *pl_utl_mutex_i_create(void)
{
    pthread_mutexattr_t attr;
    Pl_utl_mutex_os *m_p;

    m_p = (Pl_utl_mutex_os*) pl_mm_malloc(sizeof(Pl_utl_mutex_os));
    ss_assert(m_p);
    m_p->magic = PL_UTL_MUTEX_MAGIC;
    
    if (pthread_mutexattr_init(&attr))
        ERR(("mutexattr_init"));
    //if (pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK_NP))
    //    ERR(("mutexattr_settype"));
    if (pthread_mutex_init(&m_p->mutex, &attr))
        ERR(("mutex_init"));
    if (pthread_mutexattr_destroy(&attr))
        ERR(("mutexattr_destroy"));

    return (struct Pl_utl_mutex*)m_p;
}

static void pl_utl_mutex_i_destroy(struct Pl_utl_mutex *_m_p)
{
    Pl_utl_mutex_os *m_p = (Pl_utl_mutex_os *) _m_p;
    ss_debugassert(m_p) ;
    ss_debugassert(PL_UTL_MUTEX_MAGIC == m_p->magic);
    
    if (pthread_mutex_destroy(&m_p->mutex))
        ERR(("pl_spin_destroy: pthread_mutex_destroy"));

    free(m_p);
}

static void pl_utl_mutex_i_lock(struct Pl_utl_mutex *_m_p)
{
    int ern;
    Pl_utl_mutex_os *m_p = (Pl_utl_mutex_os *) _m_p;

    ss_debugassert(m_p) ;
    ss_debugassert(m_p) ;
    
    if ((ern = pthread_mutex_lock(&m_p->mutex))) 
        ERR(("pthread_mutex_lock: (%d) %s", ern, strerror(ern))) ; 
}

static void pl_utl_mutex_i_unlock(struct Pl_utl_mutex *_m_p)
{
    int ern;
    Pl_utl_mutex_os *m_p = (Pl_utl_mutex_os *) _m_p;

    ss_debugassert(m_p) ; 
    ss_debugassert(PL_UTL_MUTEX_MAGIC == m_p->magic);

    if ((ern = pthread_mutex_unlock(&m_p->mutex)))
        ERR(("pthread_mutex_unlock: (%d) %s", ern, strerror(ern))) ; 
}

static int pl_utl_mutex_i_test_lock(struct Pl_utl_mutex *_m_p)
{
    int rc;
    Pl_utl_mutex_os *m_p = (Pl_utl_mutex_os *) _m_p;
    
    ss_debugassert(m_p);
    ss_debugassert(PL_UTL_MUTEX_MAGIC == m_p->magic);
    
    rc = pthread_mutex_trylock(&m_p->mutex);

    if (EBUSY == rc) return 1 /*TRUE*/;

    pthread_mutex_unlock(&m_p->mutex);
    return 0 /*FALSE*/;
}

/**************************************************************/
/**************************************************************/

// Set a global variable stating that this is a GPFS configuration
void pl_utl_set_gpfs(void)
{
    printf("====================\n"); 
    printf("PL_UTL: GPFS mode\n");
    printf("====================\n");    

    pl_utl_cfg.gpfs = 1;
}

void pl_utl_set_sim(void)
{
    printf("====================\n"); 
    printf("PL_UTL: Simulator mode\n");
    printf("====================\n");    

    pl_utl_cfg.sim = 1;
}

void pl_utl_set_hns(void)
{
    printf("====================\n"); 
    printf("PL_UTL: Harness mode\n");
    printf("====================\n");    

    pl_utl_cfg.hns = 1;
}

void pl_utl_set_funs(Pl_utl_funs *funs_p)
{
    ss_assert(funs_p != NULL);
    pl_utl_cfg.funs = *funs_p;
}

/**************************************************************/

const char* pl_utl_string_of_target_type(void)
{
    static char *target_str = "real";
    static char *sim_str = "sim";
    static char *hns_str = "hns";
    static char *hnsim_str = "hnsim";
    
    if (pl_utl_cfg.hns)
    {
        if (pl_utl_cfg.sim)
            return hnsim_str;
        else 
            return hns_str;
    }
    else {
        if (pl_utl_cfg.sim)
            return sim_str;
        else 
            return target_str;
    }
}

/**************************************************************/

