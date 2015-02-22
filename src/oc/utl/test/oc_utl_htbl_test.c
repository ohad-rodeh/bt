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
/* 
 * A self test for the hashtable module
 */
#include <memory.h>
#include <stdlib.h>

#include "pl_int.h"
#include "oc_utl_htbl.h"
#include "oc_utl.h"

/********************************************************************/
/* An (int,string) tuple in a hashtable.
 */
#define DATA_SIZE 10

typedef struct {
    Ss_slist_node link;
    int key;
    char data[DATA_SIZE];
} Elem ;

#define NUM_ELEM (1000)

static Pl_mm_op *elem_pool_p = NULL;

// A linked-list implementation of a hash-table
typedef struct Oc_list {
    Ss_slist list;
    Oc_utl_htbl_compare comp;
} Oc_list;
/********************************************************************/

static void test_init(void);
static Elem *elem_alloc(void);
//static void elem_free(Elem *elem_p);

static uint32 int_hash(void *_key, int num_buckets, int dummy);
static bool int_compare(void *_elem, void *_key);
//static void elem_print(void *_elem);
//static bool discard_fun(void *_elem, void *_data);

static int generate_and_insert_elements(Oc_list *l_i, Oc_utl_htbl *h_i);
static void random_ops( Oc_list *l_i, Oc_utl_htbl *h_i,
                        Ss_slist *free_list1, Ss_slist *free_list2 );
static void add_element_to_both_impl( Oc_list *l_i, Oc_utl_htbl *h_i, int key_i);
static void test_ssslists(void);

/********************************************************************/

static uint32 int_hash(void *_key, int num_buckets, int dummy)
{
    int key = (int) _key;
    return key%num_buckets;
}

static bool int_compare(void *_elem, void *_key)
{
    Elem *elem = (Elem*)_elem;
    int key = (int)_key;

    if (elem->key == key) return TRUE;
    else return FALSE;
}

/*
static void elem_print(void *_elem)
{
    Elem *elem = (Elem*) _elem;
    
    printf("key=%d data=%s\n", elem->key, elem->data);
}

static bool discard_fun(void *_elem, void *_data)
{
    Elem *elem = (Elem*) _elem;
    int key = (int) _data;

    printf("discard function called (key=%d)\n", elem->key);
    if (elem->key < key) return TRUE;
    else return FALSE;
}
*/

/********************************************************************/

void list_create(Oc_list *l_i, Oc_utl_htbl_compare comp)
{
    ssslist_init(&l_i->list);
    l_i->comp = comp;
}

void* list_lookup(Oc_list *l_i, void *key_i)
{
    Elem *elem_p;
    
    for( elem_p = ssslist_head(&l_i->list);
         elem_p != NULL ;
         elem_p= (Elem *) elem_p->link.next )
        if (TRUE == (l_i->comp)(elem_p, key_i)) return elem_p;

    return NULL;
}

bool list_insert(Oc_list *l_i, void *key_i, void* _elem)
{
    if (list_lookup(l_i, key_i) != NULL) return FALSE;

    ssslist_add_tail(&l_i->list, (Ss_slist_node*)_elem);
    return FALSE;
}

bool list_remove(Oc_list *l_i, void *key_i)
{
    Elem *elem_p;

    elem_p = list_lookup(l_i, key_i);
    if (NULL == elem_p) return FALSE;

    ssslist_remove_node(&l_i->list, (Ss_slist_node*) elem_p);
    return TRUE;
}

void* list_extract(Oc_list *l_i, void *key_i)
{
    Elem *elem_p;

    elem_p = list_lookup(l_i, key_i);
    if (NULL == elem_p) return NULL;

    ssslist_remove_node(&l_i->list, (Ss_slist_node*) elem_p);
    return elem_p;
}

/********************************************************************/
static void test_init(void)
{
    if (!pl_mm_pool_create(sizeof(Elem),
                                        PL_MM_ALIGN_32BITS,
                                        NUM_ELEM,
                                        NULL,
                                        &elem_pool_p)) {
        ERR(("failed to create element pool"));
    }
}

