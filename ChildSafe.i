/*
 ****************************************************************************
 ** Copyright (c) 1997 David Boyce (dsb@world.std.com). All rights reserved.
 ** This program is free software; you can redistribute it and/or
 ** modify it under the same terms as Perl itself.
 ****************************************************************************
 *
 * To regenerate wrappers using SWIG:
 * Remember to move ChildSafe.pm aside first, since swig seems
 * to want to overwrite it! Then, run:
 *   swig -perl5 -dhtml -package IPC::ChildSafe -module ChildSafe ChildSafe.i
 * After this you have to patch a few places:
 *   perl -pi -e 's/boot_ChildSafe/boot_IPC__ChildSafe/' ChildSafe_wrap.c
 * Then move the ChildSafe.pm file back into place, and proceed with the usual:
 *   perl Makefile.PL
 *   make
 *   make test
 *   (as root?) make install
 *
 * SWIG is at http://www.swig.org/.
 */

%module ChildSafe
%{
#include "child.h"
%}

/**
 ** The interfaces we make available to IPC::ChildSafe.pm
 **/
CHILD *child_open(char *, char *, char *); /* Open (start) a child */
int child_puts(char *, CHILD *); /* Send a command to the child */
%new char *child_get_stdout_perl(CHILD *); /* Read back a line of output */
%new char *child_get_stderr_perl(CHILD *); /* Read a line of stderr output */
int child_close(CHILD *);     /* Close (end) the child */
int child_kill(CHILD *, int); /* Send a signal to the child process */

int Debug_Level; /* For tracing parent/child communication */
int Alarm_Wait;  /* Period to allow blocking before poll of stderr */
