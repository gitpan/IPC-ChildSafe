#!/usr/local/bin/perl -w
# This is a benchmarking version of the cleartool demo script.
# It runs a "cleartool describe" on every file in @ARGV twice:
# once using the ClearTool coprocess and once doing a system()
# to fork/exec each cleartool process. It then reports how long that
# took. In my env I've gotten results typically and repeatably like this:
#
# Visited 26 files, 12 of them clearcase elements:
# ClearTool: 2 secs ( 0.01 usr  0.08 sys +  0.69 cusr  0.62 csys =  1.40 cpu)
# Regular:  18 secs ( 0.01 usr  0.10 sys +  6.12 cusr  8.11 csys = 14.34 cpu)
#
# Visited 789 files, 782 of them clearcase elements:
# ClearTool: 44 secs ( 1.68 usr 0.65 sys + 13.07 cusr  6.17 csys = 21.57 cpu)
# Regular:  523 secs ( 0.21 usr 3.91 sys + 185.16 cusr 224.16 csys = 413.44 cpu)
#
# Of course, the bigger the directory (the more cleartool cmds run),
# the larger the gap will become. As shown above, in my environment this
# scales to beyond an order of magnitude improvement. In other situations,
# though, I've seen much less, as little as a factor of 2.

use IPC::ClearTool qw(IGNORE);
use Benchmark;

die "Usage: $0 file ...\n" unless $#ARGV >= 0;

########################################################################
# In this block we look for versioned elements using the coprocess.
########################################################################

$t0 = new Benchmark;

# Instantiate a ClearTool object (start the cleartool program).
$CT = IPC::ClearTool->new;

# In this loop we iterate over each file in the current directory,
# sending a cleartool describe command to cleartool for each one.
my $elems = 0;
for my $file (@ARGV) {
   # Send a 'cleartool describe' down the pipe for each file
   # Bump the found-element counter if describe succeeded.
   $elems++ if $CT->cmd("desc -s $file@@" => IGNORE) == 0;
}
$CT->finish && die;

########################################################################
# and now by forking and exec-ing a cleartool process for each file.
########################################################################

$t1 = new Benchmark;

for my $file (@ARGV) {
   system("cleartool desc -s $file >/dev/null 2>&1");
}

$t2 = new Benchmark;

$files = $#ARGV + 1;
print "Visited $files files, $elems of them clearcase elements:\n";
$cp_diff = timediff($t1, $t0);
print " ClearTool:",timestr($cp_diff),"\n";
$sy_diff = timediff($t2, $t1);
print " Regular:  ",timestr($sy_diff),"\n";

exit 0;
