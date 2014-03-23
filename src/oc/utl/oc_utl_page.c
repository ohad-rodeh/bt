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
/* Code for handling the global page pool
 */
/******************************************************************/
#include <memory.h>

#include "pl_int.h"
#include "oc_utl.h"
#include "oc_crt_int.h"
#include "oc_utl_page.h"
#include "oc_utl_trace.h"

/******************************************************************/
typedef struct Oc_utl_page_ctx {
    // a pool of pages.     
    Pl_mm_op      *page_pool;         

    // for waiting until a page is available
    Oc_crt_atom_cnt acnt;            

    // The number of free pages.
    int           num_free_pages;    

    // The maximal number of free pages available    
    int           max_num_free_pages;  
} Oc_utl_page_ctx;

static Oc_utl_page_ctx ctx;

/******************************************************************/

static void init_page (void *_pg_p);
static void destroy_page (void *_pg_p);
static void init_page_w_data (Oc_utl_page *pg_p);
static Oc_utl_page *oc_utl_page_alloc_core(Oc_wu *wu_p);

/******************************************************************/

void oc_utl_page_init(int num_pages)
{
    bool rc;

    // create the page-pool
    rc = pl_mm_pool_create( sizeof(Oc_utl_page),
                                         PL_MM_ALIGN_DEFAULT,
                                         num_pages,
                                         init_page,
                                         &ctx.page_pool );
    if (FALSE == rc || NULL == ctx.page_pool)
        ERR(("Could not create the page-pool"));
    
    ctx.num_free_pages = num_pages;

    oc_crt_atom_cnt_init(&ctx.acnt, ctx.num_free_pages);
}

void oc_utl_page_free_resources(void)
{
    pl_mm_pool_delete( ctx.page_pool , destroy_page);
    memset(&ctx, 0, sizeof(ctx));
}

/******************************************************************/

static void init_page_w_data (Oc_utl_page *pg_p)
{
    char *buf_p;
    struct Pl_utl_iobuf *iobuf_p;

    // save the buffer pointer
    buf_p = pg_p->buf_p;
    iobuf_p = pg_p->iobuf_p;

    // cleanup the page structure
    memset(pg_p, 0, sizeof(Oc_utl_page));
    oc_crt_init_wait_q(&pg_p->wait_q);
    pg_p->flags.invalid = TRUE;
    
    // return the buffer pointer
    pg_p->buf_p = buf_p;
    pg_p->iobuf_p = iobuf_p;

    oc_utl_debugassert(NULL != pg_p->buf_p);
}

static void init_page (void *_pg_p)
{
    Oc_utl_page *pg_p = (Oc_utl_page *)_pg_p;

    pl_utl_cfg.funs.iobuf_alloc_f(
        SS_PAGE_SIZE,
        &pg_p->iobuf_p,
        &pg_p->buf_p);
    init_page_w_data(pg_p);
}

static void destroy_page (void *_pg_p)
{
    Oc_utl_page *pg_p = (Oc_utl_page *)_pg_p;
    
    pl_utl_cfg.funs.iobuf_free_f(pg_p->iobuf_p);
}

// page stuff

static Oc_utl_page *oc_utl_page_alloc_core(Oc_wu *wu_p)
{
    bool rc;
    Oc_utl_page *pg_p;

    // There are pages currently
    oc_utl_assert (ctx.num_free_pages > 0);
    ctx.num_free_pages--;
    rc = pl_mm_pool_alloc(ctx.page_pool,
                                                (void**)&pg_p);
    oc_utl_debugassert(rc && pg_p && pg_p->buf_p);
    oc_utl_debugassert(0 == pg_p->ref_count);
    pg_p->ref_count++;
    pg_p->flags.invalid = FALSE;

    oc_utl_trace_wu_lvl(4, OC_EV_UTL_PG_ALLOC_CORE,
                        wu_p,
                        "#free=%d pg=%p buf_p=%p", 
                        ctx.num_free_pages, pg_p , pg_p->buf_p);

    return pg_p;
}

// Allocate a page from the page-pool 
Oc_utl_page *oc_utl_page_alloc_b(Oc_wu *wu_p)
{
    oc_utl_trace_wu_lvl(4, OC_EV_UTL_PG_ALLOC_B,
                        wu_p,
                        "#free=%d", ctx.num_free_pages);

    // Safe-guard the set of free-pages
    oc_crt_atom_cnt_dec(&ctx.acnt, 1);

    return oc_utl_page_alloc_core(wu_p);
}

Oc_utl_page *oc_utl_page_alloc_strict(Oc_wu *wu_p)
{
    // make sure there is a free page before actually allocating it. 
    oc_utl_assert(ctx.num_free_pages > 0);
    
    return oc_utl_page_alloc_b(wu_p);
}


// Increment the ref-count for the page
void oc_utl_page_inc(Oc_wu *wu_p, Oc_utl_page *pg_p)
{
    oc_utl_debugassert(pg_p->ref_count > 0);
    pg_p->ref_count++;

    oc_utl_trace_wu_lvl(4, OC_EV_UTL_PG_INC, wu_p, "#addr=%Lu pg=%p",
                        pg_p->pos.addr_i, pg_p);
}

/* Decrement the ref-count for the page.
 * If the ref-count reaches zero then the page
 * is returned to the page-pool.
 */
void oc_utl_page_dec(Oc_wu *wu_p, Oc_utl_page *pg_p)
{
    oc_utl_debugassert(pg_p->ref_count > 0);
    pg_p->ref_count--;


    oc_utl_trace_wu_lvl(4, OC_EV_UTL_PG_DEC, wu_p,
                        "#addr=%Lu pg=%p ref=%d buf_p=%p",
                        pg_p->pos.addr_i, pg_p, pg_p->ref_count, pg_p->buf_p);
    
   
    if (0 == pg_p->ref_count) {
        /* extra sanitation, erase previous contents of the page.
         * don't forget the data pointer.
         */
        init_page_w_data(pg_p);
        
        ctx.num_free_pages++;

        oc_utl_trace_wu_lvl(4, OC_EV_UTL_PG_FREE, wu_p,
                            "#free=%d pg=%p buf_p=%p",
                            ctx.num_free_pages, pg_p, pg_p->buf_p);
        
        oc_utl_debugassert(NULL != pg_p->buf_p);
 
        pl_mm_pool_free(ctx.page_pool, pg_p);

        oc_crt_atom_cnt_inc(&ctx.acnt, 1);
    }
}


/******************************************************************/

#define CASE(s) case s: return #s ; break

const char *oc_utl_page_string_of_state(Oc_utl_page_state state)
{
    switch (state)
    {
        CASE(OC_UTL_PAGE_NORMAL);
        CASE(OC_UTL_PAGE_IN_READ);
        CASE(OC_UTL_PAGE_IN_WRITE);

    default:
        ERR(("unknown state"));
    }
}

#undef CASE

/******************************************************************/

int oc_utl_page_num_free(void)
{
    return ctx.num_free_pages;
}

/******************************************************************/
