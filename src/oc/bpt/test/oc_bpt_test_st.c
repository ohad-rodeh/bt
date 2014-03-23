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
/******************************************************************/
/* Small test program for the b-tree
 *
 * the keys are uint32 the data is all zeros
 */
/******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "oc_bpt_int.h"
#include "oc_bpt_test_utl.h"

#include "pl_trace_base.h"

/******************************************************************/
static void test_init_fun(void);
static void print_and_exit(struct Oc_bpt_test_state *s_p);
    
static void large_trees (void);
static void small_trees (void);

/******************************************************************/
// Print tree [s_p] and exit
static void print_and_exit(struct Oc_bpt_test_state *s_p)
{
    oc_bpt_test_utl_btree_display(s_p, OC_BPT_TEST_UTL_TREE_ONLY);
    oc_bpt_dbg_output_end();
    exit(1);
}

static struct Oc_bpt_test_state *s_p = NULL;

static void display_tree(void)
{
    oc_bpt_test_utl_btree_display(s_p, OC_BPT_TEST_UTL_TREE_ONLY);    
}

static bool validate_tree(void)
{
    return oc_bpt_test_utl_btree_validate(s_p);
}

/******************************************************************/

static void small_trees (void)
{
    int i, k, start;
    struct Oc_wu wu;
    Oc_rm_ticket rm;
    
    oc_bpt_test_utl_setup_wu(&wu, &rm);

    for (k=0; k<3; k++) {
        s_p = oc_bpt_test_utl_btree_init(&wu, 0);        
        oc_bpt_test_utl_btree_create(&wu, s_p);
        
        for (i=0; i<1000; i++)
        {
            bool rc = TRUE;
            
            switch (oc_bpt_test_utl_random_number(8)) {
            case 0:
                oc_bpt_test_utl_btree_remove_key(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(max_int),
                    &rc);
                break;
            case 1:
                oc_bpt_test_utl_btree_insert(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(max_int),
                    &rc);
                break;
            case 2:
                oc_bpt_test_utl_btree_lookup(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(max_int),
                    &rc);
                break;
            case 3:
                oc_bpt_test_utl_btree_insert_range(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(max_int),
                    oc_bpt_test_utl_random_number(10),
                    &rc);
                break;
            case 4:
                start = oc_bpt_test_utl_random_number(max_int);
                oc_bpt_test_utl_btree_lookup_range(
                    &wu,
                    s_p, 
                    start, 
                    start + oc_bpt_test_utl_random_number(10),
                    &rc);
                break;
            case 5:
                start = oc_bpt_test_utl_random_number(max_int);
                oc_bpt_test_utl_btree_remove_range(
                    &wu,
                    s_p, 
                    start, 
                    start + oc_bpt_test_utl_random_number(10),
                    &rc);
                break;
                
            case 6:
                // removing a lot of entries so as to reduce tree size

                start = oc_bpt_test_utl_random_number(max_int);
                
                oc_bpt_test_utl_btree_remove_range(
                    &wu,
                    s_p, 
                    start, 
                    start + oc_bpt_test_utl_random_number(max_int/3),
                    &rc);
                break;
            case 7:
                // long lookup-range
                start = oc_bpt_test_utl_random_number(max_int),
                oc_bpt_test_utl_btree_lookup_range(
                    &wu,
                    s_p, 
                    start,
                    start + oc_bpt_test_utl_random_number(max_int/3),
                    &rc);
                break;

            default:
                // do nothing
                break;
            }
            
            if (!rc)
                print_and_exit(s_p);
            oc_bpt_test_utl_finalize(1);            
        }
        
        if (statistics) oc_bpt_test_utl_statistics(s_p);         
        oc_bpt_test_utl_btree_delete(&wu, s_p);
        oc_bpt_test_utl_finalize(0);
        oc_bpt_test_utl_btree_destroy(s_p);
        s_p = NULL;
    }
}

