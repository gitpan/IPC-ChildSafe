#!/usr/local/bin/perl -w
# Usage: $0 [iterations]

=pod

This is a benchmarking version of the ClearTool module demo script.  It
measures the time to run a single cleartool command ('pwv') using both
the traditional `backtick` method and the ClearTool module.  The upshot
is to demonstrate that there is only a small additional cost to using
the module even for a single command, while the module is much faster for
multiple commands.  Thus if you're writing a trigger or something
which might in one code path only run cleartool once but could run it n
times in other paths, this shows that it may well be worth using the
module.

However, this benchmark does not include the time of loading the module;
it presumes the module has already been use-d.
See ccbm-load.pl for a measurement including constant load times.

Here is a typical comparison:

Benchmark: timing 300 iterations of Backticks, Module...
  Backticks: 46 secs ( 0.07 usr  0.40 sys + 30.17 cusr 13.82 csys = 44.46 cpu)
    Regular: 49 secs ( 0.61 usr  0.81 sys + 32.96 cusr 13.58 csys = 47.96 cpu)

=cut

use IPC::ClearTool;
use Benchmark;

my $backticks = sub {
   my @junk = `/usr/atria/bin/cleartool pwv`;
};

my $module = sub {
   my $CT = IPC::ClearTool->new;
   my %junk = $CT->cmd('pwv');
   $CT->finish && die;
};

my $iterations = $ARGV[0] || 100;

timethese($iterations, {
   'ClearTool' => $module,
   'Regular  ' => $backticks,
});
