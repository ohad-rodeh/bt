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
/* OC_BPT_TEST_FS.H
 *
 * A free-space implementation that helps tests the correct 
 * functioning of the b-tree. 
 */
/**********************************************************************/
#ifndef OC_BPT_TEST_FS_H
#define OC_BPT_TEST_FS_H

#include "pl_base.h"

struct Oc_wu;
struct Oc_rm_resource;

// create an FS instance
void oc_bpt_test_fs_create(
    char *str_desc_p,
    int num_blocks,
    bool verbose);

/* Allocate a block. Return the initial offset of the allocated block.
 */
uint64 oc_bpt_test_fs_alloc(void);
    
// deallocate a block 
void oc_bpt_test_fs_dealloc(uint64 addr);

// increment the ref-count for the page in address [addr]
void oc_bpt_test_fs_inc_refcount(struct Oc_wu *wu_p, uint64 addr);

// return the ref-count of the page at address [addr]
int oc_bpt_test_fs_get_refcount(struct Oc_wu *wu_p, uint64 addr);

void oc_bpt_test_fs_query_dealloc( struct Oc_rm_resource *r_p, int n_pages);
void oc_bpt_test_fs_query_alloc( struct Oc_rm_resource *r_p, int n_pages);

// Verify the free-space has [num_blocks] allocated
void oc_bpt_test_fs_verify(int num_blocks);

/**********************************************************************/
// Verifying that free-space has the correct allocation map

// Create an alternate block-map
void oc_bpt_test_fs_alt_create(void);

// Increase the ref-count for one of the blocks
void oc_bpt_test_fs_alt_inc_refcount(struct Oc_wu *wu_p, uint64 addr);

// compare the alternate block-map 
bool oc_bpt_test_fs_alt_compare(int num_clones);

// Release the alternate block map 
void oc_bpt_test_fs_alt_free(void);

#endif
/**********************************************************************/
