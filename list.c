/*
** list.c
*/

#include <stdio.h>
#include <stdlib.h>
#include "list.h"

extern int debug;


void
list_free(List *lp)
{
    if (lp->v)
	free(lp->v);

    lp->v = NULL;
    lp->s = 0;
    lp->c = 0;
    
    free(lp);
}

List *
list_new(unsigned int s)
{
    List *lp = malloc(sizeof(List));

    
    if (!lp)
	return NULL;

    lp->c = 0;
    lp->s = s ? s : LIST_DEFAULT_SIZE;
    lp->v = malloc(lp->s * sizeof(void *));
    if (lp->v == NULL) {
	list_free(lp);
	return NULL;
    }

    return lp;
}

int
list_foreach(List *lp,
	     int (*foreach)(void *p, void *x), void *x)
{
    int rc = 0;
    unsigned int i;


    for (i = 0; i < lp->c; i++) {
	rc = foreach(lp->v[i], x);
	if (rc)
	    return rc;
    }

    return 0;
}

int
list_append(List *lp, void *p)
{
    void **nv;
    unsigned int ns;
    
    
    if (lp->c+1 >= lp->s) {
	ns = lp->s + LIST_DEFAULT_SIZE;
	nv = realloc(lp->v, ns * sizeof(void *));
	if (nv == NULL)
	    return -1;

	lp->s = ns;
	lp->v = nv;
    }

    if (debug > 4)
	fprintf(stderr, "*** list_append(lp=%p, i=%d, p=%p)\n", lp, lp->c, p);
	    
    lp->v[lp->c++] = p;
    return 0;
}

int
list_add(List *lp,
	 void *p)
{
    unsigned int i;


    for (i = 0; i < lp->c && lp->v != p; i++)
	;

    if (i < lp->c) {
	if (debug > 4)
	    fprintf(stderr, "*** list_add: lp=%p, p=%p already on list\n", lp, p);
	return 1;
    }

    return list_append(lp, p);
}

int
list_delete(List *lp, void *p)
{
    unsigned int i;


    for (i = 0; i < lp->c && lp->v[i] != p; i++)
	;

    if (i == lp->c)
	return -1; /* Not found */

    for (; i+1 < lp->c; i++)
	lp->v[i] = lp->v[i+1];

    lp->c--;
    return 0;
}


void
list_sort(List *lp,
	  int (*sortfun)(const void *p1, const void *p2))
{
    qsort(lp->v, lp->c, sizeof(void *), sortfun);
}
