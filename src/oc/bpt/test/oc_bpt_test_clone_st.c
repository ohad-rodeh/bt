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
/* OC_BPT_TEST_CLONE_ST.C
 * Test program for clones and b-trees.
 */
/******************************************************************/
/* Test program for the b-tree that:
 *  - Tests the clone operation
 *  - Is single-threaded, i.e, does not use the CRT module
 *
 * the keys are uint32, the data is all zeros
 */
/******************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pl_trace_base.h"
#include "oc_utl_trk.h"
#include "oc_bpt_int.h"
#include "oc_bpt_test_clone.h"
#include "oc_bpt_test_utl.h"

/******************************************************************/
static void test_init_fun(void);
static void small_trees (void);
static void print_and_exit(void);

static Oc_bpt_test_param *param = NULL;

/******************************************************************/

// Print the entire clone-set exit
static void print_and_exit(void)
{
    oc_bpt_test_clone_display_all();
    oc_bpt_dbg_output_end();
    exit(1);
}

static uint64 get_tid(void)
{
    static uint64 tid = 0;

    return tid++;
}

/******************************************************************/

static void small_trees (void)
{
    int i, k, start;
    struct Oc_bpt_test_state *s_p;
    struct Oc_wu wu;
    Oc_rm_ticket rm;
    
    for (k=0; k<3; k++) {
        oc_bpt_test_utl_setup_wu(&wu, &rm);
        s_p = oc_bpt_test_utl_btree_init(&wu, get_tid());       
        oc_bpt_test_utl_btree_create(&wu, s_p);
        oc_bpt_test_clone_add(s_p);
        wu.po_id++;
        
        for (i=0; i<param->num_rounds; i++)
        {
            bool rc = TRUE;
            int choice;
            
            oc_bpt_test_utl_setup_wu(&wu, &rm);
            
            // choose a clone to perform an operation on
            s_p = oc_bpt_test_clone_choose();

            choice = oc_bpt_test_utl_random_number(11);
            switch (choice) {
            case 0:
                oc_bpt_test_utl_btree_remove_key(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(param->max_int),
                    &rc);
                break;
            case 1:
                oc_bpt_test_utl_btree_insert(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(param->max_int),
                    &rc);            
                break;
            case 2:
                oc_bpt_test_utl_btree_lookup(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(param->max_int),
                    &rc);          
                break;

            case 3:
                oc_bpt_test_utl_btree_insert_range(
                    &wu,
                    s_p, 
                    oc_bpt_test_utl_random_number(param->max_int),
                    oc_bpt_test_utl_random_number(10),
                    &rc);          
                break;
            case 4:
                start = oc_bpt_test_utl_random_number(param->max_int);
                oc_bpt_test_utl_btree_lookup_range(
                    &wu,
                    s_p, 
                    start, 
                    start + oc_bpt_test_utl_random_number(10),
                    &rc); 
                break;
            case 5:
                start = oc_bpt_test_utl_random_number(param->max_int);
                oc_bpt_test_utl_btree_remove_range(
                    &wu,
                    s_p, 
                    start, 
                    start + oc_bpt_test_utl_random_number(10),
                    &rc); 
                break;

            case 6:
                // removing a lot of entries so as to reduce tree size
                start = oc_bpt_test_utl_random_number(param->max_int);
                
                oc_bpt_test_utl_btree_remove_range(
                    &wu,
                    s_p, 
                    start, 
                    start + oc_bpt_test_utl_random_number(param->max_int/3),
                    &rc);                   
                break;
            case 7:
                // long lookup-range
                start = oc_bpt_test_utl_random_number(param->max_int),
                oc_bpt_test_utl_btree_lookup_range(
                    &wu,
                    s_p, 
                    start,
                    start + oc_bpt_test_utl_random_number(param->max_int/3),
                    &rc);
                    break;
                
                // clone operations
            case 8:
                // clone random tree
                if (oc_bpt_test_clone_can_create_more()) {
                    struct Oc_bpt_test_state *clone_p;

                    clone_p = oc_bpt_test_utl_btree_init(&wu, get_tid());
                    oc_bpt_test_utl_btree_clone(&wu, s_p, clone_p);
                    oc_bpt_test_clone_add(clone_p);
                }
                break;
            case 9:
                // delete random clone
                if (oc_bpt_test_utl_random_number(5) == 0 ||
                    oc_bpt_test_clone_num_live() > 5)
                {
                    if (oc_bpt_test_clone_num_live() > 1)
                        oc_bpt_test_clone_delete(&wu, s_p);
                }
                break;
                
            case 10:
                // compare and verify the whole set
                if (!oc_bpt_test_clone_validate_all())
                    print_and_exit();
                break;
                
            default:
                // do nothing
                break;
            }

            // make sure all locks have been released
            if (choice != 8)
                oc_utl_trk_finalize(&wu);            
            if (!rc) {
                // The operation did not match against the linked-list
                print_and_exit();
            }
            oc_bpt_test_utl_finalize(oc_bpt_test_clone_num_live());            
        }

        if (param->statistics) oc_bpt_test_utl_statistics(s_p);         

        oc_bpt_test_clone_delete_all(&wu);
        oc_bpt_test_utl_finalize(0);
    }
}


/******************************************************************/

static void test_init_fun(void)
{
    oc_bpt_test_utl_init();
    oc_bpt_test_utl_set_print_fun(oc_bpt_test_clone_display_all);
    oc_bpt_test_utl_set_validate_fun(oc_bpt_test_clone_validate_all);
    oc_bpt_test_clone_init_ds();
    
    // Open a task to run all the tests
    oc_bpt_dbg_output_init();
            
    // There is only one test here
    small_trees();

    // verify the free-space has no block allocated
    oc_bpt_test_utl_fs_verify(0);
    
    printf("   // total_ops=%d\n", param->total_ops);
    oc_bpt_dbg_output_end();
}

int main(int argc, char *argv[])
{
    // initialize the tracing facility 
    pl_trace_base_init();

    if (oc_bpt_test_utl_parse_cmd_line(argc, argv) == FALSE)
        oc_bpt_test_utl_help_msg();
    param = oc_bpt_test_utl_get_param();

    // done initializing tracing
    pl_trace_base_init_done();
    
    // The following line creates a new thread and must be left last in function
    // inorder to ensure deterministic behaviour.
    test_init_fun();

    return 0;
}    

/******************************************************************/

