# Makefile for building the C sample code. Modify as required.

CC = gcc
CFLAGS = -I.. -g

all:: libchildsafe.a shdemo ccdemo

childsafe.o: ../childsafe.h ../childsafe.c
	$(CC) $(CFLAGS) -c ../$*.c

libchildsafe.a: childsafe.o
	@rm -f $@
	$(AR) r $@ $?

shdemo: shdemo.o libchildsafe.a
	$(CC) $(CFLAGS) -o shdemo shdemo.o libchildsafe.a

ccdemo: ccdemo.o libchildsafe.a
	$(CC) $(CFLAGS) -o ccdemo ccdemo.o libchildsafe.a

clean::
	@rm -f *.o *.a
