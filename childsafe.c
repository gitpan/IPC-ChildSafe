#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "childsafe.h"
#include "npoll.h"

/**
 ****************************************************************************
 ** Copyright (c) 1997-2001 David Boyce (dsbperl@cleartool.com).
 ** All rights reserved.
 ** This program is free software; you can redistribute it and/or
 ** modify it under the same terms as Perl.
 ****************************************************************************
 ** To understand the C code and underlying design, look at
 ** W.R. Stevens (Advanced Programming in the Unix Environment)
 ** chapters 14 and 15. Also chapter 19 to get into how it interacts
 ** with coprocesses using ptys. This code goes way beyond any discussions
 ** of coprocesses in Stevens but he does a good job describing the
 ** issues and defining terms. Also look at the comments in
 ** IPC::Open3.pm in the Perl5 distribution.
 ** Some of the code here was cut-and-pasted directly from the
 ** examples given by Stevens (ISBN 0-201-56317-7).
 **/

static char *xs_ver = "@(#) IPC::ChildSafe " XS_VERSION;

/** Used for debugging output **/
#define F __FILE__
#define L __LINE__

/** Convenience **/
#ifndef NUL
#define NUL '\0'
#endif

/** Ok, these should be dynamic allocations ... maybe someday **/
#define STDLINE 131072
#define BIGLINE	(STDLINE<<3)

/** Print debugging messages that assert this level (0=off, 1=low, ...) **/
int Debug_Level = 0;

/** Boolean - print commands with a leading "-" but don't run them **/
int No_Exec;

static CHILD *mru_handle;	/* most-recently-used handle */

/** Return a ptr to the '\0' which terminates the given string **/
#define endof(str) strchr(str,NUL)

/**
 ** This is used to (1) remember the most-recently-used handle
 ** (in parameter 'm'), and (2) use this most-recently-used handle if the
 ** handle we were passed (parameter 'h') is NULL.  The value to return
 ** on error is taken as a parameter ('r') because different functions
 ** return different types.
 **/
#define UPDATE_HANDLE(p,m,r)		\
   {					\
      if (p == NULL) p = m;		\
      if ((m = p) == NULL) return r;	\
      if (p->cph_pid == 0) return r;	\
   }

typedef	void	Sigfunc(int);

Sigfunc *
reliable_signal(int signo, Sigfunc *func)
{
    struct sigaction act, oact;

    act.sa_handler = func;
    if (sigemptyset(&act.sa_mask))
	return (SIG_ERR);
    act.sa_flags = 0;
    if (signo == SIGALRM) {
#ifdef	SA_INTERRUPT
	act.sa_flags |= SA_INTERRUPT;	/* SunOS */
#endif
    } else {
#ifdef	SA_RESTART
	act.sa_flags |= SA_RESTART;	/* SVR4, 44BSD */
#endif
    }
    if (sigaction(signo, &act, &oact) < 0)
	return (SIG_ERR);
    return (oact.sa_handler);
}

/**
 ** This prints a format string and args, in the manner of printf,
 ** to stderr if the specified debug level is exceeded.
 ** The filename, line number, and user name are prepended.
 ** We append a newline if there wasn't already one at the end
 ** of the format string.
 **/
static void
_dbg(const char *f, const unsigned long l, int lvl, const char *fmt,...)
{
   va_list ap;
   char buf[STDLINE], *e, x;

   if ((!No_Exec && Debug_Level < lvl) || (No_Exec && lvl > 1))
      return;

   x = No_Exec ? '-' : '+';
   if (lvl > 2) {
       (void) fprintf(stderr, "%c %s:%d ", x, ((e=strrchr(f,'/'))?e+1:f), l);
   } else {
       (void) fprintf(stderr, "%c ", x);
   }
   va_start(ap, fmt);
   (void) vsprintf(buf, fmt, ap);
   va_end(ap);
   (void) fputs(buf, stderr);
   if ((e = strchr(buf, NUL)) && (*--e != '\n'))
      (void) fputc('\n', stderr);
   (void) fflush(stdout);
}

/**
 ** Print an error message and quit.
 **/
static void
_cp_syserr(const char *fmt,...)
{
   va_list ap;
   char buf[STDLINE], *e;

   va_start(ap, fmt);
   (void) vsprintf(buf, fmt, ap);
   va_end(ap);
   (void) fputs(buf, stderr);
   if ((e = strchr(buf, NUL)) && (*--e != '\n'))
      (void) fputc('\n', stderr);
   exit(1);
}

