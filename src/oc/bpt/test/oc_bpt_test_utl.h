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
/**********************************************************************/
/* OC_BPT_TEST_UTL.H
 *
 * Utilities for testing
 */
/**********************************************************************/
#ifndef OC_BPT_TEST_UTL_H
#define OC_BPT_TEST_UTL_H

#include "pl_base.h"
#include "oc_rm_s.h"

// configuration defined on the command line
extern int num_rounds;
extern int max_int;
extern int num_tasks;
extern int num_ops_per_task;
extern int max_num_clones;
extern bool verbose;
extern bool statistics;
extern int max_root_fanout;
extern int max_non_root_fanout;
extern int min_fanout;
extern int total_ops;

typedef enum Oc_bpt_test_utl_type {
    OC_BPT_TEST_UTL_ANY,
    OC_BPT_TEST_UTL_SMALL_TREES,
    OC_BPT_TEST_UTL_SMALL_TREES_W_RANGES,
    OC_BPT_TEST_UTL_SMALL_TREES_MIXED,
    OC_BPT_TEST_UTL_LARGE_TREES,
} Oc_bpt_test_utl_type;

extern Oc_bpt_test_utl_type test_type;

struct Oc_wu;

/* A structure that includes a b-tree and its linked-list
 * debugging equivalent. 
 */
struct Oc_bpt_test_state;

// setup
void oc_bpt_test_utl_init(void);
bool oc_bpt_test_utl_parse_cmd_line(int argc, char *argv[]);
void oc_bpt_test_utl_help_msg (void);

void oc_bpt_test_utl_set_print_fun(void (*print_fun)(void));
void oc_bpt_test_utl_set_validate_fun(bool (*validate_fun)(void));

// create a b-tree state
struct Oc_bpt_test_state* oc_bpt_test_utl_btree_init(
    struct Oc_wu *wu_p,
    uint64 tid);    

// release the b-tree state
void oc_bpt_test_utl_btree_destroy(
    struct Oc_bpt_test_state *s_p);
    
// create a b-tree.
// The [name_p] argument is used to identify this clone 
void oc_bpt_test_utl_btree_create(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* s_p);

/* insert [key] into the tree.
 * if [*check_eq_pio] is true then verify against the linked-list.
 * if the check succeeds, set [*check_eq_pio] to TRUE. Otherwise, set
 * it to FALSE;
 */
void oc_bpt_test_utl_btree_insert(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* s_p,
    uint32 key,
    bool *check_eq_pio);    

void oc_bpt_test_utl_btree_lookup(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* s_p,
    uint32 key,
    bool *check_eq_pio);    

void oc_bpt_test_utl_btree_remove_key(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* s_p,
    uint32 key,
    bool *check_eq_pio);    

void oc_bpt_test_utl_btree_delete(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* s_p);

bool oc_bpt_test_utl_btree_compare_and_verify(
    struct Oc_bpt_test_state* s_p);

// debugging
typedef enum Oc_bpt_test_utl_disp_choice {
    OC_BPT_TEST_UTL_TREE_ONLY,
    OC_BPT_TEST_UTL_LL_ONLY,
    OC_BPT_TEST_UTL_BOTH,    
} Oc_bpt_test_utl_disp_choice;

void oc_bpt_test_utl_btree_display(
    struct Oc_bpt_test_state* s_p,
    Oc_bpt_test_utl_disp_choice choice);

bool oc_bpt_test_utl_btree_validate(
    struct Oc_bpt_test_state* s_p);

// validate an array of clones
bool oc_bpt_test_utl_btree_validate_clones(
    int n_clones,
    struct Oc_bpt_test_state* s_p[]);

void oc_bpt_test_utl_statistics(
    struct Oc_bpt_test_state* s_p);

// range operations
void oc_bpt_test_utl_btree_lookup_range(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* s_p,
    uint32 lo_key, uint32 hi_key,
    bool *check_eq_pio);    

void oc_bpt_test_utl_btree_insert_range(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* s_p,
    uint32 lo_key, uint32 len,
    bool *check_eq_pio);    

void oc_bpt_test_utl_btree_remove_range(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state *s_p,    
    uint32 lo_key, uint32 hi_key,
    bool *check_eq_pio);    
    
// clone a tree
void oc_bpt_test_utl_btree_clone(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* src_p,
    struct Oc_bpt_test_state* trg_p);

// Display the set of clones together in one tree
void oc_bpt_test_utl_display_all(
    int n_clones,
    struct Oc_bpt_test_state* clone_array[]);

void oc_bpt_test_utl_finalize(int refcnt);

void oc_bpt_test_utl_setup_wu(Oc_wu *wu_p, struct Oc_rm_ticket *rm_ticket_p);

// generate a random unsigned 32-bit integer
uint32 oc_bpt_test_utl_random_number(uint32 top);

// Verify the free-space has [num_blocks] allocated
void oc_bpt_test_utl_fs_verify(int num_blocks);

struct Oc_bpt_state *oc_bpt_test_utl_get_state(struct Oc_bpt_test_state *s_p);
#endif