static void large_trees (void)
{
    int i, k, start;
    struct Oc_wu wu;
    bool small_tree;
    Oc_rm_ticket rm;
    
    oc_bpt_test_utl_setup_wu(&wu, &rm);
    s_p = oc_bpt_test_utl_btree_init(&wu, 0);        
    printf ("running large_trees test\n");
    
    for (k=0; k<20; k++) {
        small_tree = oc_bpt_test_utl_random_number(2);
        oc_bpt_test_utl_btree_create(&wu, s_p);
        
        for (i=0; i<num_rounds; i++)
        {
            bool rc = TRUE;

            switch (oc_bpt_test_utl_random_number(8)) {
            case 0:
                oc_bpt_test_utl_btree_remove_key(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(max_int),
                    &rc);
                break;
            case 1:
                oc_bpt_test_utl_btree_insert(
                    &wu,
                    s_p,
                    oc_bpt_test_utl_random_number(max_int),
                    &rc);            
                break;
            case 2:
                oc_bpt_test_utl_btree_lookup(
                    &wu,
                    s_p,
                    oc_bpt_test_utl_random_number(max_int),
                    &rc);
                break;
            case 3:
                oc_bpt_test_utl_btree_insert_range(
                    &wu,
                    s_p,
                    oc_bpt_test_utl_random_number(max_int),
                    oc_bpt_test_utl_random_number(10),
                    &rc);
                break;
            case 4:
                start = oc_bpt_test_utl_random_number(max_int);
                oc_bpt_test_utl_btree_lookup_range(
                    &wu,
                    s_p,
                    start, 
                    start + oc_bpt_test_utl_random_number(10),
                    &rc);
                break;
            case 5:
                start = oc_bpt_test_utl_random_number(max_int);
                
                oc_bpt_test_utl_btree_remove_range(
                    &wu,
                    s_p,
                    start, 
                    start + oc_bpt_test_utl_random_number(10),
                    &rc);
                break;
            case 6:
                if (small_tree) {
                    // remove many entries so as to reduce tree size
                    start = oc_bpt_test_utl_random_number(max_int);
                    
                    oc_bpt_test_utl_btree_remove_range(
                        &wu,
                        s_p,
                        start, 
                        start + oc_bpt_test_utl_random_number(max_int/3),
                        &rc);
                }
                break;
            case 7:
                // lonag lookup-range
                start = oc_bpt_test_utl_random_number(max_int);
                oc_bpt_test_utl_btree_lookup_range(
                    &wu,
                    s_p,
                    start,
                    start + oc_bpt_test_utl_random_number(max_int/3),
                    &rc);
                break;
            }

            if (!rc)
                print_and_exit(s_p);
            oc_bpt_test_utl_finalize(1);            
        }
        
        // final sanity check
        oc_bpt_test_utl_btree_compare_and_verify(s_p);

        if (statistics) oc_bpt_test_utl_statistics(s_p);   
        oc_bpt_test_utl_btree_delete(&wu, s_p);
        oc_bpt_test_utl_finalize(0);
    }

    oc_bpt_test_utl_btree_destroy(s_p);
    s_p = NULL;
}

/******************************************************************/

static void test_init_fun(void)
{
    oc_bpt_test_utl_init();
    
    // Open a task to run all the tests
    oc_bpt_dbg_output_init();

    oc_bpt_test_utl_set_print_fun(display_tree);
    oc_bpt_test_utl_set_validate_fun(validate_tree);
    
    // random test
    switch (test_type) {
    case OC_BPT_TEST_UTL_ANY:
        small_trees();
        large_trees();
        break;
    case OC_BPT_TEST_UTL_LARGE_TREES:
        large_trees();
        break;
    case OC_BPT_TEST_UTL_SMALL_TREES:
        small_trees();
        break;
    case OC_BPT_TEST_UTL_SMALL_TREES_W_RANGES:
        break;
    case OC_BPT_TEST_UTL_SMALL_TREES_MIXED:
        break;
    }

    // verify the free-space has no block allocated
    oc_bpt_test_utl_fs_verify(0);
    
    printf("   // total_ops=%d\n", total_ops);
    oc_bpt_dbg_output_end();
}

int main(int argc, char *argv[])
{
    // initialize the tracing facility 
    pl_trace_base_init();

    if (oc_bpt_test_utl_parse_cmd_line(argc, argv) == FALSE)
        oc_bpt_test_utl_help_msg();

    // done initializing tracing
    pl_trace_base_init_done();
    
    // The following line creates a new thread and must be left last in function
    // inorder to ensure deterministic behaviour.
    test_init_fun();

    return 0;
}    

/******************************************************************/

