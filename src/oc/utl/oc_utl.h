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
/*
 * Description: misc utility functions,
 *    basic memory functions and pools
 *    basic locking idioms
 *    etc...
*/
/******************************************************************/

#ifndef OC_UTL_H
#define OC_UTL_H

#include "pl_dstru.h"
#include "oc_wu_s.h"
#include "oc_utl_s.h"
#include "oc_config.h"
#include "pl_mm_int.h"

// The list of OC subcomponents
typedef enum {
    // Utility, making sure nobody uses component zero by mistake    
    OC_MIN_COMP = 0, 

    OC_CRT,     // Co-routines. 
    OC_IO,      // The IO client
    OC_FS,      // The free-space 
    OC_BT,      // The b-tree
    OC_SN,      // The s-node
    OC_CAT,     // The catalog
    OC_OCT,     // object catalog
    OC_PCT,     // partition catalog
    OC_RM,      // The resource-manager
    OC_SM,      // The state-machine
    OC_OH,      // The object-handle
    OC_PH,      // The partition-handle
    OC_RT,      // Running Table
    OC_PM,      // Page-Manager
    OC_JL,      // Journal
    OC_RC,      // The IO client

    OC_MAX_COMP, // Utility
} Oc_subcomponent_id;

/* Anonymous struct for defining the work-unit. The actual Oc_wu
 * is defined oc_sm_s.h
 */
struct Oc_wu;

// The configuation structure
extern Oc_utl_config oc_utl_conf_g;

typedef enum Oc_atomic {
    OC_ATOMIC = 1,
    OC_NON_ATOMIC, 
} Oc_atomic;

typedef void*(*thread_routine_t)(void*);

// A way to hide a FILE structure from <stdio.h>
struct Oc_utl_file;

/* Asserts used for the OC components
 */
#define oc_utl_assert( expr ) assert(expr)

#ifdef OC_DEBUG
#define oc_utl_debugassert( expr ) oc_utl_assert( expr )
#else
#define oc_utl_debugassert( expr )
#endif

#ifndef MAX
#define MAX(x,y) ((x) < (y) ? (y) : (x))
#endif

#ifndef MIN
#define MIN(x,y) ((x) > (y) ? (y) : (x))
#endif

void oc_utl_default_config(Oc_utl_config *conf_po);
void oc_utl_init(void);
void oc_utl_init_full(Oc_utl_config *conf_pi);

void oc_utl_free_resources();

/** Initialize a 32-bit Linear Redundancy Check */
static inline uint32 oc_utl_lrc_init(void)
{
    return 0;
}

/** Update a 32-bit Linear Redundancy Check */
uint32 oc_utl_lrc_update(uint32 lrc, char * buf, int len);

const char *oc_utl_string_of_subcomponent_id(Oc_subcomponent_id id);

uint64 oc_query_input_lun_size(uint32 lun);

uint64 oc_utl_get_req_id(void);

/******************************************************************/

/* Compute the log_2 of [num]. For example,
 *    log_2(4) = 2
 *    log_2(7) = 2
 *    log_2(8) = 3
 *    log_2(9) = 3
 */
uint32 oc_utl_log2(uint32 num);

/******************************************************************/

#endif
