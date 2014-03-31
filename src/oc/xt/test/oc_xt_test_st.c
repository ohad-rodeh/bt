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
#include <malloc.h>

#include "oc_xt_int.h"
#include "oc_xt_test_utl.h"

/******************************************************************/
static void test_init_fun(void);

static void large_trees (void);
static void small_trees (void);
static void small_trees_w_ranges (void);
static void small_trees_mixed (void);

/******************************************************************/
static void small_trees (void)
{
    int k, i;
    struct Oc_wu wu;
    Oc_rm_ticket rm;
    
    oc_xt_test_utl_setup_wu(&wu, &rm);

    // small trees with insert-range and lookup-range
    for (k=0; k<10; k++) {
        oc_xt_test_utl_init(&wu);        
        oc_xt_test_utl_create(&wu);
        
        for (i=0; i<3; i++) {
            int key = oc_xt_test_utl_random_number(100);
            int len = oc_xt_test_utl_random_number(10) + 1;
            
            oc_xt_test_utl_insert(&wu, key, len, TRUE);
            oc_xt_test_utl_lookup_range(&wu, key, key+len, TRUE);
            oc_xt_test_utl_finalize(1);
        }
        
        for (i=0; i<3; i++) {
            oc_xt_test_utl_insert(
                &wu,
                oc_xt_test_utl_random_number(200),
                oc_xt_test_utl_random_number(30) + 1,
                TRUE);
            oc_xt_test_utl_finalize(1);
        }
        
        if (statistics) oc_xt_test_utl_statistics();
        
        for (i=0; i<10; i++) {
            oc_xt_test_utl_lookup_range(
                &wu,
                oc_xt_test_utl_random_number(200),
                oc_xt_test_utl_random_number(30), TRUE);
            oc_xt_test_utl_finalize(1);
        }
        
        oc_xt_test_utl_delete(&wu);
        oc_xt_test_utl_finalize(0);            
    }
    
    printf ("done with small-trees\n"); fflush(stdout);
}

// small trees with -all- range-operations
static void small_trees_w_ranges (void)
{
    int k, i;
    struct Oc_wu wu;
    Oc_rm_ticket rm;
    
    oc_xt_test_utl_setup_wu(&wu, &rm);

    for (k=0; k<10; k++) {
        oc_xt_test_utl_init(&wu);        
        oc_xt_test_utl_create(&wu);
        
        for (i=0; i<3; i++) {
            int key = oc_xt_test_utl_random_number(100);
            int len = oc_xt_test_utl_random_number(10) + 1;
            
            oc_xt_test_utl_insert(&wu, key, len, TRUE);
            oc_xt_test_utl_lookup_range(&wu, key, key+len, TRUE);
            oc_xt_test_utl_finalize(1);
        }
        
        oc_xt_test_utl_compare_and_verify(&wu, max_int);            
        oc_xt_test_utl_finalize(1);                

        for (i=0; i<10; i++) {
            oc_xt_test_utl_lookup_range(
                &wu,
                oc_xt_test_utl_random_number(200),
                oc_xt_test_utl_random_number(30), TRUE);
            oc_xt_test_utl_finalize(1);
        }

        if (statistics) oc_xt_test_utl_statistics();       
        
        for (i=0; i<6; i++) {
            int key = oc_xt_test_utl_random_number(100);
            int len = oc_xt_test_utl_random_number(10);
            
            oc_xt_test_utl_remove_range(&wu, key, key+len, TRUE);
            oc_xt_test_utl_finalize(1);                

            oc_xt_test_utl_compare_and_verify(&wu, max_int);            
            oc_xt_test_utl_finalize(1);                
        }

        if (statistics) oc_xt_test_utl_statistics();
        
        oc_xt_test_utl_delete(&wu);
        oc_xt_test_utl_finalize(0);            
    }    
}

