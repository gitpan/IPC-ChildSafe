#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "npoll.h"

/***
* Copyright (c) 1999 Wolfgang Laun, Wien, Austria (Wolfgang.Laun@chello.at)
* Permission to use, copy, modify, and distribute this material for any legal
* purpose and without fee is hereby granted, provided that the above copyright
* notice and this permission notice appear in all copies. The author makes no
* representations about the accuracy or suitability of this material for any
* purpose;) it is provided "as is", without any express or implied warranties.
***/

/*** Description: See npoll.h ***/

#define EXTEND(Type,Vec,Len,Inc,Use) \
  ( (Use) == Len ? Vec = (Type*)realloc( Vec,(Len+=Inc)*sizeof(Type) ) : Vec )

/* Poll vector */
static int            fd_use = 0;
static struct pollfd* fd_vec = NULL;
static int            fd_len = 0;

/* Internal info vector (Some redundancy so we can use EXTEND) */
static int         pi_use = 0;
static pollinfo_t* pi_vec = NULL;
static int         pi_len = 0;

int
poll_add_fd( int     fd,
             int     pmode,
             pread_t pread,
             pfail_t pfail,
             void*   puser ){

  if( EXTEND( pollinfo_t, pi_vec, pi_len, NPOLL_VECLEN, pi_use ) &&
      EXTEND( struct pollfd,   fd_vec, fd_len, NPOLL_VECLEN, fd_use ) ){
    /* Add another entry to both vectors. No mallocs yet. */
    fd_vec[fd_use].fd      = fd;
    fd_vec[fd_use].events  = POLLIN | POLLPRI;
    fd_vec[fd_use].revents = 0;
    fd_use++;

    pi_vec[pi_use].pmode = pmode;
    pi_vec[pi_use].pflag = fcntl( fd, F_GETFL );
    pi_vec[pi_use].pbinc = pmode == NPOLL_TXT ? NPOLL_TXT_BUFLEN :
                           pmode == NPOLL_BIN ? NPOLL_BIN_BUFLEN : pmode;
    pi_vec[pi_use].pblen = 0;
    pi_vec[pi_use].pbptr = NULL;
    pi_vec[pi_use].pdbeg = NULL;
    pi_vec[pi_use].pdend = NULL;
    pi_vec[pi_use].pread = pread;
    pi_vec[pi_use].pfail = pfail;
    pi_vec[pi_use].puser = puser;
    pi_use++;

    if( fcntl( fd, F_SETFL,  pi_vec[pi_use].pflag | O_NONBLOCK ) == -1 ){
      return 0;
    }
    return 1;

  } else {

    /* Cannot allocate memory */
    return 0;
  }
}


int
poll_del_fd( int fd ){
  int id;
  for( id = 0; id < fd_use; id++ ){
    if( fd == fd_vec[id].fd ){
      /* Found the fd. Restore flags. Clean up. Compress vector. */
      fcntl( fd, F_SETFL,  pi_vec[id].pflag );
      free( pi_vec[id].pbptr );
      fd_vec[id] = fd_vec[--fd_use];
      pi_vec[id] = pi_vec[--pi_use];
      return 1;
    }
  }
  return 0; /* fd not found */
}

pread_t
poll_set_read_cb( int fd, pread_t pread ){
  int id;
  for( id = 0; id < fd_use; id++ ){
    if( fd == fd_vec[id].fd ){
      /* Found the fd: set/get read callback */
      pread_t prev = pi_vec[pi_use].pread;
      pi_vec[pi_use].pread = pread;
      return prev;
    }
  }
  return NULL; /* fd not found */
}


pfail_t
poll_set_fail_cb( int fd, pfail_t pfail ){
  int id;
  for( id = 0; id < fd_use; id++ ){
    if( fd == fd_vec[id].fd ){
      /* Found the fd: set/get fail callback */
      pfail_t prev = pi_vec[pi_use].pfail;
      pi_vec[pi_use].pfail = pfail;
      return prev;
    }
  }
  return NULL; /* fd not found */
}


