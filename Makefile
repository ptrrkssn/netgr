# Makefile for netgr

CC=gcc
CFLAGS=-O -g -Wall

OBJS=netgr.o strmatch.o list.o

all: netgr

netgr: $(OBJS)
	$(CC) -o netgr $(OBJS) -lnsl

clean distclean:
	-rm -f *~ \#* core *.o netgr
