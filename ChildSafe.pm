package IPC::ChildSafe;

require 5.004;

use strict;
use vars qw($VERSION @ISA @EXPORT_OK);
use Carp;

use constant	NOTIFY => 0;	# default - print stderr msgs as they arrive
use constant	STORE  => 1;	# store stderr msgs for later retrieval
use constant	PRINT  => 2;	# send all output to screen immediately
use constant	IGNORE => 3;	# throw away all output, ignore retcode

# This is retained for backward compatibility. Modern uses should
# employ the methods of the same name, e.g. $obj->store(1);
@EXPORT_OK = qw(NOTIFY STORE PRINT IGNORE);

require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);

# SWIG-generated XS code.
bootstrap IPC::ChildSafe;
var_ChildSafe_init();

# The current version and a way to access it.
$VERSION = "2.32"; sub version {$VERSION}

########################################################################
# Just a thin layer over child_open (see childsafe.c). Optional last
# argument is a function to examine stderr output and determine
# whether it constitutes an error.  If no REF CODE is supplied for $chk,
# we use a default internal subroutine.
########################################################################
sub new {
   my $proto = shift;
   my $class = ref($proto) || $proto;
   my($cmd, $tag, $ret, $mode, $chk) = @_;

   # Hack for compatibility with <= 2.30
   if ($mode && ref($mode)) {
      $chk = $mode;
      undef $mode;
   }

   # Initialize the hash which will represent this object.
   my $self  = {
      _CHILD	=> undef,
      _ERRCHK	=> undef,
      _MODE	=> $mode || NOTIFY,
      _STDOUT	=> undef,
      _STDERR	=> undef,
   };

   if ($chk) {
      $self->{_ERRCHK} = $chk;
   } else {
      $self->{_ERRCHK} = sub
	 {
	    my($r_stderr, $r_stdout) = @_;
	    grep(!/^\+\s|warning:/i, @$r_stderr);
	 };
   }

   $self->{_CHILD} = \child_open($cmd, $tag, $ret);
   @{$self->{_STDOUT}} = ();
   @{$self->{_STDERR}} = ();

   bless ($self, $class);
   return $self;
}

sub cmd {
   my $self = shift;
   my($cmd, $mode, @junk) = @_;

   croak "extraneous data '@junk' follows command" if @junk;

   # If used in void context with no args, throw away all stored output.
   if (! $cmd) {
      croak "must provide a command line" if defined wantarray;
      $self->{_STDOUT} = [];
      $self->{_STDERR} = [];
      return;
   }

   # Send the command down to the child.
   child_puts($cmd, ${$self->{_CHILD}});

   # Read back lines from stdout and stderr pipes and push them
   # onto the appropriate arrays.
   my $line;
   push(@{$self->{_STDOUT}}, $line)
	 while $line = child_get_stdout_perl(${$self->{_CHILD}});
   push(@{$self->{_STDERR}}, $line)
	 while $line = child_get_stderr_perl(${$self->{_CHILD}});

   # This line should be self-documenting ... well, ok, we're
   # passing references to the stdout and stderr arrays into the
   # currently-registered discriminator function. The return value
   # is the number of errors it determined by examining
   # the output. Typically the discriminator only cares about the
   # stderr stream but we pass it stdout also in case it matters.
   my $errcnt = int &{$self->{_ERRCHK}}(\@{$self->{_STDERR}},
					   \@{$self->{_STDOUT}});

   # Now return different things depending on the context this
   # method was used in - see comment above for details.
   if (wantarray) {
      my $r_results = {
	 'stdout' => $self->{_STDOUT},
	 'stderr' => $self->{_STDERR},
	 'status' => $errcnt,
      };
      $self->{_STDOUT} = [];
      $self->{_STDERR} = [];
      return %$r_results;
   } else {
      $mode ||= $self->{_MODE};
      if ($mode == NOTIFY) {
	 $self->stderr;
      } elsif ($mode == PRINT) {
	 $self->stdout;
	 $self->stderr;
      } elsif ($mode == IGNORE) {
	 $self->{_STDOUT} = [];
	 $self->{_STDERR} = [];
      }
      exit $errcnt if !defined(wantarray) && $errcnt && ($mode != IGNORE);
      return $errcnt;
   }
}

# Backward compatibility. Undocumented.
*command = *cmd;

