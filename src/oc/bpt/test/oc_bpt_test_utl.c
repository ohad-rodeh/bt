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
/* OC_BPT_TEST_UTL.H
 *
 * Utilities for testing
 */
/**********************************************************************/
#include <malloc.h>
#include <string.h>
#include <stdlib.h>
#include <alloca.h>

#include "pl_trace_base.h"
#include "pl_int.h"
#include "oc_utl_htbl.h"
#include "oc_utl_trk.h"
#include "oc_bpt_int.h"
#include "oc_bpt_nd.h"
#include "oc_bpt_alt.h"
#include "oc_bpt_test_utl.h"
#include "oc_bpt_test_fs.h"

/**********************************************************************/

#define MAGIC (2718)

static Oc_bpt_cfg cfg;
static Oc_bpt_alt_cfg alt_cfg;

// set default values
static Oc_bpt_test_param param_global = {
    .num_rounds = 100,
    .max_int = 200,
    .num_tasks = 10,
    .num_ops_per_task = 10,
    .max_num_clones = 2,
    .verbose = FALSE,
    .statistics = FALSE,
    .max_root_fanout = 5,
    .max_non_root_fanout = 5,
    .min_fanout = 2,
    .total_ops = 0};
static Oc_bpt_test_param *param = &param_global;


#define NODE_SIZE (1256)
typedef uint32 Oc_bpt_test_key;
typedef uint32 Oc_bpt_test_data;

Oc_bpt_test_utl_type test_type = OC_BPT_TEST_UTL_ANY;

// work-unit used for validate and display
static Oc_wu utl_wu;
static Oc_rm_ticket utl_rm_ticket;

typedef struct Oc_bpt_test_node {
    Ss_slist_node link;       // link inside the hash-table
    Oc_bpt_node node;
    int    magic;             // magic number, for testing purposes
} OC_PACKED Oc_bpt_test_node;

// A structure that encapsulates both implementations of a b-tree
typedef struct Oc_bpt_test_state {
    Oc_bpt_state bpt_s;
    Oc_bpt_alt_state alt_s;
} Oc_bpt_test_state;

// Lock to ensure operation atomicity of the infrastructure. This is
// required for the testing harness, not for the core b-tree
// algorithms. In a real file-system, the free space and memory management
// will have their own, highly concurrent, method of ensuring atomicity.
static Oc_crt_rw_lock g_lock;

static void (*print_fun)(void) = NULL;
static bool (*validate_fun)(void) = NULL;

/**********************************************************************/

static uint64 get_tid(Oc_bpt_test_state *s_p);

// virtual-disk section
static uint32 vd_hash(void *_key, int num_buckets, int dummy);
static bool vd_compare(void *_elem, void *_key);
static void vd_create(void);
static void vd_node_remove(uint64 addr);
static void vd_node_insert(Oc_bpt_test_node *node_p);
static Oc_bpt_test_node *vd_node_lookup(uint64 addr);

static Oc_bpt_test_node *tnode_clone(Oc_bpt_test_node *tnode_p);

static void *wrap_malloc(int size);
static Oc_bpt_node* node_alloc(struct Oc_wu *wu_p);
static void node_dealloc(struct Oc_wu *wu_p, uint64 _node_p);
static Oc_bpt_node* node_get(struct Oc_wu *wu_p, uint64 addr);
static Oc_bpt_node* node_get_sl(Oc_wu *wu_p, uint64 addr);
static Oc_bpt_node* node_get_xl(Oc_wu *wu_p, uint64 addr);
static void node_release(struct Oc_wu *wu_p, Oc_bpt_node *node_p);
static void node_mark_dirty(Oc_wu *wu_p,
                            Oc_bpt_node *node_p,
                            bool multiple_refs) ;
static int key_compare(struct Oc_bpt_key *key1_p,
                       struct Oc_bpt_key *key2_p);
static void key_inc(struct Oc_bpt_key *_key_p, struct Oc_bpt_key *_result_p);
static void key_to_string(struct Oc_bpt_key *key_p, char *str_p, int max_len);
static void data_release(struct Oc_wu *wu_p, struct Oc_bpt_data *data);
static void data_to_string(struct Oc_bpt_data *data_p, char *str_p, int max_len);

/**********************************************************************/

static void *wrap_malloc(int size)
{
    void *p = malloc(size);

    oc_utl_assert(p != NULL);
    return p;
}


/**********************************************************************/
/* An implementation of a virtual disk (vd). This is used to test the
 * mark-dirty operation that can move a the physical location of a
 * page from address X to address Y.
 *
 * Details:
 * - a hashtable H defines a mapping between [disk-address] and
 *   [Oc_bpt_test_node]
 */

