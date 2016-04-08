/*
** list.h
*/

#ifndef LIST_H

#define LIST_DEFAULT_SIZE 256

typedef struct {
    unsigned int c;
    unsigned int s;
    void **v;
} List;

extern List *list_new(unsigned int s);

extern void list_free(List *lp);

extern int list_foreach(List *lp, int (*foreach)(void *p, void *x), void *x);

extern int list_append(List *lp, void *p);
extern int list_add(List *lp, void *p);

extern int list_delete(List *lp, void *p);

extern void list_sort(List *lp, int (*sortfun)(const void *p1, const void *p2));

#endif


