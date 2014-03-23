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
/***************************************************************************/
#ifndef PL_MM_INT_H
#define PL_MM_INT_H

#include "pl_base.h"     // For uint32, etc.
#include "pl_dstru.h"     // For linked lists

typedef struct Pl_mm_op_s Pl_mm_op; // Private to pl_mm

/***************************************************************************/
/* INITIALISATION AND SHUTDOWN                                             */
/***************************************************************************/

void pl_mm_init(void);
void pl_mm_shutdown( void );

/***************************************************************************/
/* raw allocation/deallocation.
 * Should be used only during initialization.
 */
void* pl_mm_malloc (int size);
void  pl_mm_free(void *ptr);

/***************************************************************************/
/* Functions to create and delete object pools
 */

typedef enum {
  PL_MM_ALIGN_8BITS     = 1,
  PL_MM_ALIGN_16BITS    = 2,
  PL_MM_ALIGN_32BITS    = 4,
  PL_MM_ALIGN_64BITS    = 8,
  PL_MM_ALIGN_2BYTES    = 2,
  PL_MM_ALIGN_4BYTES    = 4,
  PL_MM_ALIGN_8BYTES    = 8,
  PL_MM_ALIGN_16BYTES   = 16,
  PL_MM_ALIGN_32BYTES   = 32,
  PL_MM_ALIGN_CACHELINE = 32,
  PL_MM_ALIGN_DEFAULT   = 4,
} Pl_mm_alignment_t;

bool pl_mm_pool_create( uint32 size_i,
                        Pl_mm_alignment_t align_i,
                        uint32 number_i,
                        void (*init_object_i)( void *object_i ),
                        Pl_mm_op **pool_o );

// destroy all objects in pool and release pool resources
void pl_mm_pool_delete( Pl_mm_op *pool_p,
                        void (*destroy_object_fun)( void *object_fun ) );

/***************************************************************************/
/* reserve (and allocate) from object pools
 */
bool pl_mm_pool_alloc( Pl_mm_op *pool_i,
                                             void **object_o );

/* allocate, free and unreserve from object pools
 */
void pl_mm_pool_free( Pl_mm_op *pool_i, void *object_i );

/***************************************************************************/

#endif


