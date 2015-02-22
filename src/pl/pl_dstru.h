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
#ifndef SS_DSTRU_H
#define SS_DSTRU_H

/* Changes by PMW for debug information */

#include "pl_base.h"
#include "pl_base.h"
#include <stdio.h>

/* 
** If a given structure is a substructure, then return the structure it is
** part of.
*/
#define SS_START_OF_STRUCT(node_p,type,member) \
((type *)((unsigned int)node_p-(unsigned int)(&(((type *)0)->member))))

/*
 * Simplistic Singly linked list  (Not thread safe)
 */
typedef struct Ss_slist_node {
    struct Ss_slist_node * next;
} Ss_slist_node;

typedef struct {
    struct Ss_slist_node * first;
    struct Ss_slist_node * last;
    int32 count;
} Ss_slist;

/*
** Returns true if the singly linked list is empty, false otherwise
*/
static __inline__ bool ssslist_empty(Ss_slist *slist)
{
    return (bool)(slist->count == 0);
}

/*
** Initialses the singly linked list.
*/
static __inline__ void ssslist_init( /*@out@*/ Ss_slist * slist) 
{ 
    slist->first = slist->last = (Ss_slist_node *)0; 
    slist->count = 0;
}

/*
** Initialises a Singly linked list node
*/
static __inline__ void ssslist_node_init( /*@out@*/ Ss_slist_node *slist_node) 
{ 
    slist_node->next = (Ss_slist_node *)0; 
}

/*
** Return the number of elements in a singly linked list
*/
static __inline__ int32 ssslist_get_length(Ss_slist *slist)
{
    return slist->count;
}

/*
** Returns the first element of a singly linked list
*/
/*@null@*/ static __inline__ void *ssslist_head( /*@partial@*/ Ss_slist *slist )
{
    return (slist->first);
}

/*
** Returns the last element of a singly linked list
*/
/*@null@*/ static __inline__ void *ssslist_tail( /*@partial@*/ Ss_slist *slist )
{
    return (slist->last);
}

/*
** Returns the element after the element given.
*/
/*@null@*/ static __inline__ void *ssslist_next( /*@partial@*/ Ss_slist_node *node )
{
    return (node->next);
}

/*
** Calculates the number of elements in a singly linked list by iterating
** through the next pointers of nodes.
*/
static __inline__ int ssslist_calculate_len (Ss_slist *slist) 
{
    int i = 0;
    Ss_slist_node *node = slist->first;

    while (node != (Ss_slist_node *) 0) {
        i++;
        node = node -> next;
    }

    return i;
}

/*
** Adds an element to the tail of a Singly linked list
*/
static __inline__ void ssslist_add_tail( Ss_slist *slist,
                                         Ss_slist_node *slist_node )
{
    slist_node->next = (Ss_slist_node *)0;
    if (slist->first == (Ss_slist_node *)0) { /* First node */
        slist->first = slist->last = slist_node;
    } else {
        slist->last->next = slist_node;
        slist->last = slist_node;
    }
    slist->count++;

} 

/*
** Add an element to the head of a singly linked list
*/
static __inline__ void ssslist_add_head(Ss_slist *slist, Ss_slist_node *slist_node)
{
    if (slist->first == (Ss_slist_node *)0) {
        slist_node->next = (Ss_slist_node *)0;
        slist->first = slist->last = slist_node;
    } else {
        slist_node->next = slist->first;
        slist->first = slist_node;
    }
    slist->count++;

} 

/*
** Remove, and return the head of a singly linked list
*/
/*@null@*/ static __inline__ Ss_slist_node * ssslist_remove_head(Ss_slist *slist)
{
    Ss_slist_node *node = slist->first;

    if (node) {
        if (node->next==(Ss_slist_node*)0) { /* only the 1 node */
            slist->first = slist->last = (Ss_slist_node *)0;
        } else {
            slist->first = node->next;
        }
    }

    if (node) {
        slist->count--;
    } else {
#ifndef __KERNEL__
        ss_assert(slist->count == 0);
        ss_assert(slist->first == slist->last);
        ss_assert(slist->first == (Ss_slist_node *)0);
#endif
    }

    return node;
} 

