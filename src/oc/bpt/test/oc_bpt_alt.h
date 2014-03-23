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
// OC_BPT_ALT.H
// An alternate implementation of a b-tree. Uses a linked-list.
/******************************************************************/
#ifndef OC_BPT_ALT_H
#define OC_BPT_ALT_H

#include "oc_bpt_int.h"
#include "pl_base.h"

struct Oc_wu;

typedef struct Oc_bpt_alt_cfg {
    uint32 key_size;
    uint32 data_size;
    
    /* compare keys [key1] and [key2].
     *  return 0 if keys are equal
     *        -1 if key1 > key2
     *         1 if key1 < key2
     */
    int         ((*key_compare)(struct Oc_bpt_key *key1_p,
                                struct Oc_bpt_key *key2_p));
    
    // increment [key_p]. put the result in [result_p]
    void        ((*key_inc)(struct Oc_bpt_key *key_p,
                            struct Oc_bpt_key *result_p));
    
    // string representation of a key
    void        ((*key_to_string)(struct Oc_bpt_key *key_p,
                                  char *str_p,
                                  int max_len));
    
    /* release data. This is useful if the data is, like in an s-node,
     * an extent allocated on disk.
     */
    void        ((*data_release)(struct Oc_wu *wu_p,
                                 struct Oc_bpt_data *data));
    
    // string representation of data
    void        ((*data_to_string)(struct Oc_bpt_data *data_p,
                                   char *str_p,
                                   int max_len));
} Oc_bpt_alt_cfg;

// A sorted list of records
typedef struct Oc_bpt_alt_state {
    Ss_dlist list;
    Oc_bpt_alt_cfg *cfg_p;
} Oc_bpt_alt_state;

void oc_bpt_alt_init_state_b(
    struct Oc_wu *wu_pi,
    struct Oc_bpt_alt_state *state_p,
    Oc_bpt_alt_cfg *cfg_p);

void oc_bpt_alt_create_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p);

// return TRUE if this is a case of replacement. FALSE otherwise.
bool oc_bpt_alt_insert_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_p);

bool oc_bpt_alt_lookup_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *key_p,
    struct Oc_bpt_data *data_po);    

bool oc_bpt_alt_remove_key_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *key_p);

void oc_bpt_alt_delete_b(
    struct Oc_wu *wu_p,
    struct Oc_bpt_alt_state *s_p);

bool oc_bpt_alt_dbg_validate_b(struct Oc_wu *wu_p,
                           Oc_bpt_alt_state *s_p);
void oc_bpt_alt_dbg_output_b(struct Oc_wu *wu_p,
                             Oc_bpt_alt_state *s_p);


/* range operations
 */
void oc_bpt_alt_lookup_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p,
    int max_num_keys_i,
    struct Oc_bpt_key *key_array_po,
    struct Oc_bpt_data *data_array_po,
    int *nkeys_found_po);

int oc_bpt_alt_insert_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_alt_state *s_p,
    int length,
    struct Oc_bpt_key *key_array,
    struct Oc_bpt_data *data_array);

int oc_bpt_alt_remove_range_b(
    struct Oc_wu *wu_p,
    Oc_bpt_alt_state *s_p,
    struct Oc_bpt_key *min_key_p,
    struct Oc_bpt_key *max_key_p);

uint64 oc_bpt_alt_clone_b(
    struct Oc_wu *wu_p,
    Oc_bpt_alt_state *src_p,
    Oc_bpt_alt_state *trg_p);

#endif
