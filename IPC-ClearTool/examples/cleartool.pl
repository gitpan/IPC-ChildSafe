# Usage: perl cleartool.pl <subcommand> args ...

use Win32::OLE;

# Stop err msgs from being printed automatically
Win32::OLE->Option(Warn => 0);

# Instantiate a ClearTool object.
my $ct = Win32::OLE->new('ClearCase.ClearTool');

# Stdout equivalent
## Note: CmdExec always returns a scalar through Win32::OLE so we
## should really split the output in case it started out as a list.
$out = $ct->CmdExec(@ARGV);
print $out;

# Return code
my $rc = int Win32::OLE->LastError;

# Nearest stderr equivalent
print STDERR Win32::OLE->LastError, "\n" if $rc;

exit $rc;
