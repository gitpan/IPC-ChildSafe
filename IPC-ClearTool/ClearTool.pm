package IPC::ClearTool;

use strict;
use vars qw($VERSION @ISA @EXPORT_OK %EXPORT_TAGS);

use IPC::ChildSafe;
@EXPORT_OK = @IPC::ChildSafe::EXPORT_OK;
%EXPORT_TAGS = ( BehaviorMod => \@EXPORT_OK );

@ISA = q(IPC::ChildSafe);

# The current version and a way to access it.
$VERSION = "1.11"; sub version {$VERSION}

sub new
{
   my $proto = shift;
   my $class = ref($proto) || $proto;
   my $ct = join('/', $ENV{ATRIAHOME} || '/usr/atria', 'bin/cleartool');
   $ct = 'cleartool' unless -x $ct;
   my $chk = sub {
      my($r_stderr, $r_stdout) = @_;
      return int grep /Error:\s/, @$r_stderr;
   };
   my %params = ( QUIT => 'exit', CHK => $chk );
   my $self = $class->SUPER::new($ct, 'pwd -h', 'Usage: pwd', \%params);
   bless ($self, $class);
   return $self;
}

1;

__END__

=head1 NAME

IPC::ClearTool, ClearTool - run a bidirectional pipe to a cleartool process

=head1 SYNOPSIS

  use IPC::ClearTool;

  my $CT = IPC::ClearTool->new;
  $CT->cmd("pwv");
  $CT->cmd("lsview");
  $CT->cmd("lsvob -s");
  for ($CT->stdout) { print }
  $CT->finish;

=head1 ALTERNATE SYNOPSES

  use IPC::ClearTool;

  $rc = $CT->cmd("pwv");		# Assign return code to $rc

  $CT->notify;				# "notify mode" is default;
  $rc = $CT->cmd("pwv");		# same as above

  $CT->store;				# "Store mode" - stack up stderr for
  $rc = $CT->cmd("pwv -X");		# later retrieval via $CT->stderr
  @errs = $CT->stderr;			# Retrieve it now

  $CT->ignore;				# Discard all stdout/stderr and
  $CT->cmd("pwv");			# ignore nonzero return codes

  $CT->cmd("ls foo@@");			# In void context, store stdout,
					# print stderr immediately,
					# exit on error.

  my %results = $CT->cmd("pwv");	# Place all results in %results,
					# available as:
					#   @{$results{stdout}}
					#   @{$results{stderr}}
					#   @{$results{status}}

  $CT->cmd();				# Clear all accumulators

  $CT->stdout;				# In void context, print stored output

=head1 DESCRIPTION

This module invokes the ClearCase 'cleartool' command as a child
process and opens pipes to its standard input, output, and standard
error. Cleartool commands may be sent "down the pipe" via the
$CT->cmd() method.  All stdout resulting from commands is stored in the
object and can be retrieved at any time via the $CT->stdout method. By
default, stderr from commands is sent directly to the real (parent's)
stderr but if the I<store> attribute is set as shown above, stderr will
accumulate just like stdout and must be retrieved via $CT->stderr.

If $CT->cmd is called in a void context it will exit on error unless
the I<ignore> attribute is set, in which case all output is thrown away
and error messages suppressed.  If called in a scalar context it
returns the exit status of the command.

When used with no arguments and in a void context, $CT->cmd simply
clears the stdout and stderr accumulators.

The $CT->stdout and $CT-stderr methods behave just like arrays; when
used in a scalar context they return the number of lines currently
stored.  When used in an array context they return, well, an array
containing all currently stored lines, and then clear the internal
stack.

The $CT->finish method ends the child process and returns its exit
status.

This is only a summary of the documentation. There are more advanced
methods for error detection, data return, etc. which are documented as
part of IPC::ChildSafe. Note that IPC::ClearTool is simply a very small
subclass of IPC::ChildSafe; it merely provides the right defaults to
IPC::ChildSafe's constructor for running cleartool. In all other ways
it is identical to ChildSafe and all ChildSafe documentation applies.

=head1 AUTHOR

David Boyce dsb@world.std.com

=head1 SEE ALSO

perl(1), "perldoc IPC::ChildSafe"

=cut
