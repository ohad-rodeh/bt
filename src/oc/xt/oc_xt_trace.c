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
/**********************************************************************/
/* OC_XT_TRACE.C : code that handles tracing.
 */
/**********************************************************************/
#include "oc_xt_trace.h"
#include "oc_utl_trace_base.h"
#include "pl_base.h"
#include "oc_wu_s.h"
/**********************************************************************/

#define CASE(s) case s: return #s ; break

const char *oc_xt_string_of_trace_event(int ev_i)
{
    switch (ev_i) {
        CASE(OC_EV_XT_CREATE);
        CASE(OC_EV_XT_DELETE);
        CASE(OC_EV_XT_COW_ROOT_AND_UPDATE);        
        CASE(OC_EV_XT_LOOKUP_RANGE);
        CASE(OC_EV_XT_INSERT_RANGE);
        CASE(OC_EV_XT_REMOVE_RANGE);
        CASE(OC_EV_XT_ATTR_GET);
        CASE(OC_EV_XT_ATTR_SET);
        
        
        CASE(OC_EV_XT_QUERY);
        CASE(OC_EV_XT_INIT_CFG);
        CASE(OC_EV_XT_DESTROY_STATE);
        
        CASE(OC_EV_XT_LEAF_SPLIT);
        CASE(OC_EV_XT_ROOT_SPLIT);
        CASE(OC_EV_XT_INDEX_SPLIT);
        CASE(OC_EV_XT_REBALANCE);
        CASE(OC_EV_XT_REBALANCE2);
        CASE(OC_EV_XT_REPLACE_MIN_IN_INDEX);
        CASE(OC_EV_XT_INSERT_ITER);
        CASE(OC_EV_XT_INSERT_SRC_IN_INDEX);
        CASE(OC_EV_XT_ND_INDEX_LOOKUP);
        CASE(OC_EV_XT_ND_SPLIT);
        CASE(OC_EV_XT_ND_MERGE_AND_DEALLOC);
        CASE(OC_EV_XT_COPY_INTO_ROOT);
        CASE(OC_EV_XT_RMV_KEY);
        CASE(OC_EV_XT_LOOKUP_RNG);
        CASE(OC_EV_XT_LOOKUP_RNG_SEARCH_LEAF);
        CASE(OC_EV_XT_LOOKUP_RNG_SIMPLE_DECENT);
        CASE(OC_EV_XT_LOOKUP_RNG_MINI);
        CASE(OC_EV_XT_LOOKUP_RNG_MINI_ITER);
        CASE(OC_EV_XT_LOOKUP_RNG_MINI_SPLIT);
        CASE(OC_EV_XT_INSERT_RNG);
        CASE(OC_EV_XT_LEAF_REMOVE_RNG);
        CASE(OC_EV_XT_LEAF_REMOVE_RNG_2);
        CASE(OC_EV_XT_INDEX_REMOVE_RNG);
        CASE(OC_EV_XT_ND_INSERT_INTO_LEAF);
        CASE(OC_EV_XT_ND_INSERT_INTO_LEAF_DONE);
        CASE(OC_EV_XT_ND_INSERT_INTO_LEAF_UNDERFLOW_FIX);
        CASE(OC_EV_XT_ND_RMV_KEY_RNG);
        CASE(OC_EV_XT_ND_MOVE_MIN_KEY);
        CASE(OC_EV_XT_ND_MOVE_MAX_KEY);
        CASE(OC_EV_XT_ND_NEW_LEAF_ENTRY);
        CASE(OC_EV_XT_INSERT_RNG_LEN);
        CASE(OC_EV_XT_REMOVE_PART_1);
        CASE(OC_EV_XT_REMOVE_PART_2);
        CASE(OC_EV_XT_FILL_SINGLE_LEAF_1);
        CASE(OC_EV_XT_FILL_SINGLE_LEAF_2);
        CASE(OC_EV_XT_FILL_SINGLE_LEAF_3);

        CASE(OC_EV_XT_REMOVE_RNG);
    default:
        ERR(("no such case"));
    }
}

#if OSD_DEBUG
void oc_xt_trace_wu_lvl(int level,
                         Oc_xt_trace_event ev,
                         Oc_wu *wu_pi,
                         const char *fmt_p,
                         ...)
{
    uint32 req_id = 0;
    uint32 po_id = 0;
    
    if (osd_trace_is_set(OSD_TRACE_OC_XT, level)) {
        va_list args;
        char buf [100];
        
        va_start(args, fmt_p);
        vsnprintf(buf, 100, fmt_p, args);
        va_end(args);
        
        if (wu_pi) {
            req_id = wu_pi->req_id;
            po_id = wu_pi->po_id;
        } 
        
        osd_trace(OSD_TRACE_OC_XT,
                  level,
                  "(req=%lu, po=%lu, %s, %s)\n",
                  req_id, po_id, 
                  oc_xt_string_of_trace_event(ev),
                  buf);
    }
}
#endif
