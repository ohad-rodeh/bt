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
#include <semaphore.h>
#include <unistd.h>

#include "pl_int.h"
#include "oc_crt_int.h"
#include "oc_bpt_int.h"
#include "oc_bpt_test_utl.h"
#include "oc_bpt_alt.h"
#include "pl_trace_base.h"
/******************************************************************/
static void print_and_exit(struct Oc_bpt_test_state *s_p);
static void run_tests(void* dummy);
static void test_init_fun(void);

static void multi_threaded_test (void);
/******************************************************************/

static Oc_bpt_test_param *param = NULL;
static struct Oc_bpt_test_state *s_p;
static Oc_crt_sema sema;

// Print tree [s_p] and exit
static void print_and_exit(struct Oc_bpt_test_state *s_p)
{
    oc_bpt_test_utl_btree_display(s_p, OC_BPT_TEST_UTL_TREE_ONLY);
    oc_bpt_dbg_output_end();
    exit(1);
}

static void display_tree(void)
{
    oc_bpt_test_utl_btree_display(s_p, OC_BPT_TEST_UTL_TREE_ONLY);    
}

static bool validate_tree(void)
{
    return oc_bpt_test_utl_btree_validate(s_p);
}

static void* insert_thread(void *dummy)
{
    int i;
    Oc_wu wu;
    Oc_rm_ticket rm_ticket;
    bool rc = FALSE;

    oc_bpt_test_utl_setup_wu(&wu, &rm_ticket);    
    if (param->verbose) printf("   // running insert thread\n");

    for (i=0; i<param->num_rounds/param->num_tasks + 10; i++)
        oc_bpt_test_utl_btree_insert(
            &wu,
            s_p,
            oc_bpt_test_utl_random_number(param->max_int),
            &rc);
    
    oc_crt_sema_post(&sema);    
    return NULL;
}

static void* remove_thread(void *dummy)
{
    int i;
    Oc_wu wu;
    Oc_rm_ticket rm;
    bool rc = FALSE;

    oc_bpt_test_utl_setup_wu(&wu, &rm);
    if (param->verbose) printf("    // running remove thread\n");

    for (i=0; i<param->num_rounds/param->num_tasks + 10; i++)
        oc_bpt_test_utl_btree_remove_key(
            &wu,
            s_p,            
            oc_bpt_test_utl_random_number(param->max_int),
            &rc);
    
    oc_crt_sema_post(&sema);    
    return NULL;
}

static void* random_thread(void *_small_tree)
{
    bool small_tree = (bool) small_tree;
    int i, start;
    Oc_wu wu;
    Oc_rm_ticket rm;
    
    oc_bpt_test_utl_setup_wu(&wu, &rm);
    if (param->verbose) printf("    // running random thread\n");
    
    for (i=0; i<param->num_rounds; i++)
    {
        bool rc = FALSE;
        
        switch (oc_bpt_test_utl_random_number(8)) {
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
            start = oc_bpt_test_utl_random_number(param->max_int),
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
            if (small_tree) {
                // remove many entries so as to reduce tree size
                start = oc_bpt_test_utl_random_number(param->max_int);
                
                oc_bpt_test_utl_btree_remove_range(
                    &wu,
                    s_p,                            
                    start, 
                    start + oc_bpt_test_utl_random_number(param->max_int/3),
                    &rc);
            }
            break;
        case 7:
            // long lookup-range
            start = oc_bpt_test_utl_random_number(param->max_int);
            oc_bpt_test_utl_btree_lookup_range(
                &wu,
                s_p,                            
                start,
                start + oc_bpt_test_utl_random_number(param->max_int/3),
                &rc);
            break;
        case 8:
            // occasionally, lock the tree and validate it
            if (oc_bpt_test_utl_random_number(100) == 0)
                oc_bpt_test_utl_btree_validate(s_p);
            break;
        }
    }
    
    oc_crt_sema_post(&sema);    
    return NULL;
}