static Oc_utl_htbl vd_htbl;

static uint32 vd_hash(void *_key, int num_buckets, int dummy)
{
    uint64 lba = *((uint64*) _key);

    return (lba * 1069597 + 1066133) & (num_buckets-1);
}

static bool vd_compare(void *_elem, void *_key)
{
    uint64 disk_addr = *((uint64*) _key);
    Oc_bpt_test_node *tnode_p = (Oc_bpt_test_node *) _elem;

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
static void vd_node_insert(Oc_bpt_test_node *tnode_p)
{
    oc_utl_htbl_insert(&vd_htbl, (void*)&tnode_p->node.disk_addr, (void*)tnode_p);
}

static Oc_bpt_test_node *vd_node_lookup(uint64 addr)
{
    return (Oc_bpt_test_node*)oc_utl_htbl_lookup(&vd_htbl, (void*)&addr);
}

/**********************************************************************/

static Oc_bpt_node* node_alloc(Oc_wu *wu_p)
{
    Oc_bpt_test_node *tnode_p;
    Oc_bpt_node *node_p;

    if (wu_p->po_id != 0)
        if (oc_bpt_test_utl_random_number(2) == 0)
            oc_crt_yield_task();

    tnode_p = (struct Oc_bpt_test_node*) wrap_malloc(sizeof(Oc_bpt_test_node));
    memset(tnode_p, 0, sizeof(Oc_bpt_test_node));
    oc_crt_init_rw_lock(&tnode_p->node.lock);

    oc_crt_lock_write(&g_lock);
    {
        tnode_p->node.data = (char*) wrap_malloc(NODE_SIZE);
        tnode_p->node.disk_addr = oc_bpt_test_fs_alloc();
        tnode_p->magic = MAGIC;

        // add the node into the virtual-disk
        vd_node_insert(tnode_p);

        node_p = &tnode_p->node;
    }
    oc_crt_unlock(&g_lock);

    // Lock the node
    oc_utl_trk_crt_lock_write(wu_p, &node_p->lock);

    return node_p;
}

static void node_dealloc(Oc_wu *wu_p, uint64 addr)
{
    if (wu_p->po_id != 0)
        if (oc_bpt_test_utl_random_number(2) == 0)
            oc_crt_yield_task();

    oc_crt_lock_write(&g_lock);
    {
        oc_bpt_test_fs_dealloc(addr);

        if (oc_bpt_test_fs_get_refcount(wu_p, addr) == 0)
        {
        // Free the block only if it's ref-count is zero
            Oc_bpt_test_node *tnode_p;
            char *data;

            // extract the node from the table prior to removal
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
    }
    oc_crt_unlock(&g_lock);
}

static Oc_bpt_node* node_get(Oc_wu *wu_p, uint64 addr)
{
    Oc_bpt_test_node *tnode_p;

    oc_utl_assert(addr != 0);

    if (wu_p->po_id != 0)
        if (oc_bpt_test_utl_random_number(2) == 0)
            oc_crt_yield_task();

    oc_crt_lock_read(&g_lock);
    {
        tnode_p = vd_node_lookup(addr);
        if (NULL == tnode_p) {
            printf("error, did not find a b-tree node at address=%Lu po=%lu\n",
                   addr, wu_p->po_id);
            oc_utl_assert(0);
        }
        oc_utl_assert(MAGIC == tnode_p->magic);
    }
    oc_crt_unlock(&g_lock);

    return &tnode_p->node;
}

static Oc_bpt_node* node_get_sl(Oc_wu *wu_p, uint64 addr)
{
    Oc_bpt_node *node_p;

    while (1) {
        node_p = node_get(wu_p, addr);
        oc_utl_trk_crt_lock_read(wu_p, &node_p->lock);
        if (node_p->disk_addr != addr) {
            node_release(wu_p, node_p);
        } else
            break;
    }
    oc_utl_debugassert(addr == node_p->disk_addr);

    return node_p;
}

static Oc_bpt_node* node_get_xl(Oc_wu *wu_p, uint64 addr)
{
    Oc_bpt_node *node_p;

    while (1) {
        node_p = node_get(wu_p, addr);
        oc_utl_trk_crt_lock_write(wu_p, &node_p->lock);
        if (node_p->disk_addr != addr) {
            node_release(wu_p, node_p);
        } else
            break;
    }
    oc_utl_debugassert(addr == node_p->disk_addr);

    return node_p;
}

static void node_release(Oc_wu *wu_p, Oc_bpt_node *node_p)
{
    oc_utl_trk_crt_unlock(wu_p, &node_p->lock);
}

static Oc_bpt_test_node *tnode_clone(Oc_bpt_test_node *tnode_p)
{
    Oc_bpt_test_node *new_tnode_p;

    new_tnode_p =
        (Oc_bpt_test_node *) wrap_malloc(sizeof(Oc_bpt_test_state));

    memset(new_tnode_p, 0, sizeof(Oc_bpt_test_state));
    new_tnode_p->node.disk_addr = tnode_p->node.disk_addr;
    new_tnode_p->node.data = wrap_malloc(NODE_SIZE);
    memcpy(new_tnode_p->node.data, tnode_p->node.data, NODE_SIZE);
    oc_crt_init_rw_lock(&new_tnode_p->node.lock);
    new_tnode_p->magic = MAGIC;

    return new_tnode_p;
}

static void node_mark_dirty(Oc_wu *wu_p,
                            Oc_bpt_node *node_p,
                            bool multi_refs)
{
    if (!multi_refs) {
        uint64 new_addr;
        Oc_bpt_test_node *tnode_p;

        /* Try to emulate what the mark-dirty operation actually does.
         * Move the page from its current disk-address to a new one.
         *
         * There is one exception: never move the root node.
         */
        if (oc_bpt_nd_is_root(NULL, node_p))
            return;

        if (oc_bpt_test_utl_random_number(4) != 0)
            return;

        // This is a non-root node, move it
        oc_crt_lock_write(&g_lock);
        {
            new_addr = oc_bpt_test_fs_alloc();
            oc_bpt_test_fs_dealloc(node_p->disk_addr);

            tnode_p = vd_node_lookup(node_p->disk_addr);
            vd_node_remove(node_p->disk_addr);
            node_p->disk_addr = new_addr;
            vd_node_insert(tnode_p);
        }
        oc_crt_unlock(&g_lock);
    }
    else {
        /* This page is referenced by multiple clones. We
         * can't move it; we need to leave the old page where
         * it was.
         */
        Oc_bpt_test_node *tnode_p, *old_tnode_p;;

        oc_crt_lock_write(&g_lock);
        {
            tnode_p = vd_node_lookup(node_p->disk_addr);
            old_tnode_p = tnode_clone(tnode_p);

            // move the page to a new address
            vd_node_remove(tnode_p->node.disk_addr);
            tnode_p->node.disk_addr = oc_bpt_test_fs_alloc();
            vd_node_insert(tnode_p);

            // Place a copy of the page in the old disk-address
            vd_node_insert(old_tnode_p);

            // reduce the FS ref-count on the old address.
            oc_bpt_test_fs_dealloc(old_tnode_p->node.disk_addr);
        }
        oc_crt_unlock(&g_lock);
    }
}

static int key_compare(struct Oc_bpt_key *key1_p,
                       struct Oc_bpt_key *key2_p)
{
    Oc_bpt_test_key *key1 = (uint32*) key1_p;
    Oc_bpt_test_key *key2 = (uint32*) key2_p;

    if (*key1 == *key2) return 0;
    else if (*key1 > *key2) return -1;
    else return 1;
}

static void key_inc(struct Oc_bpt_key *_key_p, struct Oc_bpt_key *_result_p)
{
    Oc_bpt_test_key *key_p = (uint32*) _key_p;
    Oc_bpt_test_key *result_p = (uint32*) _result_p;
    *result_p = *key_p;
    (*result_p)++;
}


static void key_to_string(struct Oc_bpt_key *key_p, char *str_p, int max_len)
{
    if (max_len < 5)
        ERR(("key_to_string: %d is not enough", max_len));
    sprintf(str_p, "%lu", *((uint32*)key_p));
}


static void data_release(Oc_wu *wu_p, struct Oc_bpt_data *data)
{
    if (wu_p->po_id != 0)
        if (oc_bpt_test_utl_random_number(3) == 0)
            oc_crt_yield_task();
}

static void data_to_string(struct Oc_bpt_data *data_p, char *str_p, int max_len)
{
    if (max_len < 5)
        ERR(("data_to_string: %d is not enough", max_len));
    sprintf(str_p, "%lu", *((uint32*)data_p));
}

void oc_bpt_test_utl_finalize(int refcnt)
{
    // TODO: check that all node ref-counts are zero.
}


/******************************************************************/

static uint64 get_tid(Oc_bpt_test_state *s_p)
{
    return oc_bpt_get_tid(&s_p->bpt_s);
}

/******************************************************************/
void oc_bpt_test_utl_setup_wu(Oc_wu *wu_p, Oc_rm_ticket *rm_p)
{
    static int po_id =0;

    memset(wu_p, 0, sizeof(Oc_wu));
    memset(rm_p, 0, sizeof(Oc_rm_ticket));
    wu_p->po_id = po_id++;
    wu_p->rm_p = rm_p;
}

Oc_bpt_test_state *oc_bpt_test_utl_btree_init(struct Oc_wu *wu_p, uint64 tid)
{
    Oc_bpt_test_state *s_p;

    s_p = (Oc_bpt_test_state *) wrap_malloc(sizeof(Oc_bpt_test_state));

    oc_bpt_init_state_b(NULL, &s_p->bpt_s, &cfg, tid);
    oc_bpt_alt_init_state_b(NULL, &s_p->alt_s, &alt_cfg);

    return s_p;
}

void oc_bpt_test_utl_btree_destroy(
    Oc_bpt_test_state *s_p)
{
    free(s_p);
}

void oc_bpt_test_utl_btree_create(
    Oc_wu *wu_p,
    Oc_bpt_test_state *s_p)
{
    oc_bpt_create_b(wu_p, &s_p->bpt_s);

    oc_crt_lock_write(&g_lock);
    {
        oc_bpt_alt_create_b(wu_p, &s_p->alt_s);
    }
    oc_crt_unlock(&g_lock);
}

void oc_bpt_test_utl_btree_display(
    Oc_bpt_test_state *s_p,
    Oc_bpt_test_utl_disp_choice choice)
{
    char tag[10];
    static int cnt =0;

    cnt++;
    sprintf(tag, "Tree_%d", cnt);

    switch (choice)
    {
    case OC_BPT_TEST_UTL_TREE_ONLY:
        oc_bpt_dbg_output_b(&utl_wu, &s_p->bpt_s, tag);
        break;

    case OC_BPT_TEST_UTL_LL_ONLY:
        printf("// Linked-list:\n");
        oc_bpt_alt_dbg_output_b(&utl_wu, &s_p->alt_s);
        break;

    case OC_BPT_TEST_UTL_BOTH:
        oc_bpt_dbg_output_b(&utl_wu, &s_p->bpt_s, tag);
        printf("// Linked-list:\n");
        oc_bpt_alt_dbg_output_b(&utl_wu, &s_p->alt_s);
        break;
    }

    fflush(stdout);
}

bool oc_bpt_test_utl_btree_validate(
    Oc_bpt_test_state *s_p)
{
    bool rc1, rc2;

    rc1 = oc_bpt_dbg_validate_b(&utl_wu, &s_p->bpt_s);
    rc2 = oc_bpt_alt_dbg_validate_b(&utl_wu, &s_p->alt_s);

    if (!rc1)
        printf("    // invalid b-tree\n");

    return rc1 && rc2;
}

// validate an array of clones
bool oc_bpt_test_utl_btree_validate_clones(
    int n_clones,
    struct Oc_bpt_test_state* st_array[])
{
    int i;
    struct Oc_bpt_state *s_array[20];

    if (n_clones > 20)
        ERR(("Can only do up to 20 clones"));

    for (i=0; i<n_clones; i++)
        s_array[i] = &st_array[i]->bpt_s;

    return oc_bpt_dbg_validate_clones_b(&utl_wu, n_clones, s_array);
}

void oc_bpt_test_utl_statistics(Oc_bpt_test_state *s_p)
{
    oc_bpt_statistics_b(&utl_wu, &s_p->bpt_s);
}

void oc_bpt_test_utl_btree_insert(
    Oc_wu* wu_p,
    Oc_bpt_test_state *s_p,
    uint32 key,
    bool *check_eq_pio)
{
    bool rc1, rc2;

    param->total_ops++;

    if (param->verbose) printf("// insert %lu TID=%Lu\n", key, get_tid(s_p));

    rc1 = oc_bpt_insert_key_b(wu_p,
                              &s_p->bpt_s,
                              (struct Oc_bpt_key*) &key,
                              (struct Oc_bpt_data*) &key);
    oc_crt_lock_write(&g_lock);
    {
        rc2 = oc_bpt_alt_insert_key_b(wu_p,
                                      &s_p->alt_s,
                                      (struct Oc_bpt_key*) &key,
                                      (struct Oc_bpt_data*) &key);
    }
    oc_crt_unlock(&g_lock);

    if (*check_eq_pio) {
        if (rc1 != rc2) {
            printf("  // mismatch in insert (%lu)\n", key);
            *check_eq_pio = FALSE;
        }
        if (!validate_fun()) {
            *check_eq_pio = FALSE;
        }
    }

    if (param->verbose && (*check_eq_pio))
        print_fun();
}

void oc_bpt_test_utl_btree_lookup_internal(
    Oc_wu* wu_p,
    Oc_bpt_test_state *s_p,
    uint32 key,
    bool *check_eq_pio)
{
    uint32 data1,data2;
    bool rc1, rc2;

    rc1 = rc2 = FALSE;
    data1 = data2 = 0;

    rc1 = oc_bpt_lookup_key_b(
        wu_p,
        &s_p->bpt_s,
        (struct Oc_bpt_key*) &key,
        (struct Oc_bpt_data*) &data1);

    oc_crt_lock_read(&g_lock);
    {
        rc2 = oc_bpt_alt_lookup_key_b(
            wu_p,
            &s_p->alt_s,
            (struct Oc_bpt_key*) &key,
            (struct Oc_bpt_data*) &data2);
    }
    oc_crt_unlock(&g_lock);

    if (*check_eq_pio) {
        if (rc1 != rc2 ||
            data1 != data2) {
            oc_bpt_test_utl_btree_display(s_p, OC_BPT_TEST_UTL_LL_ONLY);
            printf("  // mismatch in lookup (%lu)\n", key);
            *check_eq_pio = FALSE;
        }
    }
}

void oc_bpt_test_utl_btree_lookup(
    Oc_wu* wu_p,
    Oc_bpt_test_state *s_p,
    uint32 key,
    bool *check_eq_pio)
{
    if (param->verbose) {
        printf("// lookup %lu  TID=%Lu\n", key, get_tid(s_p));
        fflush(stdout);
    }

    oc_bpt_test_utl_btree_lookup_internal(
        wu_p, s_p, key, check_eq_pio);
}

void oc_bpt_test_utl_btree_remove_key(
    Oc_wu *wu_p,
    Oc_bpt_test_state *s_p,
    uint32 key,
    bool *check_eq_pio)
{
    bool rc1, rc2;

    param->total_ops++;

    if (param->verbose) printf("// remove %lu TID=%Lu\n", key, get_tid(s_p));
    rc1 = oc_bpt_remove_key_b(
        wu_p,
        &s_p->bpt_s,
        (struct Oc_bpt_key *)&key);

    oc_crt_lock_write(&g_lock);
    {
        rc2 = oc_bpt_alt_remove_key_b(
            wu_p,
            &s_p->alt_s,
            (struct Oc_bpt_key *)&key);
    }
    oc_crt_unlock(&g_lock);

    if (*check_eq_pio) {
        if (rc1 != rc2) {
            printf("  // mismatch in remove_key(%lu)\n", key);
        }
        if (!validate_fun())
            *check_eq_pio = FALSE;
    }

    // print only if a key has actually been removed
    if (param->verbose && (*check_eq_pio) && rc1)
        print_fun();
}

void oc_bpt_test_utl_btree_delete(
    Oc_wu *wu_p,
    Oc_bpt_test_state *s_p)
{
    if (param->verbose) printf("// btree delete\n");
    oc_bpt_delete_b(wu_p, &s_p->bpt_s);
    oc_crt_lock_write(&g_lock);
    {
        oc_bpt_alt_delete_b(wu_p, &s_p->alt_s);
    }
    oc_crt_unlock(&g_lock);

    if (param->verbose) print_fun();
}

// create a random number between 0 and [top]
uint32 oc_bpt_test_utl_random_number(uint32 top)
{
    return (uint32) (rand() % top);
}


void oc_bpt_test_utl_btree_lookup_range(
    Oc_wu *wu_p,
    Oc_bpt_test_state *s_p,
    uint32 lo_key,
    uint32 hi_key,
    bool *check_eq_pio)
{
    int i,j;
    int nkeys_found1, nkeys_found2;
    uint32 *key_array1, *key_array2, *data_array1, *data_array2;
    int n_keys;

    param->total_ops++;
    if (param->verbose)
        printf("// lookup_range [lo_key=%lu, hi_key=%lu] TID=%Lu\n",
               lo_key, hi_key, get_tid(s_p));

    if (hi_key >= lo_key)
        n_keys = MIN(((hi_key - lo_key) + 1), 10000);
    else
        /* There aren't going to be any returned keys, the value
         * does not matter.
         */
        n_keys = 30;

    // We allocate these arrrays on the stack to get thread-safety
    key_array1 = (uint32*) alloca(n_keys * sizeof(uint32));
    key_array2 = (uint32*) alloca(n_keys * sizeof(uint32));
    data_array1 = (uint32*) alloca(n_keys * sizeof(uint32));
    data_array2 = (uint32*) alloca(n_keys * sizeof(uint32));

    oc_bpt_lookup_range_b(
        wu_p, &s_p->bpt_s,
        (struct Oc_bpt_key*)&lo_key,
        (struct Oc_bpt_key*)&hi_key,
        n_keys,
        (struct Oc_bpt_key*)key_array1,
        (struct Oc_bpt_data*)data_array1,
        &nkeys_found1);

    if (! (*check_eq_pio)) return;

    oc_crt_lock_read(&g_lock);
    {
        oc_bpt_alt_lookup_range_b(
            wu_p, &s_p->alt_s,
            (struct Oc_bpt_key*)&lo_key,
            (struct Oc_bpt_key*)&hi_key,
            n_keys,
            (struct Oc_bpt_key*)key_array2,
            (struct Oc_bpt_data*)data_array2,
            &nkeys_found2);
    }
    oc_crt_unlock(&g_lock);


    if (nkeys_found2 != nkeys_found1)
        goto error;

    for (i=0; i<nkeys_found1; i++)
        if (key_array1[i] != key_array2[i] ||
            data_array1[i] != data_array2[i]) {
            goto error;
        }

    return;

 error:

    for (j=0; j<nkeys_found1; j++)
        printf("  // bpt] (key=%lu data=%lu)\n",
               key_array1[j],
               data_array1[j]);
    for (j=0; j<nkeys_found2; j++)
        printf("  // alt] (key=%lu data=%lu)\n",
               key_array2[j],
               data_array2[j]);
    printf("  // #entries found btree = %d\n", nkeys_found1);
    printf("  // #entries found linked-list = %d\n", nkeys_found2);

    for (i=0; i<nkeys_found1; i++) {
        if (key_array1[i] != key_array2[i]) {
            printf("// The error is in key number %d <KEY %lu != %lu>\n",
                   i, key_array1[i], key_array2[i]);
            break;
        }
        if (data_array1[i] != data_array2[i]) {
            printf("// The error is in key number %d <DATA %lu != %lu>\n",
                   i, data_array1[i], data_array2[i]);
            break;
        }
    }

    *check_eq_pio = FALSE;
}

void oc_bpt_test_utl_btree_insert_range(
    Oc_wu *wu_p,
    Oc_bpt_test_state *s_p,
    uint32 lo_key, uint32 len,
    bool *check_eq_pio)
{
    bool rc1, rc2;
    uint32 key_array[30];
    uint32 data_array[30];
    uint32 i;

    // we handle limited sized arrays
    if (len > 30)
        ERR(("The test current supports up to insert-ranges of length 30"));

    param->total_ops++;

    if (param->verbose) printf("// insert_range key=%lu len=%lu\n", lo_key, len);

    // setup the array
    for (i=0; i<len ; i++) {
        key_array[i] = lo_key+i;
        data_array[i] = lo_key+i;
    }

    rc1 = oc_bpt_insert_range_b(wu_p, &s_p->bpt_s, len,
                                (struct Oc_bpt_key*)key_array,
                                (struct Oc_bpt_data*)data_array);
    oc_crt_lock_write(&g_lock);
    {
        rc2 = oc_bpt_alt_insert_range_b(wu_p, &s_p->alt_s, len,
                                        (struct Oc_bpt_key*)key_array,
                                        (struct Oc_bpt_data*)data_array);
    }
    oc_crt_unlock(&g_lock);

    if (*check_eq_pio) {
        if (!validate_fun())
            *check_eq_pio = FALSE;
        if (rc1 != rc2) {
            printf("  // mismatch in insert_range key=%lu len=%lu\n",
                   lo_key, len);
            *check_eq_pio = FALSE;
        }
    }

    if (param->verbose && (*check_eq_pio))
        print_fun();
}

void oc_bpt_test_utl_btree_remove_range(
    Oc_wu *wu_p,
    Oc_bpt_test_state *s_p,
    uint32 lo_key, uint32 hi_key,
    bool *check_eq_pio)
{
    int rc1, rc2;

    param->total_ops++;
    if (param->verbose)
        printf("// remove_range lo_key=%lu hi_key=%lu\n", lo_key, hi_key);

    rc1 = oc_bpt_remove_range_b(wu_p, &s_p->bpt_s,
                                (struct Oc_bpt_key*)&lo_key,
                                (struct Oc_bpt_key*)&hi_key);
    oc_crt_lock_write(&g_lock);
    {
        rc2 = oc_bpt_alt_remove_range_b(wu_p, &s_p->alt_s,
                                        (struct Oc_bpt_key*)&lo_key,
                                        (struct Oc_bpt_key*)&hi_key);
    }
    oc_crt_unlock(&g_lock);

    if (*check_eq_pio) {
        if (!validate_fun())
            *check_eq_pio = FALSE;
        if (rc1 != rc2) {
            printf("mismatch in remove_range lo_key=%lu hi_key=%lu\n",
                   lo_key, hi_key);
            *check_eq_pio = FALSE;
        }
    }

    if (param->verbose && (*check_eq_pio)) {
        if (0 == rc1) printf("    // no keys removed during remove-range\n");
        if (param->verbose) print_fun();
    }
}

bool oc_bpt_test_utl_btree_compare_and_verify(Oc_bpt_test_state *s_p)
{
    bool rc = TRUE;

    if (param->verbose) {printf("// compare and verify\n"); fflush(stdout);}

#if 0
    int i
    // The old code checked key by key
    for (i=0; i<max_int; i++) {
        oc_bpt_test_utl_btree_lookup_internal(wu_p, s_p, i, &rc);
        if (!rc) return FALSE;
    }
#endif

    oc_bpt_test_utl_btree_lookup_range(&utl_wu, s_p, 0, param->max_int, &rc);
    if (!rc) return FALSE;
    else return TRUE;
}

/**********************************************************************/

// clone a b-tree
void oc_bpt_test_utl_btree_clone(
    struct Oc_wu *wu_p,
    struct Oc_bpt_test_state* src_p,
    struct Oc_bpt_test_state* trg_p
    )
{
    if (param->verbose) printf("// clone new-TID=%Lu\n", get_tid(trg_p));

    oc_bpt_clone_b(wu_p, &src_p->bpt_s, &trg_p->bpt_s);
    oc_crt_lock_write(&g_lock);
    {
        oc_bpt_alt_clone_b(wu_p, &src_p->alt_s, &trg_p->alt_s);
    }
    oc_crt_unlock(&g_lock);

    if (param->verbose) print_fun();
}

static int g_cnt = 0;

// Display the set of clones together in one tree
void oc_bpt_test_utl_display_all(
    int n_clones,
    Oc_bpt_test_state* clone_array[])
{
    struct Oc_bpt_state *bpt_array[100];
    int i;
    char tag[10];

    oc_utl_assert(n_clones <= 100);

    memset(tag, 0, 10);
    g_cnt++;
    snprintf(tag, 10, "X%d", g_cnt);

    // setup an array of b-tree states
    for (i=0; i<n_clones; i++) {
        bpt_array[i] = &clone_array[i]->bpt_s;
    }

    // pass it into the the output_dot_clones function
    oc_bpt_dbg_output_clones_b(&utl_wu,
                               n_clones,
                               bpt_array,
                               tag);
}

/**********************************************************************/

// Verify the free-space has [num_blocks] allocated
void oc_bpt_test_utl_fs_verify(int num_blocks)
{
    oc_bpt_test_fs_verify(num_blocks);
}

Oc_bpt_state *oc_bpt_test_utl_get_state(struct Oc_bpt_test_state *s_p)
{
    return &s_p->bpt_s;
}

/**********************************************************************/
void oc_bpt_test_utl_init(void)
{
    oc_bpt_test_utl_setup_wu(&utl_wu, &utl_rm_ticket);
    if (utl_wu.po_id != 0)
        ERR(("the initial po_id has to be zero"));

    pl_utl_set_hns();
    vd_create();
    oc_bpt_init();
    oc_bpt_test_fs_create("BPT free-space", 10000, FALSE);
    oc_crt_init_rw_lock(&g_lock);

    // setup
    memset(&cfg, 0, sizeof(cfg));
    cfg.key_size = sizeof(Oc_bpt_test_key);
    cfg.data_size = sizeof(Oc_bpt_test_data);
    cfg.node_size = NODE_SIZE;
    cfg.root_fanout = param->max_root_fanout;
    cfg.non_root_fanout = param->max_non_root_fanout;
    cfg.min_num_ent = param->min_fanout;
    cfg.node_alloc = node_alloc;
    cfg.node_dealloc = node_dealloc;
    cfg.node_get_sl = node_get_sl;
    cfg.node_get_xl = node_get_xl;
    cfg.fs_inc_refcount = oc_bpt_test_fs_inc_refcount;
    cfg.fs_get_refcount = oc_bpt_test_fs_get_refcount;
    cfg.node_release = node_release;
    cfg.node_mark_dirty = node_mark_dirty;
    cfg.key_compare = key_compare;
    cfg.key_inc = key_inc;
    cfg.key_to_string = key_to_string;
    cfg.data_release = data_release;
    cfg.data_to_string = data_to_string;

    memset(&alt_cfg, 0, sizeof(alt_cfg));
    alt_cfg.key_size = sizeof(Oc_bpt_test_key);
    alt_cfg.data_size = sizeof(Oc_bpt_test_data);
    alt_cfg.key_compare = key_compare;
    alt_cfg.key_inc = key_inc;
    alt_cfg.key_to_string = key_to_string;
    alt_cfg.data_release = data_release;
    alt_cfg.data_to_string = data_to_string;

    oc_bpt_init_config(&cfg);
}

void oc_bpt_test_utl_set_print_fun(void (*print_fun_i)(void))
{
    print_fun = print_fun_i;
}

void oc_bpt_test_utl_set_validate_fun(bool (*validate_fun_i)(void))
{
    validate_fun = validate_fun_i;
}


bool oc_bpt_test_utl_parse_cmd_line(int argc, char *argv[])
{
    int i = 1;

    for (i=1;i<argc;i++) {
        if (strcmp(argv[i], "-trace") == 0) {
	    if (++i >= argc)
                return FALSE;
            pl_trace_base_add_string_tag_full(argv[i]);
        }
        else if (strcmp(argv[i], "-trace_level") == 0) {
	    if (++i >= argc)
                return FALSE;
            pl_trace_base_set_level(atoi(argv[i]));
        }
        else if (strcmp(argv[i], "-trace_tags") == 0) {
            pl_trace_base_print_tag_list();
            exit(0);
        }
        else if (strcmp(argv[i], "-verbose") == 0) {
            param->verbose = TRUE;
        }
        else if (strcmp(argv[i], "-stat") == 0) {
            param->statistics = TRUE;
        }
        else if (strcmp(argv[i], "-max_int") == 0) {
	    if (++i >= argc)
                return FALSE;
            param->max_int = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-num_rounds") == 0) {
	    if (++i >= argc)
                return FALSE;
            param->num_rounds = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-num_tasks") == 0) {
	    if (++i >= argc)
                return FALSE;
            param->num_tasks = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-num_ops_per_task") == 0) {
	    if (++i >= argc)
                return FALSE;
            param->num_ops_per_task = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-max_num_clones") == 0) {
	    if (++i >= argc)
                return FALSE;
            param->max_num_clones = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-max_root_fanout") == 0) {
	    if (++i >= argc)
                return FALSE;
            param->max_root_fanout = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-max_non_root_fanout") == 0) {
	    if (++i >= argc)
                return FALSE;
            param->max_non_root_fanout = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-min_fanout") == 0) {
	    if (++i >= argc)
                return FALSE;
            param->min_fanout = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "-test") == 0) {
	    if (++i >= argc)
                return FALSE;
            if (strcmp(argv[i], "large_trees") == 0)
                test_type = OC_BPT_TEST_UTL_LARGE_TREES;
            else if (strcmp(argv[i], "small_trees") == 0)
                test_type = OC_BPT_TEST_UTL_SMALL_TREES;
            else if (strcmp(argv[i], "small_trees_w_ranges") == 0)
                test_type = OC_BPT_TEST_UTL_SMALL_TREES_W_RANGES;
            else if (strcmp(argv[i], "small_trees_mixed") == 0)
                test_type = OC_BPT_TEST_UTL_SMALL_TREES_MIXED;
            else
                ERR(("no such test. valid tests={large_trees,small_trees,small_trees_w_ranges,small_trees_mixed}"));
        }
        else
            return FALSE;
    }

    return TRUE;
}

Oc_bpt_test_param *oc_bpt_test_utl_get_param()
{
    return param;
}


void oc_bpt_test_utl_help_msg (void)
{
    printf("Usage: oc_bpt_test\n");
    printf("\t -max_int <range of keys is [0 ... max_int]>  [default=%d]\n",
           param->max_int);
    printf("\t -num_rounds <number of rounds to execute>  [default=%d]\n",
           param->num_rounds);
    printf("\t -num_tasks <number of concurrent tasks>  [default=%d]\n",
           param->num_tasks);
    printf("\t -num_ops_per_task  <for multi-task-clone test: ops per task iteration> [default=10]\n");
    printf("\t -max_num_clones <maximal number of b-tree clones> [default=%d]\n",
           param->max_num_clones);
    printf("\t -max_root_fanout <maximal number of entries in a root node>  [default=%d]\n",
           param->max_root_fanout);
    printf("\t -max_non_root_fanout <maximal number of entries in a non-root node>  [default=%d]\n",
           param->max_non_root_fanout);
    printf("\t -min_fanout <minimal number of entries in a node>  [default=%d]\n",
           param->min_fanout);
    printf("\t -verbose\n");
    printf("\t -stat\n");
    printf("\t -test <small_trees|large_trees|small_trees_w_ranges|small_trees_mixed>\n");
    exit(1);
}


/**********************************************************************/
