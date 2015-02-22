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
/****************************************************************************/
// Base tracing functions for the OC component
/****************************************************************************/
#include <memory.h>
#include <pthread.h>

#include "pl_base.h"
#include "pl_base.h"
#include "oc_crt_int.h"
#include "pl_trace_base.h"
#include "oc_utl_trace_base.h"
#include "oc_utl.h"
/****************************************************************************/
#if OC_DEBUG
static void oc_utl_trace_full( bool check_thrd_id_i,
                               Pl_trace_base_tag tag_i,
                               int level,
                               Oc_wu *wu_pi,
                               const char *fmt_p,
                               va_list args );

void oc_utl_trace_base_wu( bool check_thrd_id_i,
                           Pl_trace_base_tag tag_i,
                           int level,
                           Oc_wu *wu_pi,
                           const char *fmt_p,
                           ... )
{
    va_list args;

    va_start(args, fmt_p);
    oc_utl_trace_full(check_thrd_id_i, tag_i, level, wu_pi, fmt_p, args);
    va_end(args);
}

void oc_utl_trace_base_wu_lvl(bool check_thrd_id_i,
                              Pl_trace_base_tag tag_i,
                              int level,
                              struct Oc_wu *wu_pi,
                              const char *fmt_p,
                              ...)
{
    va_list args;

    va_start(args, fmt_p);
    oc_utl_trace_full(check_thrd_id_i, tag_i, level, wu_pi, fmt_p, args);
    va_end(args);    
}

void oc_utl_trace_full( bool check_thrd_id_i,
                        Pl_trace_base_tag tag_i,
                        int level,
                        Oc_wu *wu_pi,
                        const char *fmt_p,
                        va_list args )
{
    uint32 req_id = 0;
    uint32 po_id = 0;
    static int oc_crt_thread_id = 0;
    
    if (wu_pi) {
        req_id = wu_pi->req_id;
        po_id = wu_pi->po_id;
    }
    else {
        /* If there is no work-unit, we can't check
         * the correctness of the thread-id
        */
        check_thrd_id_i = FALSE;
    }

    if (check_thrd_id_i) {
        // Read the co-routine thread_id, if it hasn't been read so far.
        if (0 == oc_crt_thread_id)
            oc_crt_thread_id = oc_crt_get_thread ();
        
        // Make sure we are on the correct thread
        if (check_thrd_id_i) {
            oc_utl_assert(0 == oc_crt_thread_id ||
                          (int)pthread_self() == oc_crt_thread_id);
            oc_crt_assert();
        }
    }
    
    if (pl_trace_base_is_set(tag_i, level)) {
        // We are assuming here that 100 bytes are sufficient here. 
        char buf[100];
        
        vsnprintf(buf, 100, fmt_p, args);
        pl_trace_base(tag_i,
                  level,          // level
                  "(req=%lu, po=%lu, %s)\n",
                  req_id, 
                  po_id,
                  buf);
    }
}

#endif

    
