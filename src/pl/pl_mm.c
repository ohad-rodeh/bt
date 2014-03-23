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
/***************************************************************************/
#include <pthread.h>
#include <sys/mman.h>
#include <string.h>
#include <malloc.h>

#include "pl_base.h"
#include "pl_base.h"
#include "pl_dstru.h"
#include "pl_trace.h"
#include "pl_mm_int.h"

/***************************************************************************/
static int align (int size, Pl_mm_alignment_t align);

/***************************************************************************/

void* pl_mm_malloc (int size)
{
    void *ptr;
    ptr = malloc(size);
    if (NULL == ptr) 
       ERR(("out of memory"));

    return ptr;
}

void pl_mm_free(void *ptr)
{
    free(ptr);
}

/***************************************************************************/

void pl_mm_init(void)
{
}


/***************************************************************************/
// object-pools

typedef struct Pl_mm_op_s {
    int size;   // object size
    int number; // num of objects in pool
    Ss_slist free_list;   /* A pointer to the head of the free list */
    void** all_objects_pp;
} Pl_mm_op_s;

/* Align [size] by [align].
 * 
 * For example,
 *   Aligning 7 by align=2 should result in 8.
 *   Aligning 13 by align=4 should result in 16.
 *
*/
static int align (int size, Pl_mm_alignment_t align)
{
    if (1 == align) return size;
    
    return ((size+align)/align) * align;
}


/*
  [Allon 11/1/06] added a member called all_objects_pp which saves
  addresses of all allocated objects so they can be released on pool delete.
  memory overhead could be reduced if we allocate many objects in one malloc.
  (I didn't want to allocated them all at once because it may be a very
  large malloc)
 */
bool pl_mm_pool_create( uint32 size_i,
                        Pl_mm_alignment_t align_i,
                        uint32 number_i,
                        void (*init_object_i)( void *object_i),
                        Pl_mm_op **pool_o )
{
    Pl_mm_op *new_pool ;
    uint32 i;
    char *obj;
    int aligned_size;
    
    if (size_i < sizeof(int))
        ERR(("can't create a memory pool for objects smaller than an integer"));
        
    new_pool = (Pl_mm_op*) pl_mm_malloc(sizeof(Pl_mm_op_s));

    memset(new_pool, 0, sizeof(Pl_mm_op_s));
    new_pool->size = size_i;
    new_pool->number = number_i;
    new_pool->all_objects_pp = 
        (void**) pl_mm_malloc( sizeof(void*) * number_i );
    memset( new_pool->all_objects_pp, 0, sizeof(void*) * number_i );
    *pool_o = new_pool;
    ssslist_init(&new_pool->free_list);

    aligned_size = align(size_i, align_i);
    for (i=0; i<number_i ; i++) {
	obj = (char*) pl_mm_malloc (aligned_size);
	memset(obj, 0, aligned_size);
        new_pool->all_objects_pp[i] = (void*)obj;

	if (NULL != init_object_i) 
	    (*init_object_i)(obj);
	ssslist_add_tail(&new_pool->free_list, (Ss_slist_node*)obj);
    }
    return TRUE;
}


// destroy all objects in pool and release pool resources
void pl_mm_pool_delete( Pl_mm_op *pool_p,
                        void (*destroy_object_fun)( void *object_fun ) )
{
    int i;
    char *obj;
    
    assert(pool_p);
    assert(pool_p->all_objects_pp);
    for (i=0;i<pool_p->number;i++) {
        obj = (char*)pool_p->all_objects_pp[i];

        assert(obj);

        if (destroy_object_fun) {
            destroy_object_fun(obj);
        }
        
        pl_mm_free(obj);
        pool_p->all_objects_pp[i]=NULL;
    }

    pl_mm_free(pool_p->all_objects_pp);
    pl_mm_free(pool_p);
}

bool pl_mm_pool_alloc( Pl_mm_op *pool_i,
                       void **object_o )
{
    bool rc = FALSE;
    
    if (ssslist_get_length(&pool_i->free_list) > 0) {
	*object_o = (void*) ssslist_remove_head(&pool_i->free_list);
	rc = TRUE;
    } else {
	*object_o = NULL;
        rc = FALSE;
    }
    return rc;
}

void pl_mm_pool_free( Pl_mm_op *pool_i, void *object_i )
{
    ssslist_add_head(&pool_i->free_list,
                     (Ss_slist_node*) object_i);
}

/***************************************************************************/

