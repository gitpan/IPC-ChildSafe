# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

my $final = 0;

open(STDERR, ">&STDOUT");

# Automatically generates an ok/nok msg, incrementing the test number.
BEGIN {
   $| = 1;
   my($next, @msgs);
   sub printok {
      push @msgs, ($_[0] ? '' : 'not ') . "ok @{[++$next]}\n";
      return !$_[0];
   }
   END {
      print "\n1..", scalar @msgs, "\n", @msgs;
   }
}

use IPC::ChildSafe;
$final += printok(1);

if ($^O =~ /win32/i) {
    print "No further testing on $^O - module exists here only to subclass\n";
    exit 0;
}

# Bourne shell test/demo.
{
   $SH = IPC::ChildSafe->new('sh', 'echo ++EOT++', '++EOT++');
   printok($SH->cmd('id') == 0 && $SH->finish == 0);
}

# If no ksh exists, assume it's a linux or similar system whose 'sh'
# is a POSIX-like shell (e.g. bash for linux).
my $shell = -x q(/bin/ksh) ? q(/bin/ksh) : q(sh);

# Start ksh coprocess for remaining tests.
$KSH = IPC::ChildSafe->new($shell, 'echo ++EOT++', '++EOT++');

# Test #3
{
   $KSH->dbglevel(2);
   my(%results) = $KSH->cmd('tail -4 /etc/passwd');
   printok(defined(%results) && $results{status} == 0 &&
	   @{$results{stdout}} == 4 && @{$results{stderr}} == 0);
}

$KSH->dbglevel(1);

# Test #4
{
   my(%results) = $KSH->cmd('ls /bin/cat /+/- /+/+');
   printok(defined(%results) && $results{status} == 2 &&
	   @{$results{stdout}} == 1 && @{$results{stderr}} == 2);
}

# Test #5
{
   my(%results) = $KSH->cmd('ls /bin/cat /bin/mv /a/b/c/d');
   printok(defined(%results) && $results{status} == 1 &&
	   @{$results{stdout}} == 2 && @{$results{stderr}} == 1);
}

# Test #6
{
   $KSH->cmd('date');
   printok($KSH->stdout == 1 && $KSH->stderr == 0);
   $KSH->cmd;	# clear output stacks
}

# Test #7
{
   my $cnt = 10000;
   $KSH->store;		# change mode to keep stderr stored
   $KSH->cmd("$^X ./stresspipes $cnt") || 1;	# avoid exit in void context
   printok($KSH->stdout == 1 && $KSH->stderr == $cnt);
}

printok($KSH->finish == 0);		# Kornshell exit status
