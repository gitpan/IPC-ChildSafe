use IPC::ClearTool;

# Make a cleartool object and turn on verbosity.
my $ct = IPC::ClearTool->new;
$ct->dbglevel(2);

# Initial command. The signal handler must not be set up until this is done.
$ct->cmd("pwv");
print $ct->stdout;

# Now we set up our signal handler. For demo purposes, it skips the
# first few interrupts but then eventually decides to do something.
# It sends an interrupt to the child to kill the current job, then
# sends a pwv command just to show that it can, then shuts the
# child down gracefully and exits itself.
# NOTE: this must be set up AFTER the first command is sent.
my $interrupt = 0;
$SIG{INT} = sub {
    $interrupt++;
    print "\nHIC! ($interrupt)\n";
    if ($interrupt >= 3) {
	$ct->kill;
	$ct->cmd("pwv");
	print $ct->stdout;
	$ct->finish;
	exit 5;
    }
};

# This can be any long running command. We just need a long command
# so there's time to play with signals.
%results = $ct->cmd("find -avobs -print");

# Regular exit path.
$ct->finish && die;
exit 0;
