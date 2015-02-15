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
// OC_XT_ALT.H
// An alternate implementation of a b-tree. Uses a linked-list.
/******************************************************************/
#ifndef OC_XT_ALT_H
#define OC_XT_ALT_H

#include "oc_xt_int.h"
#include "pl_base.h"
#include "pl_dstru.h"
 
struct Oc_wu;

// A sorted list of records
typedef struct Oc_xt_alt_state {
    Ss_dlist list;
    Oc_xt_cfg *cfg_p;
} Oc_xt_alt_state;

void oc_xt_alt_init_state_b(
    struct Oc_wu *wu_pi,
    struct Oc_xt_alt_state *state_p,
    Oc_xt_cfg *cfg_p);

void oc_xt_alt_create_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_alt_state *s_p);

void oc_xt_alt_delete_b(
    struct Oc_wu *wu_p,
    struct Oc_xt_alt_state *s_p);

bool oc_xt_alt_dbg_validate_b(struct Oc_wu *wu_p,
                              Oc_xt_alt_state *s_p);
void oc_xt_alt_dbg_output_b(struct Oc_wu *wu_p,
                            Oc_xt_alt_state *s_p);

void oc_xt_alt_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_alt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p,
    int max_num_keys_i,
    struct Oc_xt_key *key_array_po,
    struct Oc_xt_rcrd *rcrd_array_po,
    int *n_found_po);

uint64 oc_xt_alt_insert_b(
    struct Oc_wu *wu_p,
    Oc_xt_alt_state *s_p,
    struct Oc_xt_key *key_p,
    struct Oc_xt_rcrd *rcrd_p);

uint64 oc_xt_alt_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_xt_alt_state *s_p,
    struct Oc_xt_key *min_key_p,
    struct Oc_xt_key *max_key_p);

#endif
