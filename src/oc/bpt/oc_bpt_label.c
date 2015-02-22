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
/* OC_BPT_LABEL.C
 *
 * Utility for labeling nodes. 
 */
/**********************************************************************/
#include <string.h>

#include "oc_utl_htbl.h"
#include "oc_bpt_label.h"
#include "pl_dstru.h"
#include "pl_mm_int.h"

static uint32 oc_bpt_label_hash(void* _disk_addr, int num_buckets, int dummy);
static bool oc_bpt_label_compare(void *_elem, void* _disk_addr);

/**********************************************************************/
/* build a hash-table with 10000 entries. This really needs to
 * be extensible, but we'll start with a staticly sized one.
 *
 * When a node-get-label call is made, check if the cell already
 * exists. If so, return a pointer to the integer value. Otherwise,
 * create a new cell. 
 */
static Oc_utl_htbl htbl;

typedef struct Oc_bpt_label_cell {
    Ss_slist_node node;
    uint64 addr;
    int val;
} Oc_bpt_label_cell;


static uint32 oc_bpt_label_hash(void* _disk_addr, int num_buckets, int dummy)
{
    uint64* disk_addr_p = (uint64*)_disk_addr;
    
    return (*disk_addr_p / 35291) % num_buckets;
}

static bool oc_bpt_label_compare(void *_elem, void* _disk_addr)
{
    uint64 *disk_addr_p = (uint64*)_disk_addr;
    Oc_bpt_label_cell *cell_p = (Oc_bpt_label_cell *)_elem;

    return cell_p->addr == *disk_addr_p;
}

static Oc_bpt_label_cell* oc_bpt_label_lookup(uint64 disk_addr)
{
    return (Oc_bpt_label_cell*)oc_utl_htbl_lookup(&htbl, &disk_addr);
}


static void oc_bpt_label_insert(Oc_bpt_label_cell *cell_p)
{
    oc_utl_htbl_insert(&htbl,
                       (void*)&cell_p->addr,
                       (void*)cell_p);
}

/**********************************************************************/

void oc_bpt_label_init(int max_n_nodes)
{
    oc_utl_htbl_create(&htbl,
                       max_n_nodes,
                       NULL,
                       oc_bpt_label_hash,
                       oc_bpt_label_compare);
}

// Get a pointer to the node label
int* oc_bpt_label_get(struct Oc_wu *wu_p, Oc_bpt_node *node_p)
{
    uint64 disk_addr = node_p->disk_addr;
    Oc_bpt_label_cell *cell_p;
    
    /* Lookup the cell.
     *
     * If it exists, return a pointer to the value.
     */
    cell_p = oc_bpt_label_lookup(disk_addr);
    if (cell_p != NULL)
        return &cell_p->val;

    /* Otherwise, create
     * a cell and insert it into the table.
     */

    // create a new cell 
    cell_p = (Oc_bpt_label_cell*) pl_mm_malloc(sizeof(Oc_bpt_label_cell));
    memset(cell_p, 0, sizeof(Oc_bpt_label_cell));
    cell_p->addr = disk_addr;

    // insert into the table
    oc_bpt_label_insert(cell_p);
    return &cell_p->val;
}

/**********************************************************************/

static bool oc_bpt_label_true_fun(void *elem, void *data)
{
    return TRUE;
}

// Release the labels hash-table
void oc_bpt_label_free(void)
{
    static Ss_slist cells;

    // extract the cells from the hash-table 
    ssslist_init(&cells);
    oc_utl_htbl_iter_mv_to_list(&htbl, oc_bpt_label_true_fun, NULL, &cells);

    // release all the cells
    while (!ssslist_empty(&cells)) {
        Oc_bpt_label_cell *cell_p;
        
        cell_p = (Oc_bpt_label_cell*) ssslist_remove_head(&cells);
        pl_mm_free(cell_p);
    }

    // release the hash-table
    oc_utl_htbl_free(&htbl);
    memset(&htbl, 0, sizeof(htbl));
}

/**********************************************************************/
