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
/* PL_BASE.H
 */

#ifndef PL_BASE_H
#define PL_BASE_H

#include <assert.h>
#include <stdio.h>
#include <stdint.h>


#define ss_assert assert

#if OC_DEBUG
#define ss_debugassert(cond) assert(cond)
#else
#define ss_debugassert(cond)
#endif

#define ERR(msg) { printf("\n"); printf msg; printf("\n"); fflush(stdout); ss_assert(0);}

// Constants
#define KB (1024)
#define MB (KB*KB)
#define GB (MB*KB)

#define SS_PAGE_SIZE        4096
#define SS_SECTOR_SIZE      512
#define SS_SECTORS_PER_PAGE 8

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef offsetof
#define offsetof(TYPE,MEMBER) ((uint32) &((TYPE *)0)->MEMBER)
#endif

#ifndef NULL
#if defined(__cplusplus)
#define NULL 0
#else
#define NULL ((void *)0)
#endif
#endif

// Types
typedef unsigned char      uchar;
typedef signed   char      int8;
typedef unsigned char      uint8; 
typedef signed   short     int16;
typedef unsigned short     uint16;   
typedef signed   long      int32;
typedef unsigned long      uint32;   
/*  This atrib must be left off until we resolve alignment issues   */
typedef signed long long   int64;   /* __attribute__((aligned(8))); */
typedef unsigned long long uint64;  /* __attribute__((aligned(8))); */

#ifndef __cplusplus
typedef int32              bool;
#endif

typedef uint8              bool8;

#endif