# Auto-generate methods to set the output-handling mode.
for my $mode (qw(NOTIFY STORE PRINT IGNORE)) {
   my $method = lc $mode;
   no strict 'refs';
   *$method = sub { $_[0]->{_MODE} = &$mode };
}

sub status {
   my $self = shift;
   return int &{$self->{_ERRCHK}}(\@{$self->{_STDERR}},
				  \@{$self->{_STDOUT}});
}

sub stdout {
   my $self = shift;
   return 0 unless $self->{_STDOUT};
   if (wantarray) {
      my @out = @{$self->{_STDOUT}};
      $self->{_STDOUT} = [];
      return @out;
   } elsif (defined(wantarray)) {
      return @{$self->{_STDOUT}};
   } else {
      print STDOUT @{$self->{_STDOUT}};
      $self->{_STDOUT} = [];
   }
}

sub stderr {
   my $self = shift;
   return 0 unless $self->{_STDERR};
   if (wantarray) {
      my @errs = @{$self->{_STDERR}};
      $self->{_STDERR} = [];
      return @errs;
   } elsif (defined(wantarray)) {
      return @{$self->{_STDERR}};
   } else {
      print STDERR @{$self->{_STDERR}};
      $self->{_STDERR} = [];
   }
}

########################################################################
# This function takes a reference to an error-checking subroutine and
# registers it to handle subsequent stderr output. It returns a
# reference to the superseded discriminator function.
## Note - this has never actually been tested, I just put it here in
## case it's needed someday ....
########################################################################
sub errchk {
   my $self = shift;
   my $old_errchk = $self->{_ERRCHK};
   $self->{_ERRCHK} = shift if @_;
   return $old_errchk;
}

sub finish {
   my $self = shift;
   child_close(${$self->{_CHILD}});
   undef $self->{_CHILD};
}

sub DESTROY {
   my $self = shift;
   $self->finish if $self->{_CHILD};
}

########################################################################
# Set or change the current debugging level.  Each level implies those lower:
#   0 == no debugging output
#   1 == programmer-defined temporary debug output
#   2 == commands sent to the child process
#   3 == data returned from the child process
#   4 == all meta-data exchanged by parent and child: tag, ret, polling, etc.
# Other debug levels are unassigned and available for user definition.
## With no args, this method toggles state between no-debug and max-debug.
### Debug_Level is resident in the C code.
########################################################################
sub dbglevel {
   my $self = shift;
   if (@_) {
      $IPC::ChildSafe::Debug_Level = shift;
   } else {
      $IPC::ChildSafe::Debug_Level = $IPC::ChildSafe::Debug_Level ? 0 : -1>>1;
   }
}

# Backward compatibility.
*debug = *dbglevel;

1;

__END__

=head1 NAME

IPC::ChildSafe, ChildSafe - control a child process without blocking

=head1 SYNOPSIS

   use IPC::ChildSafe;

   # Start a shell process (create a new shell object).
   $SH = IPC::ChildSafe->new('sh', 'echo ++EOT++', '++EOT++');

   # If the ls command succeeds, read lines from its stdout one at a time.
   if ($SH->cmd('ls') == 0) {
      print "Found ", scalar($SH->stdout), " files in current dir ...\n";

      # Another ls cmd - results added to the object's internal stack
      $SH->cmd('ls /tmp');

      # Since we're stuck in this dumb example, let's get the date too.
      $SH->cmd('date');

      # Now dump results to stdout - show how to get 1 line at a time
      for my $line ($SH->stdout) {
	 print $line;
      }

      # You could also print the output this way:
      # print $SH->stdout;

      # Or even just:
      # $SH->stdout;

   }

   # Send it a command, read back the stdout/stderr/return code
   # into a hash array.
   my(%results) = $SH->cmd('id');		# Send an 'id' cmd
   die if $results{status};			# Expect no errors
   die if @{$results{stdout}} != 1;		# Should be just 1 line
   die if $results{stdout}[0] !~ /^uid=/;	# Check output line

   # (lather, rinse, repeat)

   # Finishing up.
   die if $SH->finish;				# Returns final status

=head1 DESCRIPTION

   This was written to address the "blocking problem" inherent in
