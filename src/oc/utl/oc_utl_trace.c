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

/****************************************************************************/
#include <memory.h>
#include "pl_trace_base.h"
#include "pl_base.h"
#include "oc_utl.h"

#include "oc_utl_trace_base.h"
#include "oc_utl_trace.h"
#include "oc_utl.h"

/****************************************************************************/
#if OC_DEBUG

// Implementation of tracing for the UTL module itself

#define CASE(s) case s: return #s ; break

const char *oc_utl_string_of_trace_event(int ev_i)
{
    
    switch(ev_i) {
        CASE(OC_EV_UTL_PG_FREE);
        CASE(OC_EV_UTL_PG_ALLOC_STRICT);
        CASE(OC_EV_UTL_PG_ALLOC_B);
        CASE(OC_EV_UTL_PG_ALLOC_CORE);
        CASE(OC_EV_UTL_PG_INC);
        CASE(OC_EV_UTL_PG_DEC);
        CASE(OC_EV_UTL_PG_ALLOC_ARRAY_B);
        
        CASE(OC_EV_UTL_PG_SIGNAL);
        CASE(OC_EV_UTL_PG_SIGNAL_OFF);

        CASE(OC_EV_UTL_ATTR_INIT);
        CASE(OC_EV_UTL_ATTR_GET);
        CASE(OC_EV_UTL_ATTR_SET);
        
        CASE(OC_EV_UTL_VBUF_POOL_INIT);

        CASE(OC_EV_UTL_CA_RACE_COND);

    default:
        ERR(("bad state %d",ev_i ));        
    }
}

#undef CASE

void oc_utl_trace_wu_lvl(int level,
                         Oc_utl_trace_event ev_i, 
                         struct Oc_wu *wu_pi,
                         const char *fmt_p,
                         ...)
{
    va_list args;

    if (pl_trace_base_is_set(PL_TRACE_BASE_OC_UTL, level))
    {
        // We are assuming here that 100 bytes are sufficient here. 
        char buf[100];
        
        va_start(args, fmt_p);
        vsnprintf(buf, 100, fmt_p, args);
        oc_utl_trace_base_wu_lvl(TRUE, PL_TRACE_BASE_OC_UTL, level,  wu_pi,
                                 "%s %s", oc_utl_string_of_trace_event(ev_i), buf);
        va_end(args);
    }
}

#endif

/****************************************************************************/
    