/*
** Move a list from 1 storage place to another
*/
static __inline__ void ssslist_move_list ( Ss_slist * source,
                                           Ss_slist * target )
{
    target->first = source->first ;
    target->last = source->last ;
    target->count = source->count ;
    source->first = (Ss_slist_node *)0 ;
    source->last = (Ss_slist_node *)0 ;
    source->count = 0;
} /* end ssslist_move_list */


/* Enqueue a list at the tail of another list. 
 */
static __inline__ void ssslist_add_list_as_tail ( Ss_slist * source,
                                                  Ss_slist * target )
{
    if (NULL == source->first)
        // source list is empty. 
        return;
    else if (NULL == target->first) {
        // target list is empty
        ssslist_move_list(source,target);
        return;
    }
    else {
        // both are non-empty
        target->last->next = source->first;
        target->last = source->last;
        target->count += source->count ;
        source->first = (Ss_slist_node *)0 ;
        source->last = (Ss_slist_node *)0 ;
        source->count = 0;
    }
}

/* Enqueue a list at the tail of another list. 
 */
static __inline__ void ssslist_add_list_in_front ( Ss_slist * source,
                                                   Ss_slist * target )
{
    if (NULL == source->first)
        // source list is empty. 
        return;
    else if (NULL == target->first) {
        // target list is empty
        ssslist_move_list(source,target);
        return;
    }
    else {
        // both are non-empty
        Ss_slist_node * first_old;
        
        first_old = target->first;
        target->first = source->first;
        source->last->next = first_old;
        target->count += source->count ;
        source->first = (Ss_slist_node *)0 ;
        source->last = (Ss_slist_node *)0 ;
        source->count = 0;
    }
}

/*
** Remove a given node from a linked list
*/
static __inline__ bool ssslist_remove_node(Ss_slist *slist,
                                           Ss_slist_node *node)
{
    bool found = (bool)0;
    Ss_slist_node * temp=(Ss_slist_node *)0;

    if ( slist->first == node ) {
        temp = ssslist_remove_head(slist);
        found = (bool)1;
    } else {
        for(temp = slist->first;
            temp != (Ss_slist_node *)0;
            temp = temp->next ) {
            if (temp->next == node) {
                found = (bool)1; 
                temp->next = node->next;
                if (node->next == (Ss_slist_node*)0) {
                    slist->last=temp;
                }
                slist->count --;
                break; /* out of for loop */
            } /* end if */
        } /* end for */
    }

    return found;
}



/*
** Doubly linked list (Queue) (Not thread safe)
*/

typedef struct Ss_dlist
{ struct Ss_dlist_node * first;
  struct Ss_dlist_node * last;
  int32 count;
} Ss_dlist;

typedef struct Ss_dlist_node
{
    struct Ss_dlist_node * next;
    struct Ss_dlist_node * prev;
} Ss_dlist_node;


/*
** Checks if a dlist is empty
*/
static __inline__ bool ssdlist_empty(Ss_dlist *dlist)
{ 
    return (bool)(dlist->count == 0);
}

/* 
** Initialises a dlist
*/
static __inline__ void ssdlist_init (/*@out@*/ Ss_dlist *dlist)
{
    dlist->first = (Ss_dlist_node *)0;
    dlist->last  = (Ss_dlist_node *)0;
    dlist->count = 0;
}

/*
** Initialise a dlist node
*/
static __inline__ void ssdlist_node_init (/*@out@*/ Ss_dlist_node *node)
{
    node->next = (Ss_dlist_node *)0;
    node->prev = (Ss_dlist_node *)0;
}

/*
** Returns the first element of a dlist
*/
/*@null@*/ static __inline__ void *ssdlist_head( /*@partial@*/ Ss_dlist *dlist )
{
    return (dlist->first);
}

/*
** Returns the last element of a dlist
*/
/*@null@*/ static __inline__ void *ssdlist_tail( /*@partial@*/ Ss_dlist *dlist )
{
    return (dlist->last);
}

/*
** Returns the element after the element given.
*/
/*@null@*/ static __inline__ void *ssdlist_next( /*@partial@*/ Ss_dlist_node *node )
{
    return (node->next);
}
       
/*
** Returns the element before the element given
*/
/*@null@*/ static __inline__ void *ssdlist_prev( /*@partial@*/ Ss_dlist_node *node )
{
    return (node->prev);
}