static Elem *elem_alloc(void)
{
    Elem *elem_p;
    
    if (!pl_mm_pool_alloc(elem_pool_p, (void**) &elem_p)) {
        return NULL;
    } else {
        memset(elem_p,0, sizeof(Elem));
        return elem_p;
    }
}
/*
static void elem_free(Elem * elem_p)
{
    oc_utl_debugassert(elem_p);
    pl_mm_pool_free(elem_pool_p, elem_p );    
}
*/

// generate an element, the [data] -has- to be null terminated.
static Elem *elem_generate( int key, char *data )
{
    Elem *elem_p = elem_alloc();

    oc_utl_assert(elem_p);
    elem_p->key = key;
    memset(elem_p->data, 0, sizeof(DATA_SIZE));
    strncpy(elem_p->data, data, DATA_SIZE-1);

    return elem_p;
}

/********************************************************************/

static void add_element_to_both_impl( Oc_list *l_i, Oc_utl_htbl *h_i, int key_i)
{
    Elem *elem1_p, *elem2_p;
    char data[DATA_SIZE];

    snprintf(data, DATA_SIZE, "E(%d)", key_i);
    
    elem1_p = elem_generate(key_i, data);
    elem2_p = elem_generate(key_i, data);
    
//    PRN(("insert(%d)", key_i));
    list_insert(l_i, (void*) key_i, (void*) elem1_p);
    oc_utl_htbl_insert(h_i, (void*) key_i, (void*) elem2_p);
    
}

static int generate_and_insert_elements(Oc_list *l_i, Oc_utl_htbl *h_i)
{
    int i;
    
    for (i=0; i<NUM_ELEM/2; i++)
        add_element_to_both_impl( l_i, h_i, i );

    return NUM_ELEM/2;
}


static void random_ops(Oc_list *l_i, Oc_utl_htbl *h_i,
                       Ss_slist *free_list1, Ss_slist *free_list2  )
{
    int i;
    Elem *elem1_p, *elem2_p;
    int key;
//    bool rc1, rc2;

    for (i=0; i<10000; i++){
        switch (rand () % 3) {
        case 0:
            // Insert
            elem1_p = (Elem*) ssslist_remove_head(free_list1);
            elem2_p = (Elem*) ssslist_remove_head(free_list2);

            if (elem1_p != NULL) {
                oc_utl_assert(elem2_p != NULL);
                oc_utl_assert(elem1_p->key == elem2_p->key);
                
                printf("insert(%d)\n", elem1_p->key);
                list_insert(l_i,  (void*) elem1_p->key, elem1_p);
                oc_utl_htbl_insert(h_i,  (void*) elem2_p->key, elem2_p);
            }
            break;
        case 1:
            // lookup
            key = rand() % (NUM_ELEM/2);
            printf("lookup(%d)\n", key);

            elem1_p = list_lookup(l_i, (void*)key);
            elem2_p = oc_utl_htbl_lookup(h_i, (void*)key);
            if ( NULL == elem1_p && NULL == elem2_p )
                break;
            if (NULL == elem1_p
                || NULL == elem2_p
                || elem1_p->key != elem2_p->key)
                ERR(("lookup did not match"));
            break;
        case 2:
            // extract
            key = rand() % (NUM_ELEM/2);
            printf("extract(%d)\n", key);

            elem1_p = (Elem*) list_extract(l_i, (void*)key);
            elem2_p = (Elem*) oc_utl_htbl_extract(h_i, (void*)key);
            if ( NULL == elem1_p && NULL == elem2_p ) {
                printf("extract return NULL\n");
                break;
            }
            if (NULL == elem1_p) 
                ERR(("extract codes did not match"));
            if ( NULL == elem2_p )
                ERR(("extract codes did not match"));
            if ( elem1_p->key != elem2_p->key ) {
                ERR(("extract codes did not match"));
            }
            ssslist_add_tail(free_list1, (Ss_slist_node*) elem1_p);
            ssslist_add_tail(free_list2, (Ss_slist_node*) elem2_p);
            printf("adding to free lists, len=%lu, len=%lu\n",
                   ssslist_get_length(free_list1),
                   ssslist_get_length(free_list2)
                );
            break;

//      case 2:
            // remove
//          key = rand() % max_elem;
//          PRN(("remove(%d)", key));

//          rc1 = list_remove(l_i, (void*) key);
//          rc2 = oc_utl_htbl_remove(h_i, (void*) key);
//          if (rc1 != rc2)
//              ERR(("remove codes did not match"));
//          if (rc1) 
//              total_num_elem--;
//          break;
        default:
            ERR(("sanity"));
        }
    }
}