most coprocessing designs such as IPC::Open3.pm, which has warnings
such as this in its documentation:

    ... additionally, this is very dangerous as you may block forever.  It
   assumes it's going to talk to something like bc, both writing to it and
   reading from it.  This is presumably safe because you "know" that
   commands like bc will read a line at a time and output a line at a
   time ...

or IPC::Open2 which has this warning from its author (Tom Christansen):

   ... I strongly advise against using open2 for almost anything, even
   though I'm its author. UNIX buffering will just drive you up the wall.
   You'll end up quite disappointed ...

The blocking problem is: once you've sent a command to your coprocess,
how do you know when the output resulting from this command has
finished?  If you guess wrong and issue one read too many you can
deadlock forever.  This implementation solves the problem, at least for
a subset of possible child programs, by using a little trick:  it sends
a 2nd (trivial) command down the pipe right in back of every real
command.  When we see the the output of this special command in the
return pipe, we know the real command is done.

This module also returns an "exit status" for each command, which is
really a count of the error messages produced by it.  The programmer
can optionally register his/her own discriminator function for
determining which output to stderr constitutes an error message.

=head1 CONSTRUCTOR

The constructor takes 3 arguments plus an optional 4th and 5th: the 1st
is the program to run, the 2nd is a command to that program which
produces a unique one-line output, and the 3rd is that unique output.
If a 4th arg is supplied it becomes the mode in which this object will
run (default: NOTIFY, see below), and if a 5th is given it must be a
code ref, which will be registered as the error discriminator.  If no
discriminator is supplied then a standard internal one is used.

The 2nd arg is called the "tag command". Preferably this would be
something lightweight, e.g. a shell builtin. Unfortunately the current
version has no support for a multi-line return value since it would
require some fairly complex buffering.

=head1 DISCRIMINATOR

The discriminator function is invoked after each command completes, and
is passed a reference to an array containing the stderr generated by
that command in its first parameter. A pointer to the stdout is
similarly supplied in the second param. Normally this function would
just apply a regular expression to one or both of these and indicate by
its return status whether it considers this to constitute an error
condition. E.g. the version provided internally is:

    sub errors {
	my($r_stderr, $r_stdout) = @_;
	grep(!/^\+\s|warning:/i, @$r_stderr);
    }

    my $sh = IPC::ChildSafe->new('sh', 'echo ++EOT++', '++EOT++', \&errors);

which treats ANY output to stderr as indicative of an error, with the
exception of lines beginning with "+ " (shell verbosity) or containing
the string "warning:".

=head1 METHODS

=over 4

=item * B<notify/store/print/ignore>

Sets the output-handling mode. The meanings of these are described
below.

=item * B<cmd>

Send specified command to child process. Return behavior varies
with context:

=over 4

=item array context

returns a hash containing an array of stdout results (key: 'stdout'),
an array of stderr messages ('stderr), and the "return code" of the
command ('status').

=item scalar context

returns command's "return code".  In the default mode (NOTIFY), sends
stderr results directly to parent's stderr while storing stdout in the
object for later retrieval via I<stdout> method. In PRINT mode both
stdout and stderr are sent directly to the "real" (parent's)
stdout/stderr. STORE mode causes both stdout and stderr to be stored
for later use, while IGNORE mode throws away both.

=item void context

similar to scalar mode but exits on nonzero return code unless in
IGNORE mode.

=item void context and no args

clears the stdout and stderr buffers

=back

=item * B<stdout>

Return stored output from previous command(s). Behavior varies with
context:

=over 4

=item array context

shifts all stored lines off the stdout stack and returns them in a list.

=item scalar context

returns the number of lines currently stored in the stdout stack.

=item void context

prints the current stdout stack to actual stdout.

=back

=item * B<stderr>

Similar to C<stdout> method above. Note that by default stderr does
not go to the accumulator, but rather to the parent's stderr.  Set the
STORE attribute to leave stderr in the accumulator instead where this
method can operate on it.

=item * B<status>

Pass the current stdout and stderr buffers to the currently-registered
error discriminator and return its results (aka the error count).

=item * B<finish>

Ends the child process and returns its final exit status.

=back

=head1 AUTHOR

David Boyce dsb@world.std.com

=head1 SEE ALSO

perl(1), "perldoc IPC::Open3", _Advanced Programming in the Unix Environment_
by W. R. Stevens

=cut
