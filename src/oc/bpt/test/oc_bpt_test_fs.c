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
#include <malloc.h>
#include <string.h>

#include "oc_bpt_test_fs.h"
#include "oc_utl.h"

/**********************************************************************/

/* Free-space functions used to verify that free-space handling is
 * working correctly
 */

struct Oc_bpt_test_fs_ctx {
    // the length of [fs_array]
    int num_blocks;

    // An array of bytes, a single byte for each allocation unit
    char *fs_array;

    // how many units have been allocated
    int tot_alloc;

    char desc[30];

    // verbose mode?
    bool verbose;
};

static struct Oc_bpt_test_fs_ctx *ctx_p = NULL;
static struct Oc_bpt_test_fs_ctx *alt_p = NULL;

static struct Oc_bpt_test_fs_ctx* fs_create(
    char *str_desc_p,
    int num_blocks,
    bool verbose);
static void fs_free(struct Oc_bpt_test_fs_ctx *fs_p);

/**********************************************************************/
static struct Oc_bpt_test_fs_ctx *fs_create(
    char *str_desc_p,
    int num_blocks,
    bool verbose)
{
    struct Oc_bpt_test_fs_ctx *fs_p;

    fs_p = (struct Oc_bpt_test_fs_ctx *)
        pl_mm_malloc(sizeof(struct Oc_bpt_test_fs_ctx));

    memset(fs_p, 0, sizeof(struct Oc_bpt_test_fs_ctx));
    fs_p->num_blocks = num_blocks;
    fs_p->fs_array = (char*) pl_mm_malloc(num_blocks);
    memset(fs_p->fs_array, 0, num_blocks);
    memset(fs_p->desc, 0, 30);
    strncpy(fs_p->desc, str_desc_p, 30);
    fs_p->verbose = verbose;

    // don't allocate the first block
    fs_p->fs_array[0] = 1;

    return fs_p;
}

static void fs_free(struct Oc_bpt_test_fs_ctx *fs_p)
{
    pl_mm_free(fs_p->fs_array);

    // extra sanitation
    memset(fs_p, 0, sizeof(struct Oc_bpt_test_fs_ctx));

    pl_mm_free(fs_p);
}

/**********************************************************************/

void oc_bpt_test_fs_create(
    char *str_desc_p,
    int num_blocks,
    bool verbose)
{
    ctx_p = fs_create(str_desc_p, num_blocks, verbose);
}

// allocate a block
uint64 oc_bpt_test_fs_alloc(void)
{
    int i, loc;

    oc_utl_assert(ctx_p);

    if (ctx_p->verbose) {
        printf("// alloc [%s]\n", ctx_p->desc);
        fflush(stdout);
    }

    /* look for a free block. Start searching from the beginning
     * so as to flush out any error in freeing blocks too early.
     */
    for (i=0, loc=-1 ; i<ctx_p->num_blocks; i++) {
        if (!ctx_p->fs_array[i]) {
            loc = i;
            break;
        }
    }

    if (loc != -1) {
        // found a free block
        ctx_p->tot_alloc++;
        ctx_p->fs_array[loc] = 1;
        return (uint64)loc;
    }
    else {
        ERR(("Free space has run out of pages, it curently has %d."
             "Either run a smaller test or increase the size of the FS.",
             ctx_p->num_blocks));
    }
}

// deallocate a block
void oc_bpt_test_fs_dealloc(uint64 loc)
{
    oc_utl_assert(ctx_p);
    oc_utl_assert(0 <= loc && loc < ctx_p->num_blocks);

    if (ctx_p->verbose) {
        printf("// dealloc [%s] (loc=%Lu)\n", ctx_p->desc, loc);
        fflush(stdout);
    }

    oc_utl_assert(ctx_p->fs_array[loc] > 0);
    ctx_p->fs_array[loc]--;

    if (0 == ctx_p->fs_array[loc])
        ctx_p->tot_alloc--;

    oc_utl_assert(ctx_p->tot_alloc >= 0);
}


void oc_bpt_test_fs_inc_refcount(struct Oc_wu *wu_p, uint64 addr)
{
    oc_utl_assert(ctx_p);

    oc_utl_assert(ctx_p->fs_array[addr] > 0);
    ctx_p->fs_array[addr]++;
}

int oc_bpt_test_fs_get_refcount(struct Oc_wu *wu_p, uint64 addr)
{
    oc_utl_assert(ctx_p);

    return (int)ctx_p->fs_array[addr];
}

/**********************************************************************/

void oc_bpt_test_fs_verify(int num_blocks)
{
    oc_utl_debugassert(ctx_p);
    oc_utl_assert(num_blocks == ctx_p->tot_alloc);
}


/**********************************************************************/

// Initialize the alternate block-map
void oc_bpt_test_fs_alt_create(void)
{
    oc_utl_assert(ctx_p);
    oc_utl_assert(NULL == alt_p);
    alt_p = fs_create("alternate", ctx_p->num_blocks, FALSE);
}

// Increase the ref-count for one of the blocks
void oc_bpt_test_fs_alt_inc_refcount(struct Oc_wu *wu_p, uint64 addr)
{
    oc_utl_assert(0 <= addr && addr < alt_p->num_blocks);
    alt_p->fs_array[addr]++;
}

// compare the alternate block-map
bool oc_bpt_test_fs_alt_compare(int num_clones)
{
    int i;
    bool flag = TRUE;

    for (i=0; i<alt_p->num_blocks; i++)
    {
        bool err = FALSE;

        if ((alt_p->fs_array[i] == 0 && ctx_p->fs_array[i] != 0) ||
            (alt_p->fs_array[i] != 0 && ctx_p->fs_array[i] == 0))
            err = TRUE;

        if (alt_p->fs_array[i] < ctx_p->fs_array[i])
            err = TRUE;

        if (num_clones < ctx_p->fs_array[i])
            err = TRUE;

        if (err)
        {
            printf("There is a discrepency in block %d\n", i);
            printf("The values are   alt=%d  fs=%d\n",
                   alt_p->fs_array[i],
                   ctx_p->fs_array[i]);
            flag = FALSE;
        }
    }

    return flag;
}

void oc_bpt_test_fs_alt_free(void)
{
    fs_free(alt_p);
    alt_p = NULL;
}

/**********************************************************************/