int
poll_rcv( double timeout ){
  int milsec = (int)(timeout*1000);
  int nready = 0;

  /* Some platforms (HP/UX) insist on this being exactly -1. */
  if( milsec < 0 ) milsec = -1;

  /* Guard against enternal blocking: avoid empty vector and no timeout. */
  while( ( fd_use > 0 || milsec >= 0 ) &&
         ( nready = poll( fd_vec, fd_use, milsec ) ) > 0 ){
    int stop = 0;
    int id;

    for( id = 0; nready > 0 && id < fd_use; id++ ){
      int rv = 0;

      if( ( POLLIN | POLLPRI ) & fd_vec[id].revents ){
        /* We have data to read. */
        long bread;
        int buse = pi_vec[id].pdend - pi_vec[id].pbptr;
        int boff = pi_vec[id].pdbeg - pi_vec[id].pbptr;
        nready--;

        /* Make sure there is some room in the buffer. */
        if( EXTEND( char, pi_vec[id].pbptr, pi_vec[id].pblen,
                    pi_vec[id].pbinc, buse ) ){
          char* thisbuf = pi_vec[id].pbptr;
          int h, bytes;

          /* Recompute - EXTEND may realloc the buffer. */
          pi_vec[id].pdbeg = pi_vec[id].pbptr + boff;
          pi_vec[id].pdend = pi_vec[id].pbptr + buse;

          /* We can read at most until the end of the buffer. */
          bread = read( fd_vec[id].fd,
                        pi_vec[id].pdend,
		        pi_vec[id].pblen - buse );

          if( bread == -1 ){

            /* Error handling. */
            if( pi_vec[id].pfail ){
	      rv = pi_vec[id].pfail( pi_vec[id].puser );
            } else {
              char msg[80];
              sprintf( msg, "poll_rcv: error reading fd %d", fd_vec[id].fd );
              perror( msg );
              exit( 1 );
            }

          } else {

            /* Success. We've read some bytes or not (EOF). */
            pi_vec[id].pdend += bread;

            if( pi_vec[id].pmode == NPOLL_TXT && bread ){

              /* Text mode (except EOF). */
              char* eolptr;
              bytes = pi_vec[id].pdend - pi_vec[id].pdbeg;

              /* Text: Scan for EOL, to give the caller one line at a time. */
              while( bytes &&
                     ( eolptr = (char*)memchr( pi_vec[id].pdbeg, '\n', bytes ) ) ){
                h = eolptr - pi_vec[id].pdbeg + 1;
                if( pi_vec[id].pread ){
                  rv = pi_vec[id].pread( pi_vec[id].puser, pi_vec[id].pdbeg, h );
                  if( thisbuf != pi_vec[id].pbptr ){
                    /* Panic: This entry got yanked from the callback. */
                    id--; goto next_fd;
                  }
                }
                pi_vec[id].pdbeg = eolptr + 1;
                bytes -= h;

                if( rv != NPOLL_CONTINUE ) break;
              }

              /* Shift what is left in the buffer (could have length 0). */
              memmove( pi_vec[id].pbptr, pi_vec[id].pdbeg, 
                       h = pi_vec[id].pdend - pi_vec[id].pdbeg );
              pi_vec[id].pdbeg = pi_vec[id].pbptr;
              pi_vec[id].pdend = pi_vec[id].pbptr + h;

            } else if( pi_vec[id].pmode == NPOLL_BIN ||
                       pi_vec[id].pmode > 0 &&
                       pi_vec[id].pmode == pi_vec[id].pdend-pi_vec[id].pdbeg ||
                       bread == 0 ){
              /* Binary, or complete record, or EOF: Give the caller everything. */
              if( pi_vec[id].pread ){
                rv = pi_vec[id].pread( pi_vec[id].puser,
                                       pi_vec[id].pdbeg,
                                       pi_vec[id].pdend-pi_vec[id].pdbeg );
                if( thisbuf != pi_vec[id].pbptr ){
                  /* Panic: this entry got yanked from callback. */
                  id--; goto next_fd;
                }
              }
              pi_vec[id].pdbeg = pi_vec[id].pbptr;
              pi_vec[id].pdend = pi_vec[id].pbptr;
            }
	  }
        }
      }
next_fd:
      if( rv == NPOLL_RET_NOW ) return NPOLL_RET_NOW;
      stop += rv;
    }
    if( stop ) milsec = 0;
  }
  return nready;
}

int
poll_gobble( void* puser, char* buf, int len){
  return len ? NPOLL_CONTINUE : NPOLL_RET_IDLE;
}
