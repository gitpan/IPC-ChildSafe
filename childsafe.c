#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>

#include "childsafe.h"

/**
 ****************************************************************************
 ** Copyright (c) 1997 David Boyce (dsb@world.std.com). All rights reserved.
 ** This program is free software; you can redistribute it and/or
 ** modify it under the same terms as Perl itself.
 ****************************************************************************
 ** To understand the C code and underlying design, look at
 ** W.R. Stevens (Advanced Programming in the Unix Environment)
 ** chapters 14 and 15. Also chapter 19 to get into how it interacts
 ** with coprocesses using ptys. This code goes way beyond any discussions
 ** of coprocesses in Stevens but he does a good job describing the
 ** issues and defining terms. Also look at the comments in
 ** IPC::Open3.pm in the Perl5 distribution. Then look at ChildSafe.html
 ** (part of this distribution).
 ** Some of the code here was cut-and-pasted directly from the
 ** examples given by Stevens (ISBN 0-201-56317-7).
 **/

/** Used for debugging output **/
#define F __FILE__
#define L __LINE__

/** Conveniences **/
#define TRUE 1
#define FALSE 0
#define NUL '\0'

/** Ok, these should be dynamic allocations ... maybe someday **/
#define STDLINE 1024
#define BIGLINE	16384

/** Print debugging messages that assert this level (0=off, 1=low, ...) **/
int Debug_Level = 0;

/** Poll stderr every Alarm_Wait seconds while blocked on read **/
int Alarm_Wait = 5;

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
   char buf[STDLINE], *e;

   if (Debug_Level < lvl)
      return;

   (void) fprintf(stderr, "+ %s:%d ", ((e=strrchr(f,'/'))?e+1:f), l);
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
#if defined(ARG_MAX)
   char buf[ARG_MAX];
#else
   char buf[_POSIX_ARG_MAX];
#endif

   va_start(ap, fmt);
   (void) vsprintf(buf, fmt, ap);
   va_end(ap);
   if ((ptr = malloc(strlen(buf) + 1)) == NULL) {
      perror("malloc");
      exit(1);
   }
   return strcpy(ptr, buf);
}

/**
 ** Sets non-blocking mode on the specified file descriptor.
 **/
static int
_cp_nonblocking(int fd)
{
   int old = fcntl(fd, F_GETFL, 0);
   if (fcntl(fd, F_SETFL, old | O_NONBLOCK) < 0)
      _cp_syserr("fcntl:F_SETFL");
   return old;
}

static int
_cp_poll_stderr(CHILD *handle, int show)
{
   char line[STDLINE];

   UPDATE_HANDLE(handle, mru_handle, 0);

   _dbg(F,L,4, "polling standard error ...");
   while (fgets(line, sizeof(line), handle->cph_err) != NULL) {
      if (show != CP_NO_SHOW_ERR)
	 (void) fputs(line, stderr);
      else
	 _dbg(F,L,2, "<<== (NO_SHOW) %s", line);

      /**
       ** Lines that look like warnings or shell-generated verbose output
       ** are ignored; anything we don't recognize is considered an
       ** error.
       **/
      if ((line[0] == '+' && line[1] == ' ') || strstr(line, "arning: "))
	 continue;

      /** Looks like an error **/
      handle->cph_errs++;
   }

   return handle->cph_errs;
}

/*
 * See comment for child_gets_stdout_perl.
 */
char *
child_get_stderr_perl(CHILD *handle)
{
   char buf[STDLINE];

   UPDATE_HANDLE(handle, mru_handle, 0);

   if (fgets(buf, sizeof(buf), handle->cph_err) == NULL)
      return NULL;
   _dbg(F,L,2, "<<== %s", buf);
   return _cp_newstr("%s", buf);
}

/**
 ** It's possible for the parent to block for a long time while
 ** reading from the child, because the child is hung on some other event.
 ** The classic example is the child needs a resource from an NFS server
 ** which is down.  In this situation, hanging until the server returns
 ** is the right thing to do; the problem is that there's a message
 ** "NFS server foo not responding, still trying" waiting on the
 ** stderr pipe and it's not being seen by the user, which can lead
 ** them to wonder what's up.  So we set this SIGALRM handler, which
 ** periodically polls the child's stderr, transfers any output it finds
 ** there to parent's stderr, and re-starts the blocked read.
 */
