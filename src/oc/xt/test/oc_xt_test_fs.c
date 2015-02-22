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
/* OC_XT_TEST_FS.H
 *
 * A free-space implementation that helps tests the correct functions
 * of the x-tree.
 */
/**********************************************************************/
#include <malloc.h>
#include <string.h>

#include "oc_xt_test_fs.h"
#include "oc_utl.h"

/* Free-space functions used to verify that free-space handling is
 * working correctly
 */

struct Oc_xt_test_fs_ctx {
    // the length of [fs_array]
    int len;

    // An array of bytes, a single byte for each allocation unit
    char *fs_array;

    // how many units have been allocated
    int tot_alloc;

    char desc[30];

    // verbose mode? 
    bool verbose;
};
/**********************************************************************/
#define INIT_LEN (10000)

struct Oc_xt_test_fs_ctx *oc_xt_test_fs_create(
    char *str_desc_p,
    bool verbose)
{
    struct Oc_xt_test_fs_ctx *ctx_p;
    
    ctx_p = (struct Oc_xt_test_fs_ctx *) malloc(sizeof(struct Oc_xt_test_fs_ctx));
    oc_utl_assert(ctx_p);
    memset(ctx_p, 0, sizeof(struct Oc_xt_test_fs_ctx));
    ctx_p->len = INIT_LEN;
    ctx_p->fs_array = (char*) malloc(INIT_LEN);
    memset(ctx_p->fs_array, 0, INIT_LEN);
    memset(ctx_p->desc, 0, 30);
    strncpy(ctx_p->desc, str_desc_p, 30);
    ctx_p->verbose = verbose;
    
    return ctx_p;
}

// allocate an extent of [len] units in free-space [ctx_p]
uint32 oc_xt_test_fs_alloc(struct Oc_xt_test_fs_ctx *ctx_p,
                           uint32 len)
{
    int i,j;
    uint32 ofs;

    if (ctx_p->verbose) {
        printf("// alloc [%s] (len=%lu)\n", ctx_p->desc, len);
        fflush(stdout);
    }
    
    // look for an existing contiguous extent of size [len]
    for (i=0, j=0, ofs=0; i<ctx_p->len; i++) {
        if (!ctx_p->fs_array[i]) {
            if (0 == j) {
                ofs = i;
                j++;
            }
            else {
                j++;
            }
            
            if (len == j) {
                // found a contiguous unallocated area, mark it
                // as allocated
                int k;
                
                for (k=ofs; k<ofs+len; k++)
                    ctx_p->fs_array[k] = TRUE;
                goto done;
            }
        }
        else {
            // zero out the count
            j=0;
            ofs = 0;
        }
    }

    // if not found, double the size of [fs_array]
    printf("// resize FS, now=%u\n", ctx_p->len *2);
    ctx_p->fs_array = (char*) realloc(ctx_p->fs_array, ctx_p->len *2);
    oc_utl_assert(ctx_p);
    memset(&ctx_p->fs_array[ctx_p->len], 0, ctx_p->len);

    // allocate in the new area 
    ofs = ctx_p->len+1;
    for (i=ctx_p->len+1; i<=ctx_p->len + len; i++)
        ctx_p->fs_array[i] = TRUE;

    ctx_p->len = 2 * ctx_p->len;

done:
    ctx_p->tot_alloc += len;
    return ofs;
}

// deallocate an extent of [len] units in free-space [ctx_p]
void oc_xt_test_fs_dealloc(struct Oc_xt_test_fs_ctx *ctx_p,
                           uint32 ofs,
                           uint32 len)
{
    uint32 i;

    if (ctx_p->verbose) {
        printf("// dealloc [%s] (ofs=%lu, len=%lu)\n", ctx_p->desc, ofs, len);
        fflush(stdout);
    }
    
    // set to FALSE the state of all the units in the range
    for (i=ofs; i<ofs+len; i++) {
        oc_utl_assert(ctx_p->fs_array[i]);
        ctx_p->fs_array[i] = FALSE;
    }

    ctx_p->tot_alloc -= len;
    oc_utl_assert(ctx_p->tot_alloc >= 0);
}


// return TRUE if [ctx_p] is unallocated starting at offset [len]
static bool make_sure_is_unallocated(
    struct Oc_xt_test_fs_ctx *ctx_p,
    uint32 len)
{
    int i;
    
    for (i=len; i<ctx_p->len; i++)
        if (ctx_p->fs_array[i])
            return FALSE;

    return TRUE;
}
    
/* compare two free-space instances. Return TRUE if they are equal, FALSE
 * otherwise.
 */
bool oc_xt_test_fs_compare(struct Oc_xt_test_fs_ctx *ctx1_p,
                           struct Oc_xt_test_fs_ctx *ctx2_p)
{
    int i;
    
    if (ctx1_p->verbose) {
        printf("// comparing FS implementations, [%s]=%d [%s]=%d\n",
               ctx1_p->desc, 
               ctx1_p->tot_alloc,
               ctx2_p->desc,           
               ctx2_p->tot_alloc);
        fflush(stdout);
    }
    if (ctx1_p->tot_alloc != ctx2_p->tot_alloc)
        return FALSE;
        
    // the two free-space instance may have different length.
    // we are only interested in comparing the allocated areas.
    for (i=0; i<ctx1_p->len; i++) {
        if (ctx1_p->fs_array[i] != ctx2_p->fs_array[i])
            return FALSE;
    }

    // account for differences in length between the two instances
    if (ctx1_p->len == ctx2_p->len)
        return TRUE;
    if (ctx1_p->len < ctx2_p->len)
        return make_sure_is_unallocated(ctx2_p, ctx1_p->len);
    else
        return make_sure_is_unallocated(ctx1_p, ctx2_p->len);
}


/**********************************************************************/