/**
 ** Massages the return value from wait/system/pclose through the
 ** standard macros. This is needed to prevent wraparaound
 ** (exit status 256 becomes 0).
 **/
static int
_cp_retcode(int code)
{
   if (WIFEXITED(code))
      return WEXITSTATUS(code);
   else if (WIFSIGNALED(code))
      return WTERMSIG(code);
   else if (WIFSTOPPED(code))
      return WSTOPSIG(code);
   else
      return code;
}

/**
 ** This is like sprintf() except, instead of taking a char buffer as an
 ** argument, it mallocs and returns a buffer containing the result.
 **/
static char *
_cp_newstr(const char *fmt,...)
{
   va_list ap;
   char *ptr;
   char buf[BIGLINE];

   va_start(ap, fmt);
   (void) vsprintf(buf, fmt, ap);
   va_end(ap);
   if ((ptr = malloc(strlen(buf) + 1)) == NULL) {
      perror("malloc");
      exit(1);
   }
   return strcpy(ptr, buf);
}

/** Obsolete but left for reasons of laziness - the old
 ** SWIG-generated XS code thinks they still exist.
 **/
char *child_get_stdout_perl(CHILD *handle) {return NULL;}
char *child_get_stderr_perl(CHILD *handle) {return NULL;}
char *child_gets(char *s, int n, CHILD *handle) {return NULL;}

/**
 ** We did a "lazy fork" when opening the coprocess ... now it's been
 ** sent a command and we actually have to run the program.
 **/
static int
_cp_start_child(CHILD *handle)
{
   pid_t pid;
   int down_pipe[2], back_pipe[2], err_pipe[2];
   FILE *downfp, *backfp, *errfp;

   if (pipe(down_pipe) < 0)
      _cp_syserr("down_pipe");
   if (pipe(back_pipe) < 0)
      _cp_syserr("back_pipe");
   if (pipe(err_pipe) < 0)
      _cp_syserr("err_pipe");

   if ((pid = fork()) < 0)
      _cp_syserr("fork");
   else if (pid > 0) {
      _dbg(F,L,2, "starting child %s (pid=%d) ...", handle->cph_cmd, pid);
      (void) close(down_pipe[0]);
      if ((downfp = fdopen(down_pipe[1], "w")) == NULL)
	 _cp_syserr("fdopen");
      if (setvbuf(downfp, NULL, _IONBF, 0) != 0)
	 _cp_syserr("setvbuf");

      (void) close(back_pipe[1]);
      if ((backfp = fdopen(back_pipe[0], "r")) == NULL)
	 _cp_syserr("fdopen");
      if (setvbuf(backfp, NULL, _IONBF, 0) != 0)
	 _cp_syserr("setvbuf");

      (void) close(err_pipe[1]);
      if ((errfp = fdopen(err_pipe[0], "r")) == NULL)
	 _cp_syserr("fdopen");

      /** Fill in the remaining fields of the CHILD handle **/
      handle->cph_pid = pid;
      handle->cph_down = downfp;
      handle->cph_back = backfp;
      handle->cph_err = errfp;

      /*
       * Make sure the usual "slow system call" suspects (see sigaction(2))
       * are restarted on (at least) a Ctrl-C.
       */
      reliable_signal(SIGINT, SIG_DFL);

      return 0;
   } else {
      /** Attach stdin to the input pipe **/
      (void) close(down_pipe[1]);
      if (down_pipe[0] != STDIN_FILENO) {
	 if (dup2(down_pipe[0], STDIN_FILENO) != STDIN_FILENO)
	    _cp_syserr("dup2");
      }

      /** Attach stdout to the output pipe **/
      (void) close(back_pipe[0]);
      if (back_pipe[1] != STDOUT_FILENO) {
	 if (dup2(back_pipe[1], STDOUT_FILENO) != STDOUT_FILENO)
	    _cp_syserr("dup2");
      }

      /** Attach stderr to one end of the error pipe **/
      (void) close(err_pipe[0]);
      if (err_pipe[1] != STDERR_FILENO) {
	 if (dup2(err_pipe[1], STDERR_FILENO) != STDERR_FILENO)
	    _cp_syserr("dup2");
	 (void) close(err_pipe[1]);
      }

      /**
       ** Put the child in its own session. This is done to
       ** keep signals sent to the parent from going to the child.
       ** For instance, if the user wants to quit a program which
       ** is running a coprocess by sending it a SIGINT, the parent
       ** may choose to handle SIGINT and trap to a cleanup routine,
       ** which may need the child process to help with the cleanup.
       ** Thus we can't let the child die before the parent. This is
       ** no big deal, because if the parent catches a signal and
       ** just dies (e.g. the SIG_DFL case), the child will exit
       ** as soon as it reads EOF from stdin anyway.
       **/
      if (setsid() == -1) {
	 _cp_syserr("setsid");
      }

      (void) execlp(handle->cph_cmd, handle->cph_cmd, NULL);
      _cp_syserr(handle->cph_cmd);
      _exit(127);
   }

   /*NOTREACHED*/
   return -1;
}

