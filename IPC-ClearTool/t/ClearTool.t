# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

my $final = 0;

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

use IPC::ClearTool;
$final += printok(1);

if ($^O !~ /win32/i && ! -x '/usr/atria/bin/cleartool') {
   warn "\nNo ClearCase found on this system, IPC::ClearTool not tested\n";
   exit 0;
}

my $CT = IPC::ClearTool->new;
printok($CT->cmd('pwv') == 0 && $CT->stdout == 2 && $CT->finish == 0);
