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

#include "pl_int.h"
#include "oc_xt_int.h"
#include "oc_xt_test_utl.h"
#include "oc_xt_alt.h"

/******************************************************************/
static void run_tests(void* dummy);
static void test_init_fun(void);

static void multi_threaded_test (void);
/******************************************************************/

static Oc_crt_sema sema;

static void insert_thread(void *dummy)
{
    int i;
    Oc_wu wu;
    Oc_rm_ticket rm_ticket;
    
    oc_xt_test_utl_setup_wu(&wu, &rm_ticket);    
    if (verbose) printf("   // running insert thread\n");

    for (i=0; i<num_rounds/num_tasks + 10; i++)
        oc_xt_test_utl_insert(
            &wu,
            oc_xt_test_utl_random_number(max_int),
            oc_xt_test_utl_random_number(10) + 1,            
            FALSE);
    
    oc_crt_sema_post(&sema);    
}

static void remove_thread(void *dummy)
{
    int i;
    Oc_wu wu;
    Oc_rm_ticket rm;
    
    oc_xt_test_utl_setup_wu(&wu, &rm);
    if (verbose) printf("    // running remove thread\n");

    for (i=0; i<num_rounds/num_tasks + 10; i++) {
        uint32 start = oc_xt_test_utl_random_number(max_int);
        oc_xt_test_utl_remove_range(
            &wu,
            start, 
            start + oc_xt_test_utl_random_number(10),
            FALSE);
    }
    
    oc_crt_sema_post(&sema);    
}

static void random_thread(void *_small_tree)
{
    bool small_tree = (bool) small_tree;
    int i, start;
    Oc_wu wu;
    Oc_rm_ticket rm;
    
    oc_xt_test_utl_setup_wu(&wu, &rm);
    if (verbose) printf("    // running random thread\n");
    
    for (i=0; i<num_rounds; i++) {
        switch (oc_xt_test_utl_random_number(6)) {
        case 0:
            oc_xt_test_utl_insert(
                &wu,
                oc_xt_test_utl_random_number(max_int),
                oc_xt_test_utl_random_number(10) + 1,
                FALSE);
            break;
        case 1:
            start = oc_xt_test_utl_random_number(max_int);
            oc_xt_test_utl_lookup_range(
                &wu,
                start, 
                start + oc_xt_test_utl_random_number(10),
                FALSE);
            break;
        case 2:
            start = oc_xt_test_utl_random_number(max_int);
            oc_xt_test_utl_remove_range(
                &wu,
                start, 
                start + oc_xt_test_utl_random_number(10),
                FALSE);
            break;
            
        case 3:
            if (small_tree) {
                // remove many entries so as to reduce tree size
                start = oc_xt_test_utl_random_number(max_int);
                
                oc_xt_test_utl_remove_range(
                    &wu,
                    start, 
                    start + oc_xt_test_utl_random_number(max_int/3),
                    FALSE);
            }
            break;
        case 4:
            // long lookup-range
            start = oc_xt_test_utl_random_number(max_int);
            oc_xt_test_utl_lookup_range(
                &wu,
                start,
                start + oc_xt_test_utl_random_number(max_int/3),
                FALSE);
            break;
        case 5:
            // occasionally, lock the tree and validate it
            if (oc_xt_test_utl_random_number(100) == 0)
                oc_xt_test_utl_validate();
            break;
        }
    }
    
    oc_crt_sema_post(&sema);    
}

static void multi_threaded_test (void)
{
    int i;
    Oc_wu wu;
    bool small_tree;
    Oc_rm_ticket rm;
    
    oc_xt_test_utl_setup_wu(&wu, &rm);
    oc_crt_sema_init(&sema, 0);
    oc_xt_test_utl_init(&wu);        
    oc_xt_test_utl_create(&wu);

    // run insert-threads
    for (i=0; i<num_tasks; i++)
        oc_crt_create_task("insert_thread", insert_thread, NULL);

    // wait for threads to complete
    for (i=0; i<num_tasks; i++)
        oc_crt_sema_wait(&sema);

    // check results
    oc_xt_test_utl_finalize(1);                
    for (i=0; i<max_int; i++) {
        uint32 start = oc_xt_test_utl_random_number(max_int);
        oc_xt_test_utl_lookup_range(
            &wu,
            start,
            start + oc_xt_test_utl_random_number(max_int/3),
            FALSE);
    }

    // run remove-threads
    for (i=0; i<num_tasks; i++)
        oc_crt_create_task("insert_thread", remove_thread, NULL);

    // wait for threads to complete
    for (i=0; i<num_tasks; i++)
        oc_crt_sema_wait(&sema);
    
    // check results
    oc_xt_test_utl_finalize(1);                
    for (i=0; i<max_int; i++) {
        uint32 start = oc_xt_test_utl_random_number(max_int);
        oc_xt_test_utl_lookup_range(
            &wu,
            start,
            start + oc_xt_test_utl_random_number(max_int/3),
            FALSE);
    }
    
    for (i=0; i<10; i++) 
        oc_xt_test_utl_lookup_range(&wu,
                                    oc_xt_test_utl_random_number(max_int),
                                    oc_xt_test_utl_random_number(max_int),
                                    FALSE);
    
    // run random-threads
    if (oc_xt_test_utl_random_number(3) == 0)
        small_tree = TRUE;
    else
        small_tree = FALSE;
    for (i=0; i<num_tasks; i++)
        oc_crt_create_task("random_thread", random_thread, (void*)small_tree);

    // wait for threads to complete
    for (i=0; i<num_tasks; i++)
        oc_crt_sema_wait(&sema);

    oc_xt_test_utl_finalize(1);
    oc_xt_test_utl_validate();
    oc_xt_test_utl_delete(&wu);
}

/******************************************************************/

static void run_tests(void* dummy)
{
    int j;
    
    oc_xt_dbg_output_init((struct Oc_utl_file*)stdout);
    
    for (j=0; j<3; j++)
        multi_threaded_test ();

    printf("   // total_ops=%d\n", total_ops);
    oc_xt_dbg_output_end((struct Oc_utl_file*)stdout);

    exit(0);
}

/******************************************************************/

static void test_init_fun(void)
{
    oc_xt_test_utl_init_module();
    run_tests(NULL);    
}

int main(int argc, char *argv[])
{
    sem_t sema ;
    Oc_crt_config crt_conf;
    
    // initialize the tracing facility 
    osd_trace_init();

    if (oc_xt_test_utl_parse_cmd_line(argc, argv) == FALSE)
        oc_xt_test_utl_help_msg();

    // done initializing tracing
    osd_trace_init_done();

    pl_init();
    pl_utl_set_hns();
    
    /* The following line creates a new thread and must be left last
     * in function inorder to ensure deterministic behaviour.
     */
    oc_crt_default_config(&crt_conf);
    crt_conf.init_fun = test_init_fun;
    oc_crt_init_full(&crt_conf);

    // go to sleep forever
    sem_init(&sema,0,0);
    sem_wait(&sema);
    return 0;
}    

/******************************************************************/

