#ifndef _NPOLL_H_
#define _NPOLL_H_
#include <stdlib.h>
#include <sys/poll.h>

/***
* Copyright (c) 1999 Wolfgang Laun, Wien, Austria (Wolfgang.Laun@chello.at)
* Permission to use, copy, modify, and distribute this material for any legal
* purpose and without fee is hereby granted, provided that the above copyright
* notice and this permission notice appear in all copies. The author makes no
* representations about the accuracy or suitability of this material for any
* purpose;) it is provided "as is", without any express or implied warranties.
***/

/***
Synopsis:
   #include "npoll.h"
   poll_add_fd( fd1, NPOLL_TXT, fread1, NULL, &data1 );
   poll_add_fd( fd2, NPOLL_BIN, fread2, NULL, &data2 );
   poll_add_fd( fd3, MY_RECLEN, fread3, ferr3, NULL );

   poll_rcv( NPOLL_NO_TIMEOUT ); 

   poll_set_read_cb( fd1, fread1a );

Description:
   npoll.c provides a wrapper for calling poll(2) and read(2) on a set of
   input file descriptors. Data is passed to the user via callback
   functions, either as a (newline terminated) line, or a fixed length
   record, or an unspecified quantity of bytes, depending on the selected
   mode. Polling is either limited by some timeout value, or else the user
   is supposed to signal a breakout from a read routine (e.g. at EOF).
   Complete blocking (e.g. poll with no file descriptors and no timeout)
   is avoided.

   To a callback, EOF is indicated by a call with data length 0, except
   for fixed length record mode, where also a short record signals EOF.

   Callbacks may make use of a pointer value defined by the caller.

   Take care to use the correct return value from a callback. NPOLL_RET_NOW
   returns immediately, so there could be more data available even for this
   file descriptor (and others as well). Using NPOLL_RET_IDLE will permit
   reading to continue until blocking would result (which may never happen
   on disk files).

   Removing a file decriptor from within a callback may result in the
   loss of lines that have been read but not yet passed to the caller.

   Not defining a read callback will read (and discard) the data but
   then there's no way of signalling termination to the poll handler.
***/


/* mode flags */
#define NPOLL_BIN 0
#define NPOLL_TXT -1

/* no timeout */
#define NPOLL_NO_TIMEOUT -1.0

/* Return values for read data callback.
 * NPOLL_CONTINUE ... continue polling
 * NPOLL_RET_IDLE ... return after reading all available data
 * NPOLL_RET_NOW  ... return at once
 */
#define NPOLL_CONTINUE 0
#define NPOLL_RET_IDLE 1
#define NPOLL_RET_NOW  2

/* standard callbacks */
#define NPOLL_GOBBLE poll_gobble
#define NPOLL_IGNORE NULL

/* function typedef for read data callback
 *  args:    user data pointer
 *           pointer to data (not zero byte delimited)
 *           length of data, 0 means end of file
 *  returns: NPOLL_... from above list  
 *  note:    To keep the data, copy.
 */
typedef int (*pread_t) ( void*, char*, int );

/* function typedef for read error callback
 *  args:    user data pointer
 *  returns: NPOLL_... from above list  
 */
typedef int (*pfail_t) ( void* );

/* poll_gobble: read callback provided for gobbling data til end of file
 *  args:    see read data callback interface
 *  returns: NPOLL_CONTINUE, NPOLL_RET_IDLE at EOF (so expect
 *           poll_rcv to return when this happens)
 */
int
poll_gobble( void* puser, char* buf, int len);


/* poll_add_fd: add another file descriptor to the polled set
 *  args:    file descriptor
 *           mode: NPOLL_TXT, NPOLL_BIN or record length
 *           callback for data (user function or NPOLL_GOBBLE, NPOLL_IGNORE)
 *           callback for error (user function or NPOLL_IGNORE)
 *           user data pointer for callbacks (NULL if not needed)
 *  returns: 1 ... OK, 0 ... error (malloc fails)
 *  note:    An integer value >0 for mode will result in a behaviour
 *           similar to fread(3).
 */
int
poll_add_fd( int     fd,
             int     pmode,
             pread_t pread,
             pfail_t pfail,
             void*   puser );

/* poll_del_fd: remove file descriptor from the polled set
 *  args:    file descriptor
 *  returns: 1 ... OK, 0 ... error (no such descriptor)
 */ 
int
poll_del_fd( int fd );


/* poll_rcv: poll file decsriptors
 *  args:    timeout value in seconds (double), or NPOLL_NO_TIMEOUT
 *  returns: =0 ... timeout expires
 *           >0 ... read callback break-out
 *           -1 ... poll() call error (errno is set)
 *  notes:   Timeout granularity is 1 millisecond.
 *           Return values >0 signal a breakout caused by a read or fail
 *           callback return values.
 */
int
poll_rcv( double timeout );


/* poll_set_read_cb: set/get read callback
 *  args:    file descriptor
 *           new read callback
 *  returns: old read callback
 */
pread_t
poll_set_read_cb( int fd, pread_t pread );

/* poll_set_fail_cb: set/get fail callback
 *  args:    file descriptor
 *           new fail callback
 *  returns: old fail callback
 */
pfail_t
poll_set_fail_cb( int fd, pfail_t pfail );


/*
 *  internal storage for registering file descriptors
 */
#define NPOLL_VECLEN 10
typedef struct pollinfo {
  int     pmode;  /* read mode: text, binary, record length */
  int     pflag;  /* saved open mode flags */
  size_t  pbinc;  /* buffer allocation and increment qtty */
  char*   pbptr;  /* buffer pointer */
  size_t  pblen;  /* allocated buffer length */
  char*   pdbeg;  /* pointer to next byte to pass to user */
  char*   pdend;  /* pointer to next free buffer byte */
  pread_t pread;  /* read callback function pointer */
  pfail_t pfail;  /* error callback function pointer */
  void*   puser;  /* user data pointer for callbacks */
} pollinfo_t;

#define NPOLL_TXT_BUFLEN 1024
#define NPOLL_BIN_BUFLEN 4096

#endif  /*_NPOLL_H_*/
