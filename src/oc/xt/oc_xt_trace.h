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
#ifndef OC_XT_TRACE_H
#define OC_XT_TRACE_H

struct Oc_wu;

typedef enum Oc_xt_trace_event {
    OC_EV_XT_CREATE,
    OC_EV_XT_DELETE,
    OC_EV_XT_COW_ROOT_AND_UPDATE,    
    OC_EV_XT_LOOKUP_RANGE,
    OC_EV_XT_INSERT_RANGE,
    OC_EV_XT_REMOVE_RANGE,
    OC_EV_XT_ATTR_GET,
    OC_EV_XT_ATTR_SET,       
    OC_EV_XT_QUERY,
    OC_EV_XT_INIT_CFG,
    OC_EV_XT_DESTROY_STATE,
    
    OC_EV_XT_LEAF_SPLIT,
    OC_EV_XT_ROOT_SPLIT,
    OC_EV_XT_INDEX_SPLIT,
    OC_EV_XT_REBALANCE,
    OC_EV_XT_REBALANCE2,
    OC_EV_XT_REPLACE_MIN_IN_INDEX,
    OC_EV_XT_INSERT_ITER,
    OC_EV_XT_INSERT_SRC_IN_INDEX,
    OC_EV_XT_ND_INDEX_LOOKUP,
    OC_EV_XT_ND_SPLIT,
    OC_EV_XT_ND_MERGE_AND_DEALLOC,
    OC_EV_XT_COPY_INTO_ROOT,
    OC_EV_XT_RMV_KEY,
    OC_EV_XT_LOOKUP_RNG,
    OC_EV_XT_LOOKUP_RNG_SEARCH_LEAF,
    OC_EV_XT_LOOKUP_RNG_SIMPLE_DECENT,
    OC_EV_XT_LOOKUP_RNG_MINI,
    OC_EV_XT_LOOKUP_RNG_MINI_ITER,
    OC_EV_XT_LOOKUP_RNG_MINI_SPLIT,
    OC_EV_XT_INSERT_RNG,
    OC_EV_XT_LEAF_REMOVE_RNG,
    OC_EV_XT_LEAF_REMOVE_RNG_2,
    OC_EV_XT_INDEX_REMOVE_RNG,
    OC_EV_XT_ND_INSERT_INTO_LEAF,
    OC_EV_XT_ND_INSERT_INTO_LEAF_DONE,
    OC_EV_XT_ND_INSERT_INTO_LEAF_UNDERFLOW_FIX,
    OC_EV_XT_ND_RMV_KEY_RNG,
    OC_EV_XT_ND_MOVE_MIN_KEY,
    OC_EV_XT_ND_MOVE_MAX_KEY,
    OC_EV_XT_ND_NEW_LEAF_ENTRY,
    OC_EV_XT_INSERT_RNG_LEN,
    OC_EV_XT_REMOVE_PART_1,
    OC_EV_XT_REMOVE_PART_2,
    OC_EV_XT_FILL_SINGLE_LEAF_1,
    OC_EV_XT_FILL_SINGLE_LEAF_2,
    OC_EV_XT_FILL_SINGLE_LEAF_3,

    OC_EV_XT_REMOVE_RNG,
} Oc_xt_trace_event;

#if OSD_DEBUG
void oc_xt_trace_wu_lvl(int level,
                         Oc_xt_trace_event ev,
                         struct Oc_wu *wu_pi,
                         const char *fmt_p,
                         ...);
#else
static inline void oc_xt_trace_wu_lvl(int level,
                                       Oc_xt_trace_event ev,
                                       struct Oc_wu *wu_pi,
                                       const char *fmt_p,
                                       ...)
{}
#endif

#endif
