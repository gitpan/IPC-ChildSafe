package IPC::ChildSafe;

require 5.004;

use strict;
use vars qw($VERSION @ISA @EXPORT @EXPORT_OK);
use Carp;

use constant	NOTIFY => 0;	# default - print stderr msgs as they arrive
use constant	STORE  => 1;	# store stderr msgs for later retrieval
use constant	PRINT  => 2;	# send all output to screen immediately
use constant	IGNORE => 3;	# throw away all output, ignore retcode

@EXPORT_OK = qw(NOTIFY STORE PRINT IGNORE);

# SWIG-generated code.
require Exporter;
require DynaLoader;
@ISA = qw(Exporter DynaLoader);
bootstrap IPC::ChildSafe;
var_ChildSafe_init();

# The current version and a way to access it.
$VERSION = "2.27"; sub version {$VERSION}

########################################################################
# Just a thin layer over child_open (see child.c). Optional last
# argument is a function to examine stderr output and determine
# whether it constitutes an error.  If no REF CODE is supplied for $chk,
# we use a default internal subroutine.
########################################################################
sub new
{
   my $proto = shift;
   my $class = ref($proto) || $proto;
   my($cmd, $tag, $ret, $chk) = @_;

   # Initialize the hash which will represent this object.
   my $self  = {
      _CHILD	=> undef,
      _ERRCHK	=> undef,
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

########################################################################
# Send specified command to child process. Return behavior varies
# with context:
#   Array context: returns a hash containing an array of stdout
#     results (key: 'stdout'), an array of stderr messages ('stderr),
#     and the "return code" of the command ('status').
#   Scalar context: returns command's "return code", stores stdout
#     in object for later access via method 'stdout'. By default,
#     sends stderr results directly to parent's stderr but if '=> STORE'
#     is passed, stderr is stored just like stdout. With '=> IGNORE',
#     throws away both stdout and stderr.
#   Void context: similar to scalar mode but exits on nonzero return
#     code unless '=> IGNORE' is passed.
#   Void context and no args: throw away all stored results.
########################################################################
sub cmd
{
   my($self) = shift;
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
   my $err_match = int &{$self->{_ERRCHK}}(\@{$self->{_STDERR}},
					   \@{$self->{_STDOUT}});

   # Now return different things depending on the context this
   # method was used in - see comment above for details.
   if (wantarray) {
      my $r_results = {
	 'stdout' => $self->{_STDOUT},
	 'stderr' => $self->{_STDERR},
	 'status' => $err_match,
      };
      $self->{_STDOUT} = [];
      $self->{_STDERR} = [];
      return %$r_results;
   } else {
      if (!defined($mode) || $mode == NOTIFY) {
	 $self->stderr;
      } elsif ($mode == PRINT) {
	 $self->stdout;
	 $self->stderr;
      } elsif ($mode == IGNORE) {
	 $self->{_STDOUT} = [];
	 $self->{_STDERR} = [];
      }
      if (defined(wantarray)) {
	 return $err_match;
      } elsif ($err_match && ($mode != IGNORE)) {
	 exit $err_match;
      }
   }
}

# Temporary backward compatibility. Undocumented.
sub command { cmd(@_); }

########################################################################
# Pass the current stdout and stderr arrays to the currently-registered
# error discriminator and return its results (aka the error count).
########################################################################
sub status
{
   my $self = shift;
   return int &{$self->{_ERRCHK}}(\@{$self->{_STDERR}},
				  \@{$self->{_STDOUT}});
}

########################################################################
# Return stored output from previous command(s). Return behavior
# varies with context:
#   Array context: shifts all stored lines off the stdout stack and 
#     returns them in a list.
#   Scalar context: returns the number of lines currently stored in
#     the stdout stack.
#   Void context: prints the current stdout stack to actual stdout.
########################################################################
sub stdout
{
   my($self) = shift;
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

########################################################################
# Similar to 'stdout' method above.
########################################################################
sub stderr
{
   my($self) = shift;
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
## case it's needed someday ...
########################################################################
sub errchk
{
   my($self) = shift;
   my $old_errchk = $self->{_ERRCHK};
   $self->{_ERRCHK} = shift;
   return $old_errchk;
}

########################################################################
# Ends the child process and returns its exit status.
########################################################################
sub finish
{
   my($self) = shift;
   child_close(${$self->{_CHILD}});
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
########################################################################
sub debug
{
   my($self) = shift;
   if (@_) {
      $IPC::ChildSafe::Debug_Level = shift;
   } else {
      $IPC::ChildSafe::Debug_Level = $IPC::ChildSafe::Debug_Level ? 0 : -1>>1;
   }
}

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

   The blocking problem is this: once you've sent a command to your
coprocess, how do you know when the output resulting from this command
has finished?  If you guess wrong and issue one read too many you can
deadlock forever.  This implementation solves the problem, at least for
a subset of possible coprocess programs, by sending a 2nd command with
a known output to mark the EOT from the 1st command.  Read the full
description available in 'ChildSafe.html'.
   This module also returns an "exit status" for each command, which is
really a count of the error messages produced by it.  The programmer
can optionally register his/her own "discriminator function" for
determining which output to stderr constitutes an error message.

=head1 AUTHOR

David Boyce dsb@world.std.com

=head1 SEE ALSO

perl(1), "perldoc IPC::Open3"

=cut