static void multi_threaded_test (void)
{
    int i;
    Oc_wu wu;
    bool small_tree;
    Oc_rm_ticket rm;
    
    oc_bpt_test_utl_setup_wu(&wu, &rm);
    oc_crt_sema_init(&sema, 0);
    s_p = oc_bpt_test_utl_btree_init(&wu, 0);        
    oc_bpt_test_utl_btree_create(&wu, s_p);

    // run insert-threads
    for (i=0; i<param->num_tasks; i++) {
        fflush(stdout);
        oc_crt_create_task("insert_thread", insert_thread, NULL);
    }

    // wait for threads to complete
    for (i=0; i<param->num_tasks; i++)
        oc_crt_sema_wait(&sema);

    // check results
    oc_bpt_test_utl_finalize(1);                
    for (i=0; i<param->max_int; i++) {
        bool rc = TRUE;

        oc_bpt_test_utl_btree_lookup(&wu, s_p, i, &rc);
        if (!rc)
            print_and_exit(s_p);
    }

    // run remove-threads
    for (i=0; i<param->num_tasks; i++)
        oc_crt_create_task("insert_thread", remove_thread, NULL);

    // wait for threads to complete
    for (i=0; i<param->num_tasks; i++)
        oc_crt_sema_wait(&sema);
    
    // check results
    oc_bpt_test_utl_finalize(1);                
    for (i=0; i<param->max_int; i++) {
        bool rc = TRUE;
        
        oc_bpt_test_utl_btree_lookup(&wu, s_p, i, &rc);
        if (!rc)
            print_and_exit(s_p);
    }
    

    for (i=0; i<10; i++) {
        bool rc = FALSE;
        
        oc_bpt_test_utl_btree_lookup_range(
            &wu,
            s_p,
            oc_bpt_test_utl_random_number(param->max_int),
            oc_bpt_test_utl_random_number(param->max_int),
            &rc);
    }
    
    // run random-threads
    if (oc_bpt_test_utl_random_number(3) == 0)
        small_tree = TRUE;
    else
        small_tree = FALSE;
    for (i=0; i<param->num_tasks; i++)
        oc_crt_create_task("random_thread", random_thread, (void*)small_tree);

    // wait for threads to complete
    for (i=0; i<param->num_tasks; i++)
        oc_crt_sema_wait(&sema);

    oc_bpt_test_utl_finalize(1);
    oc_bpt_test_utl_btree_validate(s_p);
    oc_bpt_test_utl_btree_delete(&wu, s_p);
}

/******************************************************************/

static void run_tests(void* dummy)
{
    int j;

    
    oc_bpt_dbg_output_init();
    
    for (j=0; j<3; j++)
        multi_threaded_test ();

    // verify the free-space has no block allocated
    oc_bpt_test_utl_fs_verify(0);
    
    printf("   // total_ops=%d\n", param->total_ops);
    oc_bpt_dbg_output_end();

    exit(0);
}

/******************************************************************/

static void test_init_fun(void)
{
    oc_bpt_test_utl_init();
    oc_bpt_test_utl_set_print_fun(display_tree);
    oc_bpt_test_utl_set_validate_fun(validate_tree);
    run_tests(NULL);
}

int main(int argc, char *argv[])
{
    Oc_crt_config crt_conf;
    
    // initialize the tracing facility 
    pl_trace_base_init();

    if (oc_bpt_test_utl_parse_cmd_line(argc, argv) == FALSE)
        oc_bpt_test_utl_help_msg();
    param = oc_bpt_test_utl_get_param();

    // done initializing tracing
    pl_trace_base_init_done();

    pl_init();

    /* The following line creates a new thread and must be left last
     * in function inorder to ensure deterministic behaviour.
     */
    oc_crt_default_config(&crt_conf);
    crt_conf.init_fun = test_init_fun;

    // why do we need this? 
    param->num_tasks = 10;

    //  We need more pages for the lookup-range function
    crt_conf.stack_page_size = 20;  
    oc_crt_init_full(&crt_conf);

    // go to sleep forever
    //sem_init(&sema,0,0);
    //sem_wait(&sema);
    sleep(10000);
    return 0;
}    

/******************************************************************/