static void
_cp_flush_stderr(int signo)
{
   if (mru_handle) {
      if (signo == SIGALRM)
	 _dbg(F,L,3, "flushing stderr pipe on timeout ...");
      else if (signo == -1)
	 _dbg(F,L,3, "flushing stderr pipe on async exit ...");
      else
	 _dbg(F,L,3, "flushing stderr pipe on signal %d ...", signo);
      (void) _cp_poll_stderr(mru_handle, CP_SHOW_ERR);
      /** Restart the alarm clock **/
      if (signo == SIGALRM)
	 (void) alarm(Alarm_Wait);
   }
}

/**
 ** This is intended to catch cases where the program calls exit()
 ** without having first flushed the stderr pipe via child_end().
 ** This uses the signal handler though there's no actual signal
 ** associated.  Just like pseudo-signals in ksh such as trap on EXIT.
 */
static void
_cp_flush_stderr_at_exit(void)
{
   _cp_flush_stderr(-1);
}

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

   _dbg(F,L,1, "starting child %s ...", handle->cph_cmd);

   if (pipe(down_pipe) < 0)
      _cp_syserr("down_pipe");
   if (pipe(back_pipe) < 0)
      _cp_syserr("back_pipe");
   if (pipe(err_pipe) < 0)
      _cp_syserr("err_pipe");

   if ((pid = fork()) < 0)
      _cp_syserr("fork");
   else if (pid > 0) {
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
      (void) _cp_nonblocking(err_pipe[0]);
      if ((errfp = fdopen(err_pipe[0], "r")) == NULL)
	 _cp_syserr("fdopen");

      /** Fill in the remaining fields of the CHILD handle **/
      handle->cph_pid = pid;
      handle->cph_down = downfp;
      handle->cph_back = backfp;
      handle->cph_err = errfp;

      /**
       ** Put the child in its own process group. This is done to
       ** keep signals sent to the parent from going to the child.
       ** For instance, if the user wants to quit a program which
       ** is running a coprocess by sending it a SIGINT, the parent
       ** may need to (a) handle SIGINT and trap to a cleanup routine,
       ** which may need the child process to help with the cleanup.
       ** We can't let the child die before the parent.  This is
       ** no big deal, because if the parent catches a signal and
       ** just dies (e.g. the SIG_DFL case), the child will exit
       ** as soon as it reads EOF from stdin anyway.
       ** Note that both parent and child sides of the fork
       **/
      (void) setpgid(pid, pid);

      /**
       ** Register an exit handler to make sure we read any waiting
       ** stderr data before exiting.
       **/
      (void) atexit(_cp_flush_stderr_at_exit);

      /** See comments on SIGALRM in _cp_flush_stderr() and _child_gets() **/
      {
	 struct sigaction act, oact;
	 act.sa_handler = _cp_flush_stderr;
	 if (sigemptyset(&act.sa_mask))
	    _cp_syserr("sigemptyset");
	 act.sa_flags = 0;
#ifdef	SA_RESTART
	 act.sa_flags |= SA_RESTART;	/* SVR4, 44BSD */
#endif
	 if (sigaction(SIGALRM, &act, &oact) < 0)
	    _cp_syserr("sigaction");
      }

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

      /** We want to be in our own process group.  The parent may
       ** already have done this.
       **/
      (void) setpgid(0, 0);

      (void) execlp(handle->cph_cmd, handle->cph_cmd, NULL);
      _cp_syserr(handle->cph_cmd);
      _exit(127);
   }

   /*NOTREACHED*/
   return -1;
}

/**
 ** This is how we "seek forward" to the <RET> token if there was
 ** any unused data left in the stdout pipe. This needs to be done
 ** to stay in sync and also to avoid any SIGPIPEs.
 **/
static void
_cp_sync(CHILD *handle)
{
   char no_show[STDLINE];

   /** Get rid of any unused remaining output **/
   if (handle->cph_pending == TRUE)
      while (child_gets(no_show, sizeof(no_show), handle) != NULL)
	 _dbg(F,L,2, "<<-- (NO_SHOW) %s", no_show);
}

/*
 * Starts the specified prog as a coprocess. Returns a single file pointer
 * which handles both input to and output from the process. Also returns
 * another file ptr via the parameter list which can be polled for error
 * output.
 ** We defer the actual fork/exec sequence till the first
 ** command is sent.
 */
