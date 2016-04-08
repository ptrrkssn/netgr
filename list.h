/*
** list.h
**
** Copyright (c) 2016 Peter Eriksson <pen@lysator.liu.se>
**
** This file is part of netgr.
**
** netgr is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
** 
** netgr is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
** 
** You should have received a copy of the GNU General Public License
** along with netgr.  If not, see <http://www.gnu.org/licenses/>.
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