static void small_trees_mixed (void)
{
    int i, k, start;
    struct Oc_wu wu;
    Oc_rm_ticket rm;
    
    oc_xt_test_utl_setup_wu(&wu, &rm);

    for (k=0; k<3; k++) {
        oc_xt_test_utl_init(&wu);        
        oc_xt_test_utl_create(&wu);
        
        for (i=0; i<1000; i++)
        {
            switch (oc_xt_test_utl_random_number(5)) {
            case 0:
                oc_xt_test_utl_insert(
                    &wu,
                    oc_xt_test_utl_random_number(max_int),
                    1 + oc_xt_test_utl_random_number(10),
                    TRUE);
                break;
            case 1:
                start = oc_xt_test_utl_random_number(max_int);
                oc_xt_test_utl_lookup_range(
                    &wu,
                    start, 
                    start + 1 + oc_xt_test_utl_random_number(10),
                    TRUE);
                break;
            case 2:
                start = oc_xt_test_utl_random_number(max_int);
                oc_xt_test_utl_remove_range(
                    &wu,
                    start, 
                    start + 1 + oc_xt_test_utl_random_number(10),
                    TRUE);
                break;
            case 3:
                // removing a lot of entries so as to reduce tree size
                start = oc_xt_test_utl_random_number(max_int);
                
                oc_xt_test_utl_remove_range(
                    &wu,
                    start, 
                    start + 1 + oc_xt_test_utl_random_number(max_int/3),
                    TRUE);
                break;
            case 4:
                // long lookup-range
                start = oc_xt_test_utl_random_number(max_int),
                oc_xt_test_utl_lookup_range(
                    &wu,
                    start,
                    start + 1 + oc_xt_test_utl_random_number(max_int/3),
                    TRUE);
                break;

            default:
                // do nothing
                break;
            }
            
            oc_xt_test_utl_finalize(1);            
        }

        if (statistics) oc_xt_test_utl_statistics();         
        oc_xt_test_utl_delete(&wu);
        oc_xt_test_utl_finalize(0);
    }
}

static void large_trees (void)
{
    int i, k, start;
    struct Oc_wu wu;
    bool small_tree;
    Oc_rm_ticket rm;
    
    oc_xt_test_utl_setup_wu(&wu, &rm);
    oc_xt_test_utl_init(&wu);        
    printf ("// running large_trees test\n");
    
    for (k=0; k<20; k++) {
        small_tree = oc_xt_test_utl_random_number(2);
        oc_xt_test_utl_create(&wu);
        
        for (i=0; i<num_rounds; i++)
        {
            switch (oc_xt_test_utl_random_number(5)) {
            case 0:
                oc_xt_test_utl_insert(
                    &wu,
                    oc_xt_test_utl_random_number(max_int),
                    1 + oc_xt_test_utl_random_number(10),
                    TRUE);
                break;
            case 1:
                start = oc_xt_test_utl_random_number(max_int);
                oc_xt_test_utl_lookup_range(
                    &wu,
                    start, 
                    start + 1 + oc_xt_test_utl_random_number(10),
                    TRUE);
                break;
            case 2:
                start = oc_xt_test_utl_random_number(max_int);
                
                oc_xt_test_utl_remove_range(
                    &wu,
                    start, 
                    start + 1 + oc_xt_test_utl_random_number(10),
                    TRUE);
                break;
            case 3:
                if (small_tree) {
                    // remove many entries so as to reduce tree size
                    start = oc_xt_test_utl_random_number(max_int);
                    
                    oc_xt_test_utl_remove_range(
                        &wu,
                        start, 
                        start + 1 + oc_xt_test_utl_random_number(max_int/3),
                        FALSE);
                }
                break;
            case 4:
                // long lookup-range
                start = oc_xt_test_utl_random_number(max_int);
                oc_xt_test_utl_lookup_range(
                    &wu,
                    start,
                    start + 1 + oc_xt_test_utl_random_number(max_int/3),
                    TRUE);
                break;
            }

            oc_xt_test_utl_finalize(1);            
        }
        
        // final sanity check
        oc_xt_test_utl_compare_and_verify(&wu, max_int);

        if (statistics) oc_xt_test_utl_statistics();        
        oc_xt_test_utl_delete(&wu);
        oc_xt_test_utl_finalize(0);
    }

    printf ("done large_trees test\n"); fflush(stdout);
}

/******************************************************************/

static void test_init_fun(void)
{
    oc_xt_test_utl_init_module();
    
    // Open a task to run all the tests
    oc_xt_dbg_output_init((struct Oc_utl_file*)stdout);
            
    // random test
    switch (test_type) {
    case OC_XT_TEST_UTL_ANY:
        large_trees();
        small_trees();
        small_trees_w_ranges();
        small_trees_mixed();
        break;
    case OC_XT_TEST_UTL_LARGE_TREES:
        large_trees();
        break;
    case OC_XT_TEST_UTL_SMALL_TREES:
        small_trees();
        break;
    case OC_XT_TEST_UTL_SMALL_TREES_W_RANGES:
        small_trees_w_ranges();
        break;
    case OC_XT_TEST_UTL_SMALL_TREES_MIXED:
        small_trees_mixed();
        break;
    }

    printf("   // total_ops=%d\n", total_ops);
    // oc_xt_dbg_output_end(stdout);
}

int main(int argc, char *argv[])
{
    // initialize the tracing facility 
    pl_trace_base_init();

    if (oc_xt_test_utl_parse_cmd_line(argc, argv) == FALSE)
        oc_xt_test_utl_help_msg();

    // done initializing tracing
    pl_trace_base_init_done();

    // The following line creates a new thread and must be left last in function
    // inorder to ensure deterministic behaviour.
    test_init_fun();

    return 0;
}    

/******************************************************************/