CHILD *
child_open(char *cmd, char *tag, char *eot)
{
   CHILD *handle;

   if ((handle = mru_handle = malloc(sizeof(CHILD))) == NULL) {
      perror("malloc");
      exit(1);
   }
   (void) memset(handle, 0, sizeof(CHILD));
   handle->cph_cmd = _cp_newstr(cmd);
   handle->cph_tag = _cp_newstr("%s\n", tag);
   handle->cph_eot = _cp_newstr("%s\n", eot);

   return handle;
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
child_puts(char *s, CHILD *handle)
{
   int n;

   if (handle == NULL) handle = mru_handle;
   if ((mru_handle = handle) == NULL) return 0;
   if (handle->cph_pid == 0) {
      if (_cp_start_child(handle) != 0) {
	 fprintf(stderr, "can't start child %s\n",  handle->cph_cmd);
	 exit(1);
      }
   }

   /** throw away any remaining output from the previous command **/
   _cp_sync(handle); 

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
   return n;
}

/**
 ** Analogue: fgets().
 ** Read back output from the current coprocess cmd until the special
 ** output of the tag cmd (aka EOT) is seen. When we read this line,
 ** we throw it away and return NULL as if we had seen EOF on the
 ** pipe.
 ** If passed a buffer, use it. If passed NULL, malloc a new buffer.
 **/
char *
child_gets(char *s, int n, CHILD *handle)
{
   static char buf[BIGLINE];

   UPDATE_HANDLE(handle, mru_handle, NULL);

   /** If not passed a buffer, use a malloc-ed buf instead (perl API hack) **/
   if (s == NULL)
      n = sizeof(buf);

   /**
    ** Do the read from the child, which may block.  We wrap it
    ** in an alarm handler so we can flush any pending stderr.
    ** This will happen in situations where the child blocks for
    ** a long time with a warning, such as when an NFS server is
    ** down. We automatically restart the read after the SIGALRM.
    ** Note: if Alarm_Wait is set to 0, this feature will be disabled.
    */
   if (Alarm_Wait)
      (void) alarm(Alarm_Wait);
   if (fgets(buf, n, handle->cph_back) == NULL)
      return NULL;
   if (Alarm_Wait)
      (void) alarm(0);

   if (!strcmp(buf, handle->cph_eot)) {
      if (handle->cph_pending == TRUE) {
	 handle->cph_pending = FALSE;
	 _dbg(F,L,4, "<<-- [RET]");
	 buf[0] = NUL;
	 return NULL;
      } else {
	 fprintf(stderr, "sync error - output found while not pending\n");
      }
   }

   _dbg(F,L,2, "<<-- %s", buf);
   return s ? strcpy(s, buf) : _cp_newstr("%s", buf);
}

/**
 ** Hack for Perl/SWIG: couldn't seem to figure out how to pass a
 ** NULL ptr from Perl to child_gets(), so we use this intermediary
 ** function. The malloc-ed line is freed in SWIG's wrapper via %new.
 **/
char *
child_get_stdout_perl(CHILD *handle)
{
   return child_gets(NULL, 0, handle);
}

/**
 ** We call this when a subcommand is finished, or at least we've
 ** gotten all the output from it we need ... this flushes anything
 ** remaining on the stdout pipe and polls stderr for possible
 ** error messages.
 **/
int
child_end(CHILD *handle, int show_err)
{
   int retcode;

   UPDATE_HANDLE(handle, mru_handle, 0);

   _cp_sync(handle);

   return _cp_poll_stderr(handle, show_err);
}

/**
 ** Send a signal to the coprocess.  Remember, 'kill' is a misnomer.
 ** It's just a signal.
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

   /** ensure there's no output remaining on the stdout pipe ... **/
   _cp_sync(handle);

   /** ... likewise for the stderr **/
   (void) child_end(handle, CP_SHOW_ERR);

   _dbg(F,L,1, "closing child %s (%d)", handle->cph_cmd, handle->cph_pid);

   /** Close the input pipe to the child;, which should die gracefully. **/
   if (fclose(handle->cph_down) == EOF
	 || fclose(handle->cph_back) == EOF
	 || fclose(handle->cph_err) == EOF)
      return -1;

   /** Reap the child. **/
   while ((done = waitpid(handle->cph_pid, &retstat, WNOHANG)) <= 0)
      if (done < 0 && errno != EINTR)
	 return -1;

   if (handle != NULL)
      free(handle);
   mru_handle = NULL;

   return _cp_retcode(retstat);
}
