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
/**********************************************************************************/
/* Tracing facilities for OC/UTL module
 */
/**********************************************************************************/
#ifndef OC_UTL_TRACE_H
#define OC_UTL_TRACE_H

#include <stdarg.h>

struct Oc_wu;

typedef enum Oc_utl_trace_event {
    OC_EV_UTL_PG_FREE,
    OC_EV_UTL_PG_ALLOC_STRICT,
    OC_EV_UTL_PG_ALLOC_B,
    OC_EV_UTL_PG_ALLOC_CORE,
    OC_EV_UTL_PG_ALLOC_ARRAY_B,
    OC_EV_UTL_PG_INC,
    OC_EV_UTL_PG_DEC,
    
    OC_EV_UTL_PG_SIGNAL,
    OC_EV_UTL_PG_SIGNAL_OFF,

    OC_EV_UTL_ATTR_INIT, 
    OC_EV_UTL_ATTR_GET,
    OC_EV_UTL_ATTR_SET,

    OC_EV_UTL_VBUF_POOL_INIT,
    OC_EV_UTL_CA_RACE_COND,
    
} Oc_utl_trace_event;
        
#if OC_DEBUG

void oc_utl_trace_wu_lvl(int level,
                         Oc_utl_trace_event ev_i, 
                         struct Oc_wu *wu_pi,
                         const char *fmt_p,
                         ...);

#else
static inline void oc_utl_trace_wu_lvl(int level,
                                       Oc_utl_trace_event ev_i, 
                                       struct Oc_wu *wu_pi,
                                       const char *fmt_p,
                                       ...) {}

#endif

#endif  /* __OC_TRACE_H_ */
