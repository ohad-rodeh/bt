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
/* OC_XT_TEST_VD.H
 *
 * A virtual-disk used to store the meta-date nodes.
 */
/**********************************************************************/
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "pl_dstru.h"
#include "oc_utl.h"
#include "oc_utl_htbl.h"
#include "oc_crt_int.h"
#include "oc_xt_nd.h"
#include "oc_xt_test_nd.h"

#define MAGIC (2718)

static Oc_utl_htbl vd_htbl;
static int g_refcnt = 0;

typedef struct Oc_xt_test_node {
    Ss_slist_node link;       // link inside the hash-table    
    Oc_xt_node node;
    int    magic;             // magic number, for testing purposes
} Oc_xt_test_node;

// virtual-disk section
static uint32 vd_hash(void *_key, int num_buckets, int dummy);
static bool vd_compare(void *_elem, void *_key);
static void vd_create(void);
static void vd_node_remove(uint64 addr);
static void vd_node_insert(Oc_xt_test_node *node_p);
static Oc_xt_test_node *vd_node_lookup(uint64 addr);

// emulation of free-space
static uint64 fs_alloc(void);
static uint32 random_choose(uint32 top);

/**********************************************************************/
/*
 * An implementation of a virtual disk (vd). This is used to test the
 * mark-dirty operation that can move a the physical location of a
 * page from address X to address Y.
 *
 * Details:
 * - a hashtable H defines a mapping between [disk-address] and
 *   [Oc_xt_test_node]
 */

static uint32 vd_hash(void *_key, int num_buckets, int dummy)
{
    uint64 disk_addr = *((uint64*) _key);

    return (disk_addr * 1069597 + 1066133) & (num_buckets-1);
}

static bool vd_compare(void *_elem, void *_key)
{
    uint64 disk_addr = *((uint64*) _key);
    Oc_xt_test_node *tnode_p = (Oc_xt_test_node *) _elem;

    return (tnode_p->node.disk_addr == disk_addr);
}

// create a hashtable
static void vd_create(void)
{
    oc_utl_htbl_create(
        &vd_htbl,
        2048,
        NULL,
        vd_hash,
        vd_compare);
}

// remove a node
static void vd_node_remove(uint64 addr)
{
    bool rc;

    rc = oc_utl_htbl_remove(&vd_htbl, (void*)&addr);
    oc_utl_assert(rc);
}

// insert a node
static void vd_node_insert(Oc_xt_test_node *tnode_p)
{
    oc_utl_htbl_insert(&vd_htbl, (void*)&tnode_p->node.disk_addr, (void*)tnode_p);
}

static Oc_xt_test_node *vd_node_lookup(uint64 addr)
{
    return (Oc_xt_test_node*)oc_utl_htbl_lookup(&vd_htbl, (void*)&addr);
}

/**********************************************************************/

// A trivial emulation of free-space. Always allocate the next page.
static uint64 fs_alloc(void)
{
    static uint64 fs = 1;

    return fs++;
}

/**********************************************************************/
// create a random number between 0 and [top]
static uint32 random_choose(uint32 top)
{
    if (0 == top) return 0;
    return (uint32) (rand() % top);
}


static void *wrap_malloc(int size)
{
    void *p = malloc(size);

    oc_utl_assert(p != NULL);
    return p;
}


Oc_xt_node* oc_xt_test_nd_alloc(Oc_wu *wu_p)
{
    Oc_xt_test_node *tnode_p;

    // simple checking for ref-counts
    g_refcnt++;    

    if (wu_p->po_id != 0)
        if (random_choose(2) == 0)
            oc_crt_yield_task();
    
    tnode_p = (struct Oc_xt_test_node*) wrap_malloc(sizeof(Oc_xt_test_node));
    memset(tnode_p, 0, sizeof(Oc_xt_test_node));
    oc_crt_init_rw_lock(&tnode_p->node.lock);
    tnode_p->node.data = (char*) wrap_malloc(OC_XT_TEST_ND_SIZE);
    tnode_p->node.disk_addr = fs_alloc();
    tnode_p->magic = MAGIC;

    // add the node into the virtual-disk
    vd_node_insert(tnode_p);
    
    return &tnode_p->node;
}
     
void oc_xt_test_nd_dealloc(Oc_wu *wu_p, uint64 addr)
{
    Oc_xt_test_node *tnode_p;
    char *data;
    
    if (wu_p->po_id != 0)
        if (random_choose(2) == 0)
            oc_crt_yield_task();

    // extract the node fromt the table prior to removal
    tnode_p = vd_node_lookup(addr);
    
    // remove the node from the virtual-disk
    vd_node_remove(addr);

    oc_utl_assert(tnode_p);
    oc_utl_assert(tnode_p->node.data);
    oc_utl_assert(tnode_p->magic == MAGIC);

    // mess up the node so that errors will occur on illegal accesses
    tnode_p->magic = -1;
    data = tnode_p->node.data;
    tnode_p->node.data = NULL;
    tnode_p->node.disk_addr = 0;
    
    free(data);
    free(tnode_p);
}

Oc_xt_node* oc_xt_test_nd_get(Oc_wu *wu_p, uint64 addr)
{
    Oc_xt_test_node *tnode_p;
    
    oc_utl_assert(addr != 0);

    if (wu_p->po_id != 0)
        if (random_choose(2) == 0)
            oc_crt_yield_task();

    g_refcnt++;
    tnode_p = vd_node_lookup(addr);
    if (NULL == tnode_p) {
        printf("error, did not find a b-tree node at address=%Lu po=%lu\n",
               addr, wu_p->po_id);
        oc_utl_assert(0);
    }
    oc_utl_assert(MAGIC == tnode_p->magic);
    return &tnode_p->node;
}

void oc_xt_test_nd_release(Oc_wu *wu_p, Oc_xt_node *node_p)
{
    g_refcnt--;
    oc_utl_assert(g_refcnt>=0);
}

void oc_xt_test_nd_mark_dirty(Oc_wu *wu_p, Oc_xt_node *node_p)
{
    Oc_xt_test_node *tnode_p;
    uint64 new_addr;
    
    if (random_choose(4) != 0)
        return;

    /* Try to emulate what the mark-dirty operation actually does.
     * Move the page from its current disk-address to a new one.
     *
     * There is one exception: never move the root node. 
     */
    if (oc_xt_nd_is_root(NULL, node_p))
        return;

    // This is a non-root node, move it
    new_addr = fs_alloc();
//    printf("po_id=%d, moving address %Lu->%Lu\n", wu_p->po_id, node_p->lba, new_addr);
//    fflush(stdout);
    
    tnode_p = vd_node_lookup(node_p->disk_addr);
    vd_node_remove(node_p->disk_addr);
    node_p->disk_addr = new_addr;
    vd_node_insert(tnode_p);
}

/**********************************************************************/

int  oc_xt_test_nd_get_refcount(void)
{
    return g_refcnt;
}

/**********************************************************************/

void oc_xt_test_nd_init(void)
{
    vd_create();
}
