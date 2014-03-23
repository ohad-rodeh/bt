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

struct Oc_wu;
struct Oc_bpt_test_state;

void oc_bpt_test_clone_init_ds(void);
bool oc_bpt_test_clone_can_create_more(void);
bool oc_bpt_test_clone_num_live(void);
void oc_bpt_test_clone_display_all(void);

/* Validate the set of clones and compare against linked-lists.
 * Return FALSE if there is a mismatch or a b-tree is invalid. 
 */
bool oc_bpt_test_clone_validate_all(void);

/* Same as [oc_bpt_test_clone_validate_all] except that only
 * validate is performed. 
 */
bool oc_bpt_test_clone_just_validate_all(void);

void oc_bpt_test_clone_add(struct Oc_bpt_test_state *s_p);
void oc_bpt_test_clone_delete(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state *s_p);
struct Oc_bpt_test_state *oc_bpt_test_clone_choose(void);
void oc_bpt_test_clone_delete_all(struct Oc_wu *wu_p);


/******************************************************************/

