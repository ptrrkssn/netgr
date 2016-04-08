/*
** strmatch.h - String Wildcard matching
**
** Copyright (c) 2002 Peter Eriksson <pen@lysator.liu.se>
** Copyright (c) 1995 by Marcus E. Hennecke		    
**
** Original author: Marcus E. Hennecke <marcush@leland.stanford.edu>
**
** This program is free software; you can redistribute it and/or
** modify it as you wish - as long as you don't claim that you wrote
** it.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef PLIB_STRMATCH_H
#define PLIB_STRMATCH_H

int
strmatch(const char *string,
	 const char *pattern);

#endif
