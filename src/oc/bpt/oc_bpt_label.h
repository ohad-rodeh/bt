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
/* OC_BPT_LABEL.H
 *
 * A utility for labeling nodes. This is needed for cloning validation
 * tests. We'd like to be able to mark nodes, however, this can't be done
 * on the nodes themselves without marking them dirty. Therefore, we keep an
 * extra data-structure in memory. 
 *
 * Implementation: 
 *   build a hash-table with 10000 entries. This really needs to
 * be extensible, but we'll start with a staticly sized one.
 *
 * When a node-get-label call is made, check if the cell already
 * exists. If so, return a pointer to the integer value. Otherwise,
 * create a new cell. 
 */
/**********************************************************************/
#ifndef OC_BPT_LABEL_H
#define OC_BPT_LABEL_H

#include "oc_bpt_int.h"

/* Create a structure that will hold labels. The [n_nodes_hint] value
 * is the maximal number of nodes that will exist.
 *
 * There is only a single label-structure in the system. The caller
 * needs to make sure that he does not need multiple instances. All
 * operations occur in-memory, no disk-IO is performed.
 */
void oc_bpt_label_init(int max_n_nodes);

// Get a pointer to the node label
int* oc_bpt_label_get(struct Oc_wu*, Oc_bpt_node*);

// Release the labels hash-table
void oc_bpt_label_free(void);

#endif
