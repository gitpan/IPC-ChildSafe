# Makefile for building the C sample code. Modify as required.

CC = gcc
CFLAGS = -I.. -g

all:: libchild.a kshdemo ccdemo

child.o: ../child.h ../child.c
	$(CC) $(CFLAGS) -c ../$*.c

libchild.a: child.o
	@rm -f $@
	$(AR) r $@ $?

kshdemo: kshdemo.o libchild.a
	$(CC) -o kshdemo kshdemo.o libchild.a

ccdemo: ccdemo.o libchild.a
	$(CC) -o ccdemo ccdemo.o libchild.a
