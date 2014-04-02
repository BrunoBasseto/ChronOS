
#ifndef __LISTH__
#define __LISTH__

/**
 * Common fields of a list.
 * Must be the first element of any list structure.
 */
typedef struct {
   void *next;
   void *prev;
} list_item_t;

void list_add(void *list, void *item);
void list_push(void *list, void *item);
void list_remove(void *list, void *item);
void *list_pop(void *list);
bool_t list_contains(void *list, void *item);

#define list_for_each(X, Y)   for(Y=X;Y!=NULL;Y=Y->list.next)
#define for_each(ARRAY, PTR)  for(PTR=ARRAY; ((word_t)(PTR)-(word_t)(ARRAY))<sizeof(ARRAY); PTR++)

#endif
