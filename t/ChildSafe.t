# Before `make install' is performed this script should be runnable with
# `make test'. After `make install' it should work as `perl test.pl'

######################### We start with some black magic to print on failure.

# Change 1..1 below to 1..last_test_to_print .
# (It may become useful if the test is moved to ./t subdirectory.)

BEGIN { $| = 1; print "1..8\n"; }
END {print "not ok 1\n" unless $loaded;}
use IPC::ChildSafe;
$loaded = 1;
print "ok 1\n";

######################### End of black magic.

# Hack - this can be uncommented to force diagnostic output.
#$IPC::ChildSafe::Debug_Level = 4;

# Test #2 (uses bourne shell)
{
   $SH = IPC::ChildSafe->new('sh', 'echo ++EOT++', '++EOT++');
   die "not ok 2" if $SH->cmd('id');
   die "not ok 2" if $SH->finish;
   print "ok 2\n";
}

# Start ksh coprocess for remaining tests.
$KSH = IPC::ChildSafe->new('ksh', 'echo ++EOT++', '++EOT++');

# Test #3
{
   my(%results) = $KSH->cmd('tail -4 /etc/passwd');
   die "not ok 3" if !defined(%results) || $results{status};
   die "not ok 3" if @{$results{stdout}} != 4;
   die "not ok 3" if @{$results{stderr}} != 0;
   print "ok 3\n";
}

# Test #4
{
   my(%results) = $KSH->cmd('ls /bin/cat /+/- /+/+');
   die "not ok 4" if !defined(%results);
   die "not ok 4" if @{$results{stdout}} != 1;
   die "not ok 4" if @{$results{stderr}} != 2;
   die "not ok 4" if $results{status} != 2;
   print "ok 4\n";
}

# Test #5
{
   my(%results) = $KSH->cmd('ls /bin/cat /bin/mv /a/b/c/d');
   die "not ok 5" if !defined(%results);
   die "not ok 5" if @{$results{stdout}} != 2;
   die "not ok 4" if @{$results{stderr}} != 1;
   die "not ok 5" if $results{status} != 1;
   print "ok 5\n";
}

# Test #6
{
   $KSH->cmd('date');
   die "not ok 6" unless $KSH->stdout == 1;
   die "not ok 6" unless $KSH->stderr == 0;
   print "ok 6\n";
}

# Test #7
{
   $KSH->system('echo test output >&2');
   die "not ok 7" unless $? == (1 << 8);
   print "ok 7\n";
}

# Test #8
{
   my $line = $KSH->backtick('echo hi there');
   die "not ok 8" unless $line =~ /hi there/;
   die "not ok 8" unless $? == 0;
   print "ok 8\n";
}

die "not ok last" if $KSH->finish;			# Kornshell exit status
