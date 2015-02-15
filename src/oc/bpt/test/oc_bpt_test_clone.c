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
/* OC_BPT_TEST_CLONE.C
 * 
 * Data-structures used for testing b-tree clone operations. 
 */
/******************************************************************/
#include <string.h>

#include "pl_mm_int.h"
#include "oc_utl.h"
#include "oc_bpt_test_utl.h"
#include "oc_bpt_test_fs.h"
#include "oc_bpt_int.h"

/******************************************************************/

/* There is a data-structure that keeps track of all the live clones.
 */

// An array containing pointers to clones
static struct Oc_bpt_test_state **clone_array;
static int num_clones = 0;

static void oc_bpt_test_clone_validate_fs(void);
/******************************************************************/

static void iter_fun(Oc_wu *wu_p, Oc_bpt_node *node_p)
{
    oc_bpt_test_fs_alt_inc_refcount(wu_p, node_p->disk_addr);
}

// Checking that the free-space block-map is correct 
static void oc_bpt_test_clone_validate_fs(void)
{
    int i;
    struct Oc_bpt_test_state *s_p;
    struct Oc_wu wu;
    Oc_rm_ticket rm;
    bool rc;
    
    oc_bpt_test_utl_setup_wu(&wu, &rm);
    
    // create an alternate block map
    oc_bpt_test_fs_alt_create();
    
    for (i=0; i<num_clones; i++) {
        s_p = clone_array[i];
        oc_utl_assert(s_p != NULL);
        
        oc_bpt_iter_b(&wu,
                      oc_bpt_test_utl_get_state(s_p),
                      iter_fun);
    }    


    // compare the block maps
    rc = oc_bpt_test_fs_alt_compare(num_clones);

    if (!rc)
        ERR(("The free-space block-map is incorrect"));
    
    // Release the alternate block-map
    oc_bpt_test_fs_alt_free();
}

/******************************************************************/

void oc_bpt_test_clone_init_ds(void)
{
    clone_array = pl_mm_malloc(
        sizeof(struct Oc_bpt_test_state *) * max_num_clones);
    
    memset(clone_array,
           0,
           sizeof(struct Oc_bpt_test_state *) * max_num_clones);
}

bool oc_bpt_test_clone_can_create_more(void)
{
    return (num_clones < max_num_clones);
}

bool oc_bpt_test_clone_num_live(void)
{
    return num_clones;
}

void oc_bpt_test_clone_display_all(void)
{
    // Display the tree with sharing
    oc_bpt_test_utl_display_all(num_clones, clone_array);
}

bool oc_bpt_test_clone_validate_all(void)
{
    int i;
    
    if (!oc_bpt_test_utl_btree_validate_clones(num_clones, clone_array))
        return FALSE;
    
    for (i=0; i< num_clones; i++) {
        struct Oc_bpt_test_state *s_p = clone_array[i];
        
        if (!oc_bpt_test_utl_btree_compare_and_verify(s_p))
            return FALSE;
    }

    oc_bpt_test_clone_validate_fs();
    
    return TRUE;
}

bool oc_bpt_test_clone_just_validate_all(void)
{
    int i;
    
    for (i=0; i< num_clones; i++) {
        struct Oc_bpt_test_state *s_p = clone_array[i];
        
        if (!oc_bpt_test_utl_btree_validate(s_p))
            return FALSE;
    }
    
    return TRUE;
}

void oc_bpt_test_clone_add(struct Oc_bpt_test_state *s_p)
{
    if (max_num_clones == num_clones)
        ERR(("The array of clones is full"));

    clone_array[num_clones] = s_p;
    num_clones++;
}

void oc_bpt_test_clone_delete(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state *s_p)
{
    int loc = -1;
    int i;
    
    for (i=0; i<num_clones; i++)
        if (s_p == clone_array[i]) {
            loc = i;
            break;
        }

    if (-1 == loc)
        ERR(("The clone was not found in the array"));        

    clone_array[loc] = NULL;

    // move the clones after [loc] one step back
    for (i=loc+1; i<num_clones; i++)
        clone_array[i-1] = clone_array[i];

    num_clones--;
    clone_array[num_clones] = NULL;

    // actually destroy the tree
    oc_bpt_test_utl_btree_delete(wu_p, s_p);
    oc_bpt_test_utl_btree_destroy(s_p); 
}

// Choose a random clone 
struct Oc_bpt_test_state *oc_bpt_test_clone_choose(void)
{
    int i;
    
    if (0 == num_clones) return NULL;
    i = oc_bpt_test_utl_random_number(num_clones);    
    return clone_array[i];
}

void oc_bpt_test_clone_delete_all(struct Oc_wu *wu_p)
{
    int i, tmp_num_clones;
    struct Oc_bpt_test_state *s_p;

    tmp_num_clones = num_clones;
    num_clones = 0;
    
    for (i=0; i<tmp_num_clones; i++) {
        s_p = clone_array[i];
        oc_utl_assert(s_p != NULL);
        
        clone_array[i] = NULL;
        
        oc_bpt_test_utl_btree_delete(wu_p, s_p);
        oc_bpt_test_utl_btree_destroy(s_p); 
    }
}

/******************************************************************/

