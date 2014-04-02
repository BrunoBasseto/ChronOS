/**
 * @file list.c
 * @brief Linked lists framework.
 *
 * @author Bruno Basseto (bruno@wise-ware.org)
 * @version 0
 */

/********************************************************************************
 ********************************************************************************
 * The MIT License (MIT)
 *
 * Copyright (c) 1995-2014 Bruno Basseto.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 ********************************************************************************
 ********************************************************************************/

#include "chronos.h"
#include <stdlib.h>

#define LISTPTR(x)      ((list_item_t*)x)

/**
 * Add an element to the end of a list.
 * @param list Pointer to the list (= first element).
 * @param item Item to add.
 */
void
list_add
   (void *list,
   void *item)
{
   register list_item_t *p;
   register list_item_t **lst;
   
   lst = (list_item_t**)list;
   
   if(*lst == NULL) {
      /*
       * This is going to be the first item.
       */
      *lst = item;
      (*lst)->next = NULL;
      (*lst)->prev = NULL;
      return;
   }
   
   /*
    * Insert at the end.
    * list->prev points to the last element.
    */
   p = (*lst)->prev;
   if(p == NULL) p = *lst;
   (*lst)->prev = (list_item_t*)item;
   LISTPTR(item)->prev = p;
   LISTPTR(item)->next = NULL;
   p->next = item;
}

/**
 * Insert an item at the beginning of a list.
 * @param list Pointer to the list (= first element).
 * @param item Item to add.
 */
void
list_push
   (void *list,
   void *item)
{
   register list_item_t **lst;
   
   lst = (list_item_t**)list;
   
   if(*lst == NULL) {
      /*
       * This is going to be the first item.
       */
      *lst = item;
      (*lst)->next = NULL;
      (*lst)->prev = NULL;
      return;
   }
   
   /*
    * Insert at the beginning.
    * list points to the first element.
    */
   LISTPTR(item)->prev = (*lst)->prev;
   LISTPTR(item)->next = *lst;
   *lst = item;
}

/**
 * Remove one element from the list.
 * @param list Pointer to the list (= first element).
 * @param item Item to remove.
 */
void
list_remove
   (void *list,
   void *item)
{
   list_item_t *p, *q;
   register list_item_t **lst;

   lst = (list_item_t**)list;   
   if(item == NULL) return;
   if(*lst == NULL) return;
   
   /*
    * Exchange ponters item->next and item->prev.
    */
   p = LISTPTR(item)->next;
   q = LISTPTR(item)->prev;

   if(*lst == item) {
      /*
       * We are removing the first element.
       */
       *lst = p;
       if(p != NULL) p->prev = q;
       return;
   }
   
   if(p != NULL) p->prev = q;
   else {
      /*
       * We are removing the last element.
       */
      (*lst)->prev = q;
   }
   if(q != NULL) q->next = p;
}

/**
 * Remove and returns the first element of a list.
 * @param list Pointer to the list (= first element).
 * @return Removed item or NULL if the list was empty.
 */
void*
list_pop
   (void *list)
{
   register list_item_t *p;
   register list_item_t **lst;

   lst = (list_item_t**)list;   
   if(*lst == NULL) return NULL;
   
   p = (*lst)->next;
   if(p != NULL) p->prev = (*lst)->prev;
   p = *lst;
   *lst = p->next;
   return p;
}

/**
 * Check if an element is part of a lista.
 * @param list Pointer to the list (= first element).
 * @param item Item to find.
 * @return TRUE if the list contains the element.
 */
bool_t
list_contains
   (void *list,
   void *item)
{
   list_item_t *p;
   for(p = (list_item_t*)list; p != NULL; p = p->next) {
      if(item == p) return TRUE;
   }
   return FALSE;
}

/**
 * Returns the number of elements in a list.
 * @param list Pointer to the list (= first element).
 * @return Number of elements.
 */
uint16_t
list_length
   (void *list)
{
   list_item_t *p;
   int i;
   for(i = 0, p = (list_item_t*)list; p != NULL; p = p->next) i++;
   return i;
}
