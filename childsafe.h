#ifndef _CHILD_H_
#define _CHILD_H_

/*
 ****************************************************************************
 ** Copyright (c) 1997 David Boyce (dsb@world.std.com). All rights reserved.
 ** This program is free software; you can redistribute it and/or
 ** modify it under the same terms as Perl itself.
 ****************************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/** This is analogous to a FILE handle for fopen/fgets/fputs/fclose. **/
typedef struct
{
    char *cph_cmd;
    FILE *cph_down;
    FILE *cph_back;
    FILE *cph_err;
    char *cph_tag;
    char *cph_eot;
    char *cph_quit;
    pid_t cph_pid;
    unsigned cph_errs;
    unsigned cph_pending;
    AV   *cph_out_array;
    AV   *cph_err_array;
} CHILD;

/** Public C interfaces in childsafe.c **/
extern CHILD *child_open(char *, char *, char *, char *);
extern int child_puts(char *, CHILD *, AV *, AV *);
extern char *child_gets(char *, int, CHILD *);
extern int child_end(CHILD *, int);
extern int child_close(CHILD *);
extern int child_kill(CHILD *, int);

/**
 ** These externals are exported in case the client wants to tweak them.
 **/
extern int Debug_Level;
extern int Alarm_Wait;

/**
 ** Flag values which indicate whether we want to see error output.
 **/
#define CP_SHOW_ERR 0
#define CP_NO_SHOW_ERR 1

#endif				/* _CHILD_H_ */
