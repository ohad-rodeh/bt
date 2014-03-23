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
/***************************** start_copyright_notice_OCO_2005	
 * IBM Confidential
 * OCO Source Materials
 * (C) Copyright IBM Corp. 2005
 * The source code for this program is not published or otherwise
 * divested of its trade secrets, irrespective of what has
 * been deposited with the U.S. Copyright Office.
end_copyright_notice_OCO_2005
 ***********************************************************/
/****************************************************************************/
// Trace functions tailored for PL
/****************************************************************************/
#include <memory.h>
#include "pl_base.h"
#include "pl_trace_base.h"
#include "pl_trace.h"

/****************************************************************************/
#if OC_DEBUG

#define CASE(s) case s: return #s ; break

static char *string_of_ev(Pl_trace_event ev)
{
    switch(ev) {
        CASE(PL_EV_CREATE_PTHREAD);

        CASE(PL_EV_SEMA_PIPE_POST);
        CASE(PL_EV_SEMA_PIPE_WAIT);
        
        CASE(PL_EV_IO);
        CASE(PL_EV_IO_SECOND);
    default:
        ERR(("bad event (%d)", ev));
    }
}

#undef CASE

void pl_trace( int level,
               Pl_trace_event ev,
               const char *fmt_p,
               ...)
{
    if (pl_trace_base_is_set(PL_TRACE_BASE_PL, level)) {
        va_list args;
        char buf [100];
        
        va_start(args, fmt_p);
        vsprintf(buf, fmt_p, args);
        va_end(args);
        
        pl_trace_base(PL_TRACE_BASE_PL, level, "TRACE (%s, %s)\n", string_of_ev(ev), buf);
    }
}
#endif

/****************************************************************************/