/*
 * Starts the specified prog as a coprocess. Returns a single file pointer
 * which handles both input to and output from the process. Also returns
 * another file ptr via the parameter list which can be polled for error
 * output.
 * The last parameter is the command to send to the process
 * which causes it to finish, typically "exit" or similar.
 ** We defer the actual fork/exec sequence till the first
 ** command is sent.
 */
CHILD *
child_open(char *cmd, char *tag, char *eot, char *quit)
{
   CHILD *handle;

   if ((handle = mru_handle = malloc(sizeof(CHILD))) == NULL) {
      perror("malloc");
      exit(1);
   }
   (void) memset(handle, 0, sizeof(CHILD));
   handle->cph_cmd  = _cp_newstr(cmd);
   handle->cph_tag  = _cp_newstr("%s\n", tag);
   handle->cph_eot  = _cp_newstr("%s\n", eot);
   if (quit && *quit)
       handle->cph_quit = _cp_newstr("%s\n", quit);

   return handle;
}

int bck_read( void* handle, char* buf, int len ){
  char *eot = ((CHILD*)handle)->cph_eot;
  int eot_len = strlen(eot);
  if( len ){
    if( !strncmp( buf, eot, len ) ){
      _dbg(F,L,3, "logical end of stdin from %s", ((CHILD*)handle)->cph_cmd );
      return NPOLL_RET_IDLE;
    } else if (!strncmp(eot, buf+len-eot_len, eot_len)) {
      len -= eot_len;
      _dbg(F,L,3, 
	    "unterminated end of stdin from %s", ((CHILD*)handle)->cph_cmd );
      _dbg(F,L,2, "<<-- %.*s", len, buf);
      Perl_av_push( aTHX_ ((CHILD*)handle)->cph_out_array , Perl_newSVpv( aTHX_ buf, len ) );
      return NPOLL_RET_IDLE;
    } else {
      _dbg(F,L,2, "<<-- %.*s", len, buf);
      Perl_av_push( aTHX_ ((CHILD*)handle)->cph_out_array , Perl_newSVpv( aTHX_ buf, len ) );
      return NPOLL_CONTINUE;
    }
  } else {
    _dbg(F,L,3, "eof on stdin from %s", ((CHILD*)handle)->cph_cmd );
    return NPOLL_RET_IDLE;
  }
}

int err_read( void* handle, char* buf, int len ){
  if( len ){
    /*
     * Interrupt handling doesn't work quite right. As yet undiagnosed
     * but this code is left in because it's no worse than not having it.
     */
    if( !strncmp( buf, "Interrupt", 9 ) ){
      _dbg(F,L,3, "interrupted end of cmd from %s", ((CHILD*)handle)->cph_cmd );
      return NPOLL_RET_IDLE;
    } else {
      _dbg(F,L,2, "<<== '%.*s'", len, buf);
      Perl_av_push( aTHX_ ((CHILD*)handle)->cph_err_array , Perl_newSVpv( aTHX_ buf, len ) );
      return NPOLL_CONTINUE;
    }
  } else {
    return NPOLL_RET_IDLE;
  }
}

/**
 ** Analogue: fputs().
 ** Sends two commands to the child: first the "real command", then the
 ** "tag (trivial) command".
 ** Uses a "lazy fork" concept; we don't actually start the child
 ** at the time of the child_open() call but instead when the first cmd
 ** is sent. This means a program which employs a dispatcher, for instance,
 ** can do a child_open() in the main function, pass the handle off to
 ** all the routines it dispatches to, and child_close() before exiting
 ** without worrying about which of those functions will actually need
 ** the coprocess.  If some of them don't need it, they won't send it
 ** any commands and will incur no fork/exec overhead.
 **/
