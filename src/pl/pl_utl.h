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
 *
 * When using GPFS, we -must- use its IO functions, locking
 * primitives, memory allocation etc. However, we do not want to
 * import the declarations due to namespace pollution
 * concerns. Furthermore, GPFS is written in C++ and the OSD is
 * written in C. The upshot is that various abstraction techniques are
 * used.
 *
 * This file is also included by GPFS. Therefore, it should not
 * depend on any other header file. Otherwise, the included files
 * also need to be copied to the GFPS include directory. 
 */
/**************************************************************/
#ifndef PL_UTL_H
#define PL_UTL_H

// A description of a disk 
#define PL_UTL_DEVICE_MAX_PATH (60)

// Abstraction for hiding a GPFS representation of a disk
struct Pl_utl_disk_hndl;

typedef struct Pl_utl_disk_desc {
    unsigned long long total_sectors;
    // First physical sector on disk for OC after GPFS has taken its space
    unsigned long long start_physical_sector; 
                                              
    unsigned int sector_size;

    // Computed from the above three 
    unsigned long long size_in_bytes;

    // the name of the disk (or file emulating a disk)     
    char dev_name[PL_UTL_DEVICE_MAX_PATH]; 

    struct {
        unsigned file_backend:1;  // is the device actually a file?
        unsigned fd_open:1;       // has the device already been opened? 
    } flags;

    // A file-descriptor that allows read/write access to the disk    
    int fd;

    // Used by GPFS when performing IO 
    struct Pl_utl_disk_hndl *diskdev_p;
} Pl_utl_disk_desc;



/* A create-thread function.
 *
 * GPFS wants us to use it's own threading functions. 
 */
typedef int (Pl_utl_create_thread_fun)(void* (*rtn_p)(void *), void* param);


/* Abstraction for memory allocation
 */

/* Structure describing a variable-length memory buffer that can be
 * used to perform IO.
 *
 * For example, to perform dircet-IO in Linux 2.6 the buffer has to be
 * sector-aligned. In Linux 2.4 the buffer has to be page-aligned.
 *
 * The buffer may also come from GPFS. In that case, it -must- be
 * used with GPFS IO functions. 
 */
struct Pl_utl_iobuf;

typedef struct Pl_utl_vbuf {
    // to put on the PLMM free-list
    struct Ss_slist_node *node_p;    

    // buffer length
    int len;                  
    
    /* The actual buffer may be longer than [len]. It may extend
     * before and after [data_p, data_p + len].
     * [data_p] points to the real buffer allocated through malloc,
     * or equivalent operating-system library.
     *
     * This pointer is for the vbuf implementation. Do not use
     * it externally.
     */
    char *data_p;

    /* In some cases, the [len] and [data_p] point to a sub-buffer.
     * The [orig_len] and [orig_data_p] point to the original buffer.
     */
    int orig_len;
    char *orig_data_p;
    
    // an external structure that describes the buffer. 
    struct Pl_utl_iobuf *iobuf_p;
} Pl_utl_vbuf;

/* Allocate memory. Intended for use only during initialization.
 * The memory is aligned for IO.
 *   [len_i] is the requested length. It must be sector aligned.
 *   [iobuf_ppo] is an opaque structure representing the io-buffer
 *   [buf_ppo]   is a pointer into the iobuf that is usable by 
 *               the caller
 */
typedef void (Pl_utl_iobuf_alloc)(
    int len_i,
    struct Pl_utl_iobuf **iobuf_ppo,
    char **buf_ppo);

// Free memory. Intended for use only during shutdown.
typedef void (Pl_utl_iobuf_free)(struct Pl_utl_iobuf *_buf_p);



//void pl_utl_vbuf_alloc(Pl_utl_vbuf *vbuf_p, int len);

// Special alignment of a v-buf
/*void pl_utl_iobuf_spec_align(Pl_utl_vbuf *vbuf_p,
                             int len_i,
                             int special_align_i);*/
// eliminate any special alignment 
//void pl_utl_vbuf_reset(Pl_utl_vbuf *vbuf_p);

