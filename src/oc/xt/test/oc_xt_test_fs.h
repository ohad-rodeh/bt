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
/* OC_XT_TEST_FS.H
 *
 * A free-space implementation that helps tests the correct functions
 * of the x-tree.
 */
/**********************************************************************/
#ifndef OC_XT_TEST_FS_H
#define OC_XT_TEST_FS_H

#include "pl_base.h"

struct Oc_xt_test_fs_ctx;

// create an FS instance
struct Oc_xt_test_fs_ctx *oc_xt_test_fs_create(
    char *str_desc_p,
    bool verbose);

// delete an FS instance
//void oc_xt_test_fs_reset(struct Oc_xt_test_fs_ctx *ctx_p);

/* allocate an extent of [len] units in free-space [ctx_p].
 * return the initial offset of the allocated extent.
 */
uint32 oc_xt_test_fs_alloc(struct Oc_xt_test_fs_ctx *ctx_p,
                           uint32 len);
    
// deallocate an extent of [len] units in free-space [ctx_p]
void oc_xt_test_fs_dealloc(struct Oc_xt_test_fs_ctx *ctx_p,
                           uint32 ofs,
                           uint32 len);

/* compare two free-space instances. Return TRUE if they are equal, FALSE
 * otherwise.
 */
bool oc_xt_test_fs_compare(struct Oc_xt_test_fs_ctx *ctx1_p,
                           struct Oc_xt_test_fs_ctx *ctx2_p);

#endif
/**********************************************************************/
