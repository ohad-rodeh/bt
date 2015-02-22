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
/* OC_XT_TEST_ND.H
 *
 * Handling of meta-data nodes 
 */
/**********************************************************************/ 
#ifndef OC_XT_TEST_VD_H
#define OC_XT_TEST_VD_H

#include "pl_base.h"
#include "oc_xt_int.h"

#define OC_XT_TEST_ND_SIZE (1256)

Oc_xt_node* oc_xt_test_nd_alloc(struct Oc_wu *wu_p);
void oc_xt_test_nd_dealloc(struct Oc_wu *wu_p, uint64 addr);
Oc_xt_node* oc_xt_test_nd_get(struct Oc_wu *wu_p, uint64 addr);
void oc_xt_test_nd_release(struct Oc_wu *wu_p, Oc_xt_node *node_p);
void oc_xt_test_nd_mark_dirty(struct Oc_wu *wu_p, Oc_xt_node *node_p);

int  oc_xt_test_nd_get_refcount(void);

void oc_xt_test_nd_init(void);
#endif