/* Release a v-buf. Intended for use only during shutdown.
 * Uses the [Pl_utl_vbuf_free] function.
 */
//void pl_utl_vbuf_free(Pl_utl_vbuf *vbuf_p);

/* Abstractions for GPFS provided IO function.
 */
typedef enum Pl_utl_rw {
    PL_UTL_READ,
    PL_UTL_WRITE,
} Pl_utl_rw;

typedef enum Pl_utl_io_rc {
    PL_UTL_IO_RC_SUCCESS,
    PL_UTL_IO_RC_FAILED,
} Pl_utl_io_rc;

/* Function that takes a disk, a buffer, and a description of an IO
 * and performs it. The return code describes wether the IO succeeded
 * or failed. 
 */
typedef Pl_utl_io_rc (Pl_utl_io_fun)(Pl_utl_disk_desc *disk_p,
                                     struct Pl_utl_iobuf *_buf_p, 
                                     Pl_utl_rw rw,
                                     long long disk_addr,
                                     int len,
                                     int buf_ofs);

const char* pl_utl_string_of_rw(Pl_utl_rw rw);


/* Abstraction of a binary-semaphore.
 */
struct Pl_utl_bsema;

typedef struct Pl_utl_bsema* (Pl_utl_bsema_create_fun)(void);
typedef void (Pl_utl_bsema_destroy_fun)(struct Pl_utl_bsema*);
typedef void (Pl_utl_bsema_post_fun)(struct Pl_utl_bsema*);
typedef void (Pl_utl_bsema_wait_fun)(struct Pl_utl_bsema*);

/* Abstraction of full-fledged semaphore.
 */
struct Pl_utl_sema;

typedef struct Pl_utl_sema* (Pl_utl_sema_create_fun)(int);
typedef void (Pl_utl_sema_destroy_fun)(struct Pl_utl_sema*);
typedef void (Pl_utl_sema_post_fun)(struct Pl_utl_sema*);
typedef void (Pl_utl_sema_wait_fun)(struct Pl_utl_sema*);

/* Abstraction of a mutex.
 */
struct Pl_utl_mutex;

typedef struct Pl_utl_mutex* (Pl_utl_mutex_create_fun)(void);
typedef void (Pl_utl_mutex_destroy_fun)(struct Pl_utl_mutex*);
typedef void (Pl_utl_mutex_lock_fun)(struct Pl_utl_mutex*);
typedef void (Pl_utl_mutex_unlock_fun)(struct Pl_utl_mutex*);

// Return 1 if the mutex is locked. Zero otherwise. 
typedef int (Pl_utl_mutex_test_lock_fun)(struct Pl_utl_mutex*);

/* A structure that binds together the set of configurable functions
 */
typedef struct Pl_utl_funs {
    // A create thread function. By default, pthreads are used. 
    Pl_utl_create_thread_fun *create_thread_f;

    // A function that performs disk IO
    Pl_utl_io_fun *io_f;

    // A binary-semaphore. Can count up to 1.
    Pl_utl_bsema_create_fun *bsema_create_f;
    Pl_utl_bsema_destroy_fun *bsema_destroy_f;
    Pl_utl_bsema_post_fun *bsema_post_f;
    Pl_utl_bsema_wait_fun *bsema_wait_f;

    // A full fledged semaphore. 
    Pl_utl_sema_create_fun *sema_create_f;
    Pl_utl_sema_destroy_fun *sema_destroy_f;
    Pl_utl_sema_post_fun *sema_post_f;
    Pl_utl_sema_wait_fun *sema_wait_f;

    // A mutex
    Pl_utl_mutex_create_fun *mutex_create_f;
    Pl_utl_mutex_destroy_fun *mutex_destroy_f;
    Pl_utl_mutex_lock_fun *mutex_lock_f;
    Pl_utl_mutex_unlock_fun *mutex_unlock_f;
    Pl_utl_mutex_test_lock_fun *mutex_test_lock_f;

    // Memory functions
    Pl_utl_iobuf_alloc *iobuf_alloc_f;
    Pl_utl_iobuf_free  *iobuf_free_f;

} Pl_utl_funs;

