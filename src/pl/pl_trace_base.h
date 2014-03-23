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
/****************************************************************/
// Basic tracing facility
/****************************************************************/
/* To use this API properly, do: 
 *
 * 1) Initialize:
 *     pl_trace_base_init
 * 2) set the level, and add tag-level pairs:
 *     pl_trace_base_add_string_tag_full, pl_trace_base_set_level.
 * 3) pl_trace_base_init_done
 *     complete setting the trace-tags.
 *
 * The above set of operations is intended to be performed while
 * reading the command line arguments.
 *
 * None of the APIs in this module are thread-safe. 
 */

#ifndef PL_TRACE_BASE_H
#define PL_TRACE_BASE_H

#include <stdarg.h>

/* Enumerated list of tags.
 *
 * These tags are also internally represented in string format.  For
 * example, tag PL_TRACE_BASE_ISD_LOGIN, is represented as "ISD_LOGIN".
 * This allows converting between a user-friendly string
 * representation and a compact integer representation.
 */
typedef enum {
    PL_TRACE_BASE_ALL,    // Trace all of OSD
    PL_TRACE_BASE_PL,
    PL_TRACE_BASE_HNS,

    // OC tags
    PL_TRACE_BASE_OC_CRT,     // Co-routines
    PL_TRACE_BASE_OC_UTL,     // Utilities
    PL_TRACE_BASE_OC_IO,      // The IO client
    PL_TRACE_BASE_OC_BPT,     // The b+-tree
    PL_TRACE_BASE_OC_PM,      // Page-Manager
    PL_TRACE_BASE_OC_ALL,     // Trace all OC

    PL_TRACE_BASE_LAST_TAG // used for internal purposes, do -not- use externally
} Pl_trace_base_tag;

// Initialize tracing
void pl_trace_base_init(void);

/* Add a [tag, level] pair to the set of traced tags.
 * This is a low-level API, it is easier to use the
 *     [pl_trace_base_add_string_tag_full]
 * function. The nicer API is supported only for user-mode. 
 */
void pl_trace_base_add_tag_lvl(Pl_trace_base_tag tag, int level);

// Set the debug level. The default is 1. This can be done once at the most. 
void pl_trace_base_set_level(int level);

/* The stage of initialization is complete. From now on no more -add_tag-
 * calls will be performed. 
 */
void pl_trace_base_init_done(void);

/* Are the [tag] and [level] traced?
 *
 * return 1 if true, 0 if false.
 */
int pl_trace_base_is_set(Pl_trace_base_tag tag, int level);

// Actual tracing functions
void pl_trace_base(Pl_trace_base_tag id,
                   int level,
                   const char *format_p,
               ...);

/* Add a tag, or a tag:level pair to the traces.
 *
 * There are two supported formats:
 *  <tag>  <tag>:<level>
 *
 * The first format is assumed to be for level=1.
 */
void pl_trace_base_add_string_tag_full(char *str_p);

/* Print the list of tags.
 * Used to describe to the user what tags are supported.
  */
void pl_trace_base_print_tag_list(void);


#endif