int
child_puts(char *s, CHILD *handle, AV* perl_out_array, AV* perl_err_array)
{
   int n;

   if (handle == NULL)
      handle = mru_handle;
   if ((mru_handle = handle) == NULL)
      return 0;
   if (handle->cph_pid == 0) {
      if (_cp_start_child(handle) != 0) {
	 fprintf(stderr, "can't start child %s\n",  handle->cph_cmd);
	 exit(1);
      }
      /* Add file descriptors to poll vector. */
      poll_add_fd( fileno(handle->cph_back),
                   NPOLL_TXT, bck_read, NULL, handle );
      poll_add_fd( fileno(handle->cph_err),
                   NPOLL_TXT, err_read, NULL, handle );
   }

   /* Save Perl array references for access from callbacks. */
   handle->cph_out_array = perl_out_array;
   handle->cph_err_array = perl_err_array;

   _dbg(F,L,1, "-->> %s", s);

   /** set error count to 0 for new command **/
   handle->cph_errs = 0;

   /** send the cmd down the pipe to the child **/
   if ((n = fputs(s, handle->cph_down)) == EOF)
      return EOF;
   /** be helpful and add a newline if there isn't one **/
   if (strrchr(s, '\n') != endof(s) - 1)
      if (fputc('\n', handle->cph_down) == EOF)
	 return EOF;
   /** send the tag cmd **/
   _dbg(F,L,4, "-->> [TAG]");
   if (fputs(handle->cph_tag, handle->cph_down) == EOF)
      return EOF;
   handle->cph_pending = TRUE;
   _dbg(F,L,4, "pending ...");

   /** poll **/
   poll_rcv( NPOLL_NO_TIMEOUT ); 

   return n;
}

/**
 ** Call this when a subcommand is finished.
 **/
int
child_end(CHILD *handle, int show_err)
{
   return handle->cph_errs;
}

/**
 ** Send a signal to the coprocess.  Remember, 'kill' is a misnomer;
 ** it's just a signal.
 **/
int
child_kill(CHILD *handle, int signo)
{
   UPDATE_HANDLE(handle, mru_handle, 0);
   _dbg(F,L,4, "sending signal %d to pid: %d", signo, handle->cph_pid);
   return kill(handle->cph_pid, signo);
}

/**
 ** Call this to end the coprocess.  Returns the exit code of the child.
 **/
int
child_close(CHILD *handle)
{
   int retstat = 1, done;

   /**
    ** This lets us close the most-recently-used coprocess gracefully without
    ** having a handle.  Useful from registered signal/atexit handlers.
    **/
   if (handle == NULL)
      handle = mru_handle;

   if ((mru_handle = handle) == NULL)
      return -1;

   /** If there's no child running, we're done **/
   if (handle->cph_pid == 0)
      return 0;

   /** ... likewise for the stderr **/
   (void) child_end(handle, CP_SHOW_ERR);

   /** provide a high-level dbg msg */
   _dbg(F,L,2, "ending child %s (pid=%d) ...", handle->cph_cmd,handle->cph_pid);

   /** Optionally, tell the child explicitly to give up the ghost */
   if (handle->cph_quit && *(handle->cph_quit)) {
      _dbg(F,L,4, "sending to pid %d: %s",
		      handle->cph_pid, handle->cph_quit);
      (void) fputs(handle->cph_quit, handle->cph_down);
   }

   /** Remove the back and err file descriptors from npoll **/
   poll_del_fd( fileno(handle->cph_back) );
   poll_del_fd( fileno(handle->cph_err) );

   /** Close the input pipe to the child, which should die gracefully. **/
   if (fclose(handle->cph_down) == EOF
	 || fclose(handle->cph_back) == EOF
	 || fclose(handle->cph_err) == EOF)
      return -1;

   /** Reap the child. **/
   while ((done = waitpid( handle->cph_pid, &retstat, WNOHANG)) <= 0)
      if (done < 0 && errno != EINTR)
	 return -1;

   _dbg(F,L,3, "ended child %s (%d) d=%d r=%d",
        handle->cph_cmd, handle->cph_pid, done, retstat );

   if (handle != NULL) {
      if (handle->cph_cmd)
	  free(handle->cph_cmd);
      if (handle->cph_tag)
	  free(handle->cph_tag);
      if (handle->cph_eot)
	  free(handle->cph_eot);
      if (handle->cph_quit)
	  free(handle->cph_quit);
      free(handle);
   }
   mru_handle = NULL;

   return _cp_retcode(retstat);
}