// Set a global variable stating that this is a GPFS configuration
void pl_utl_set_gpfs(void);

// Set a global variable stating that this is a Simulator configuration
void pl_utl_set_sim(void);

// Set a global variable stating that this is a Harness configuration
void pl_utl_set_hns(void);

void pl_utl_set_funs(Pl_utl_funs *funs_p);


/*
 *  There are now three boolean flags that determine the overall
 *  configuration:
 *    gpfs
 *    hns
 *    sim 
 *
 *  The flags are independent, so, there can be a total of eight
 *  combinations. 
 *    non-gpfs, non-hns, non-sim:  Default configuration
 *    non-gpfs, non-hns, sim:      Simulator connected the ISD
 *    non-gpfs, hns, non-sim:      OC harness 
 *    non-gpfs, hns, sim:          Simulator in a harness mode
 *    gpfs, non-hns, non-sim:      OC in a real-GPFS file-system
 *    gpfs, non-hns, sim:          Does not exist today
 *    gpfs, hns, non-sim:          OC harness with algorithms tailored for GPFS
 *    gpfs, hns, sim:              Does not exist today
 *  The code has to set these flags before initializing OC.
 */

typedef struct Pl_utl_config {
    // Is this a GPFS configuration ?
    int gpfs;
    
    // Is this a Simulator configuration ?
    int sim;
    
    // Is this a Harness configuration ?
    int hns;

    Pl_utl_funs funs;
} Pl_utl_config;

// Globally visible structure that shows the basic system configuration
extern Pl_utl_config pl_utl_cfg;

// return a string representation of the target type
const char* pl_utl_string_of_target_type(void);

/**************************************************************/
// Convenience functions for locks and semaphores.

// Binary semaphore
static inline struct Pl_utl_bsema *pl_utl_bsema_create(void)
{
    return pl_utl_cfg.funs.bsema_create_f();
}

static inline void pl_utl_bsema_destroy(struct Pl_utl_bsema *sema_p)
{
    pl_utl_cfg.funs.bsema_destroy_f(sema_p);
}

static inline void pl_utl_bsema_post(struct Pl_utl_bsema *sema_p)
{
    pl_utl_cfg.funs.bsema_post_f(sema_p);    
}

static inline void pl_utl_bsema_wait(struct Pl_utl_bsema *sema_p)
{
    pl_utl_cfg.funs.bsema_wait_f(sema_p);        
}

// Semaphore
static inline struct Pl_utl_sema* pl_utl_sema_create(int init_val)
{
    return pl_utl_cfg.funs.sema_create_f(init_val);    
}

static inline void pl_utl_sema_destroy(struct Pl_utl_sema *sema_p)
{
    pl_utl_cfg.funs.sema_destroy_f(sema_p);
}

static inline void pl_utl_sema_post(struct Pl_utl_sema *sema_p)
{
    pl_utl_cfg.funs.sema_post_f(sema_p);
}

static inline void pl_utl_sema_wait(struct Pl_utl_sema *sema_p)
{
    pl_utl_cfg.funs.sema_wait_f(sema_p);            
}

// Mmutex
static inline struct Pl_utl_mutex* pl_utl_mutex_create(void)
{
    return pl_utl_cfg.funs.mutex_create_f();
}

static inline void pl_utl_mutex_destroy(struct Pl_utl_mutex *mutex_p)
{
    pl_utl_cfg.funs.mutex_destroy_f(mutex_p);
}

static inline void pl_utl_mutex_lock(struct Pl_utl_mutex *mutex_p)
{
    pl_utl_cfg.funs.mutex_lock_f(mutex_p);    
}

static inline void pl_utl_mutex_unlock(struct Pl_utl_mutex *mutex_p)
{
    pl_utl_cfg.funs.mutex_unlock_f(mutex_p);        
}


static inline int pl_utl_mutex_test_lock(struct Pl_utl_mutex *mutex_p)
{
    return pl_utl_cfg.funs.mutex_test_lock_f(mutex_p);        
}

/**************************************************************/
#endif