static void print_list(const char *s, Ss_slist *l_p)
{
    Ss_slist_node *node_p;
    Elem *e_p;
    
    printf("list %s=", s);
    for (node_p = ssslist_head(l_p);
         node_p != NULL;
         node_p = ssslist_next(node_p))
    {
        e_p = (Elem*) node_p;
        printf("<%d>", e_p->key);
    }
    
    printf ("\n");
}

static void test_ssslists(void)
{
    Ss_slist l1, l2;
    Elem e_a[10];
    int i;

    memset(e_a, 0, sizeof(e_a));

    for (i=0; i<10; i++) 
        e_a[i].key=i;

    ssslist_init(&l1);
    ssslist_init(&l2);

    ssslist_add_tail(&l1, &e_a[0].link);
    ssslist_add_tail(&l1, &e_a[1].link);

    ssslist_add_tail(&l2, &e_a[2].link);
    ssslist_add_tail(&l2, &e_a[3].link);
    ssslist_add_tail(&l2, &e_a[4].link);

    printf("\n"); fflush(stdout);
    printf("-----------------------------------");
    printf("Testing ssslist_add_list_as_tail\n");
    print_list("l1=", &l1);
    print_list("l2=", &l2);
    ssslist_add_list_as_tail(&l1, &l2);
    print_list("l1=", &l1);
    print_list("l2=", &l2);

    ssslist_add_tail(&l1, &e_a[5].link);
    ssslist_add_tail(&l1, &e_a[6].link);
    ssslist_add_list_as_tail(&l1, &l2);
    print_list("l1=", &l1);
    print_list("l2=", &l2);
    ssslist_add_list_as_tail(&l1, &l2);    
    print_list("l1=", &l1);
    print_list("l2=", &l2);
    ssslist_add_list_as_tail(&l2, &l1);        
    print_list("l1=", &l1);
    print_list("l2=", &l2);

    printf("-----------------------------------");
    printf("Testing ssslist_add_list_as_tail\n");

    ssslist_init(&l1);
    ssslist_init(&l2);

    ssslist_add_tail(&l1, &e_a[0].link);
    ssslist_add_tail(&l1, &e_a[1].link);

    ssslist_add_tail(&l2, &e_a[2].link);
    ssslist_add_tail(&l2, &e_a[3].link);
    ssslist_add_tail(&l2, &e_a[4].link);

    print_list("l1=", &l1);
    print_list("l2=", &l2);
    ssslist_add_list_in_front(&l1, &l2);
    print_list("l1=", &l1);
    print_list("l2=", &l2);

    ssslist_add_tail(&l1, &e_a[5].link);
    ssslist_add_tail(&l1, &e_a[6].link);
    ssslist_add_list_in_front(&l1, &l2);
    print_list("l1=", &l1);
    print_list("l2=", &l2);
    ssslist_add_list_in_front(&l1, &l2);    
    print_list("l1=", &l1);
    print_list("l2=", &l2);
    ssslist_add_list_in_front(&l2, &l1);        
    print_list("l1=", &l1);
    print_list("l2=", &l2);
    ssslist_add_list_in_front(&l2, &l1);        
    print_list("l1=", &l1);
    print_list("l2=", &l2);

    printf("-----------------------------------");        
}

/********************************************************************/

/* A self test of the hash-table
 */
int main(void)
{
    Oc_utl_htbl htbl;
    Oc_list list;
    int num_elem;
    Ss_slist free_list1;
    Ss_slist free_list2;
    
    ssslist_init(&free_list1);
    ssslist_init(&free_list2);
    
    // initialize Pl_mm
    pl_init();
    
    test_ssslists();
    
    // initialize memory pools for the test
    test_init();
    
    oc_utl_htbl_create(
        &htbl,
        7 /*1023*/,
        NULL,
        int_hash,
        int_compare);
    
    list_create(
        &list,
        int_compare
        );
    
    // Create 500 random elements and insert into the hashtabl/list
    num_elem = generate_and_insert_elements(&list, &htbl);
    
    // Perform 1000 random operations on the two implementation and
    // check that results match
    random_ops( &list, &htbl, &free_list1, &free_list2 );
    
    return 0;
}

/*********************************************************************************/