static __inline__ int32 ssdlist_get_length(Ss_dlist *dlist)
{ 
    return dlist->count;
}
       

/*
** Calculate the number of elements in a doubly linked list, from both
** sides.
*/
static __inline__ int ssdlist_calculate_len (Ss_dlist *dlist)
{ 
    int i = 0;
    int j = 0;

    Ss_dlist_node *node = (Ss_dlist_node *)ssdlist_head(dlist);
    
    while (node != (Ss_dlist_node *) 0) {
        i++;
        node = (Ss_dlist_node *)ssdlist_next(node);
    }
        
    node = (Ss_dlist_node *)ssdlist_tail(dlist);
    while (node != (Ss_dlist_node *) 0) {
        j++;
        node = (Ss_dlist_node *)ssdlist_prev(node);
    }

    if (i!=j) {
        ERR(("ssdlist_calculate_len, count by next %d count by prev %d",
                 i,j));
    }

    return i;
}


/*
** Adds an element to the end of a dlist
*/
static __inline__ void ssdlist_add_tail (Ss_dlist *dlist,
                                         Ss_dlist_node *node)
{
    node->next = (Ss_dlist_node *)0;
    node->prev = dlist->last;

    if (dlist->last != (Ss_dlist_node *)0) {
        dlist->last->next = node;
    } else {
        dlist->first = node;
    }

    dlist->last = node;
    dlist->count++;
}

/*
** Adds an element to the beginning of a dlist
*/
static __inline__ void ssdlist_add_head( Ss_dlist *dlist,
                                         Ss_dlist_node *node )
{
    node->prev = (Ss_dlist_node *)0;
    node->next = dlist->first;

    if (dlist->first != (Ss_dlist_node *)0) {
        dlist->first->prev = node;
    } else {
        dlist->last = node; 
    }

    dlist->first = node;
    dlist->count++;
}

/*
** Adds an element to a dlist before the element "before"
*/
static __inline__ void ssdlist_add_before( Ss_dlist *dlist,
                                           Ss_dlist_node *before, 
                                           Ss_dlist_node *node)
{
    if (before->prev == (Ss_dlist_node *)0) {
       ssdlist_add_head(dlist,node);
    } else {
        node->prev = before->prev;
        node->next = before;
        before->prev->next = node;
        before->prev = node; 
        dlist->count++;
    }

    /* Debug test to see that the length of the list is right */
}

/*
** Removes a given element from a dlist
*/
/*@null@*/ static __inline__ void *ssdlist_remove( Ss_dlist *dlist,
                                                   Ss_dlist_node *node)
{
    if (node == (Ss_dlist_node *)0) {
        return node;
    }
    if (node->next != (Ss_dlist_node *)0) {
        node->next->prev = node->prev;
    } else {
        dlist->last = node->prev; 
    }

    if (node->prev != (Ss_dlist_node *)0) {
        node->prev->next = node->next;
    } else {
        dlist->first = node->next; 
    }
    
    dlist->count--;

    return node;
}

/*
** Removes & returns the first element in a dlist
*/
/*@null@*/ static __inline__ void *ssdlist_remove_head( /*@partial@*/ Ss_dlist *dlist )
{
    Ss_dlist_node *node=(Ss_dlist_node *)0; //return NULL by default

    if( (node = dlist->first) != (Ss_dlist_node *)0) {
        node = (Ss_dlist_node *)ssdlist_remove( dlist, node );
    }
  
    return node;
}       

/*
** Removes & returns the last element in a dlist
*/
/*@null@*/ static __inline__ void *ssdlist_remove_tail( /*@partial@*/ Ss_dlist *dlist )
{
    Ss_dlist_node *node;

    if( (node = dlist->last) != (Ss_dlist_node *)0 ) {
        node = (Ss_dlist_node *)ssdlist_remove( dlist, node );
    }

    return node;
}

/*
** Move a list from 1 storage place to another
*/
static __inline__ void ssdlist_move_list ( /*@partial@*/  Ss_dlist * source,
                                           /*@out@*/ Ss_dlist * target )
{
    target->first = source->first ;
    target->last = source->last ;  
    target->count = source->count ;
    source->first = (Ss_dlist_node *)0 ;
    source->last = (Ss_dlist_node *)0 ;
    source->count = 0;
} 



#endif


