# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN { $| = 1; print "1..2\n"; }
END {print "not ok 1\n" unless $loaded;}
use IPC::ClearTool;
$loaded = 1;
print "ok 1\n";

######################### End of black magic.

# Test #2 
if (-x '/usr/atria/bin/cleartool') {
   my $CT = IPC::ClearTool->new;
   die "not ok 2" if $CT->cmd('pwv');
   die "not ok 2" if $CT->stdout != 2;	# expect two lines of output
   die "not ok 2" if $CT->finish;
} else {
   warn "\nWarning: no ClearCase installed on this system!!!\n";
}
print "ok 2\n";
