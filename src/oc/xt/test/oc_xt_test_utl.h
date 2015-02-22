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
/* OC_XT_TEST_UTL.H
 *
 * Utilities for testing
 */
/**********************************************************************/
#ifndef OC_XT_TEST_UTL_H
#define OC_XT_TEST_UTL_H

#include "pl_base.h"
#include "oc_rm_s.h"
#include "oc_wu_s.h"

// configuration defined on the command line
extern int num_rounds;
extern int max_int;
extern int num_tasks;
extern bool verbose;
extern bool statistics;
extern int max_root_fanout;
extern int max_non_root_fanout;
extern int min_fanout;
extern int total_ops;

typedef enum Oc_xt_test_utl_type {
    OC_XT_TEST_UTL_ANY,
    OC_XT_TEST_UTL_SMALL_TREES,
    OC_XT_TEST_UTL_SMALL_TREES_W_RANGES,
    OC_XT_TEST_UTL_SMALL_TREES_MIXED,
    OC_XT_TEST_UTL_LARGE_TREES,
} Oc_xt_test_utl_type;

extern Oc_xt_test_utl_type test_type;

// setup
void oc_xt_test_utl_init_module(void);
bool oc_xt_test_utl_parse_cmd_line(int argc, char *argv[]);
void oc_xt_test_utl_help_msg (void);

void oc_xt_test_utl_alt_print(void);

void oc_xt_test_utl_init(struct Oc_wu *wu_p);
void oc_xt_test_utl_create(struct Oc_wu *wu_p);
void oc_xt_test_utl_delete(struct Oc_wu *wu_p);
void oc_xt_test_utl_compare_and_verify(Oc_wu *wu_p, int max_int);

// debugging
void oc_xt_test_utl_display(bool with_linked_list);
bool oc_xt_test_utl_validate(void);
void oc_xt_test_utl_statistics(void);

// modification operations
void oc_xt_test_utl_lookup_range(Oc_wu *wu_p, uint32 lo_key, uint32 hi_key,
                                        bool check);

// Insert an extent into the tree
void oc_xt_test_utl_insert(Oc_wu *wu_p, uint32 lo_key, uint32 len,
                           bool check);

void oc_xt_test_utl_remove_range(Oc_wu *wu_p, uint32 lo_key, uint32 hi_key,
                                        bool check);

void oc_xt_test_utl_finalize(int refcnt);

void oc_xt_test_utl_setup_wu(Oc_wu *wu_p, struct Oc_rm_ticket *rm_ticket_p);
uint32 oc_xt_test_utl_random_number(uint32 top);

#endif
