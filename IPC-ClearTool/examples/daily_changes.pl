#!/usr/local/bin/perl

=pod
Prints a set of daily-changes reports for ClearCase sites.

Each report basically follows a particular branch through a specified
set of VOBS and sends its output (if -mail is used) to a specified set
of users. Thus a "report" is defined as the triple
"branch/vob-list/mail-list".

This script must be under ClearCase control itself, because it's driven
off a set of attributes which it finds on itself.  All of these
attributes begin with DCR_.  The reports are listed as
"DCR_Rpt_<name>", the vob lists are "DCR_Vobs_<name>", and the mailing
lists are "DCR_Mailto_<name>".  The values are always a string-valued
list which can be either space- or comma-separated, except for the
mailto values which must use commas between addresses.

Here's a sample set of attributes:

   # The list of reports - "branch:vob-list:mail-list"
   DCR_Rpt_1 = "main:research:research"
   DCR_Rpt_2 = "main:website:website"
   DCR_Rpt_3 = "wx_1_5_dev:webx:webx_1.5"
   DCR_Rpt_4 = "wx_1_6_dev:webx:webx_1.6"

   # VOB sets as used in the 2nd field above.
   DCR_Vobs_research = "/vobs/research/src /vobs/cookies"
   DCR_Vobs_website = "/vobs/website/src /vobs/website/do /vobs/cookies"
   DCR_Vobs_webx = "/vobs/corba /vobs/release /vobs/share /vobs/ssl /vobs/tools"

   # Mailing lists as used in the 3rd field above.
   DCR_Mailto_research = "tom@here.com,dick@here.com"
   DCR_Mailto_website = "tom@here.com,dick@here.com,harry@here.com"
   DCR_Mailto_webx_1.5 = "janice@here.com,jane@here.com"
   DCR_Mailto_webx_1.6 = "joe@here.com,jane@here.com,sara@here.com"

As a special case, the attribute 'DCR_Administrators' can be used to
point to a list of email addresses of ClearCase admins, who will then
be cc-ed on all mail messages.

=cut

require 5.004;

local($Prog) = reverse split('/', $0);

sub Usage {
   my $status = shift;
   select($status ? STDERR : STDOUT);
   print <<EOF;
Usage: $Prog [-help] [-mail [-to addr,addr,...]] [-view <view-tag>]
	     [-reports x,y,...] [-since <date>]
	     [-debug <n>]
Flags: [default]
   -help	Show this message
   -mail	Send results via email (to addresses listed in attributes)
   -to addr,...	Replace default mail distribution with specified list
   -view tag	Work in specified view ['admin']
   -since date	Show changes since specified date ['yesterday']
   -report x,y  Generate only reports x and y [all]
   -debug n	Run at debug level n (0-4) [0]
EOF
   exit $status;
}

#use strict;
use Cwd qw(chdir getcwd);
use Getopt::Long;
use Mail::Send;
use IPC::ClearTool qw(IGNORE STORE);

my($opt_help, $opt_debug, $opt_mail, @opt_to,
   $opt_view, $opt_since, @opt_reports);
GetOptions("help|usage" => \$opt_help,
	   "debug=i"	=>\$opt_debug,
	   "mail"	=> \$opt_mail,
	   "to=s@"	=> \@opt_to,
	   "view=s"	=> \$opt_view,
	   "since=s"	=> \$opt_since,
	   "reports=s@"	=> \@opt_reports);

Usage(0) if $opt_help;

my $View = $opt_view || 'admin';	# the view to run in
my $Since = $opt_since || 'yesterday';	# the date to compare against
my $Indent = ' ' x 3;			# the amount to indent output by

# Make sure $0 is the full path to this executable.
$0 = join('/', getcwd(), $0) unless $0 =~ m%^[/\\]%;

# Start the cleartool process.
my $CT = IPC::ClearTool->new;

# Debugging level.
$CT->debug($opt_debug) if $opt_debug;

# We need a view to work through.
$CT->cmd("startview $View");

# These will hold the data derived from attributes on $0
my(%Report, %Vobs, %Mailto);

# Check the CC attributes on this script for all attributes
# matching /^DCR_*/.  These control what we do.
$CT->cmd("desc -aattr -all $0\@\@");
my @dcrs = $CT->stdout;
chomp @dcrs;

for my $attr (@dcrs[2..$#dcrs]) {
   my($key, $val);
   (undef, $key, $val) = split(/[\s=]+/, $attr, 3);
   ($val) = ($val =~ /"(.*)"/);
   if ($key =~ s/^DCR_Rpt_//) {
      $Report{$key} = $val;
   } elsif ($key =~ s/^DCR_Vobs_//) {
      $Vobs{$key} = $val;
   } elsif ($key =~ s/^DCR_Mailto_//) {
      $Mailto{$key} = $val;
   } elsif ($key =~ /^DCR_Administrator/) {
      $Mailto{Admins} = $val;
   }
}

chomp(my $Date = `date '+%a %d-%b-%y'`);

my(%Changers, %Members, %Files);

if (@opt_reports) {
   @opt_reports = split(/[\s,:]/, "@opt_reports");
} else {
   @opt_reports = sort {$a <=> $b} keys %Report;
}

# Makes tail -f of output file easier when testing.
$| = 1;

# "Paragraph mode" - causes chomp to remove multiple trailing \n
$/ = "";

for my $report (@opt_reports) {
   my($branch,$grp_vobs,$grp_mailto) = split(/:/, $Report{$report});

   my $title = "Daily Changes Report #$report ($branch/$grp_vobs) for $Date";

   my %members = ();
   my %changers = ();

   for my $mkey (split(/,/, join(',', $Mailto{$grp_mailto}, $Mailto{Admins}))) {
      $mkey =~ s/@.*//;
      $mkey =~ s/\./ /;
      $Members{lc($mkey)} = $members{lc($mkey)} = $mkey;
   }

   # Clear the database for each new report.
   undef %Files;

   for my $vob (split(/[\s:,]+/, $Vobs{$grp_vobs})) {

      # The vob needs to be mounted and the child process needs
      # to be cd-ed into the vob.
      $CT->cmd("mount $vob" => IGNORE);
      next if $CT->cmd("cd /view/$View$vob" => IGNORE);

      # Get the basic list of modified elements.
      next if $CT->cmd("find . -all -type f -ver 'brtype($branch) && created_since($Since)' -print");

      # Now, for each changed elem we run two 'cleartool describe' cmds:
      # one to get the data on the elem itself and another for the comment.
      for my $version (sort $CT->stdout) {

	 chomp($version);

	 # Ignore zero-branches and checked-out elements.
	 $version =~ m+/CHECKEDOUT$|/0$+ && next;

	 # Skip anything in a 'private' subdirectory.
	 $version =~ m+/private/+ && next;

	 $CT->cmd("desc -fmt \"%Sd===%Fu===%a===%En@@%Sn\\n\" '$version'")
	       && next;
	 my($line) = $CT->stdout; # read 1st line, throw out any leftovers
	 chomp $line;
	 my($date, $who, $attrs, $xpn) = split /===/, $line;

	 # Separate the version-extended pathname into path and version.
	 my($pn, $vers) = ($xpn =~ m%(.*)@@(.*)%);

	 # Normalize the username since we'll be using it as part of a key.
	 $who =~ s/\s+/ /g;

	 # Make an entry for this user indicating that we've seen him/her.
	 $Changers{lc($who)} = $changers{lc($who)} = $who
	       unless $who =~ /admin/i;

	 # Remove the view-extended prefix from the view- and
	 # version-extended pathname.
	 $pn =~ s%^/view/[^/]+?(/.*)%$1%;

	 # Simplify the pathname to remove version-extended subdirs
	 # for readability.
	 $pn =~ s%(@@)?/main/.*?/(CHECKEDOUT\.)?\d+/%/%g;

	 # Stick any attrs (particularly SPR) at the end of the path.
	 $pn .= " $attrs" if $attrs;

	 # Get the comment and format it correctly.
	 next if $CT->cmd("desc -fmt \"%c\\n\" '$version'");
	 my $cmt = join('', $CT->stdout);
	 chomp $cmt;

	 # Show the date for each change, unless it's guaranteed
	 # to be yesterday.
	 if ($opt_since) {
	    push(@{$Files{$who,$cmt}},
	       sprintf("%-60s  - %s [%s]", $pn, $vers, $date));
	 } else {
	    push(@{$Files{$who,$cmt}},
	       sprintf("%-60s  - %s", $pn, $vers));
	 }
      }
   }

   # Now we have a database (in %Files) of all changes found.
   # Format and print this data, to stdout or to mail as requested.
   {
      my($msg, $mh) = ();

      # If using mail, create a message object and make it the default
      # output filehandle.
      if ($opt_mail) {
	 $msg = new Mail::Send;
      } else {
	 print "\n$Indent ** $title **\n\n";
      }

      # If no changes, skip to the next report and send mail (if requested)
      # only to admins.
      if (! keys %Files) {
	 if ($msg) {
	    $msg->subject("[No Changes] $title");
	    $msg->to(@opt_to ? @opt_to : $Mailto{Admins});
	    $mh = $msg->open;
	    $mh->close;
	 } else {
	    print "\n$Indent(No changes)\n\n";
	 }
	 next;
      }

      # Now we know there were changes, so it's time to generate a report.

      # Set up the mailer if requested.
      if ($msg) {
	 $msg->subject($title);
	 $msg->to(@opt_to ? @opt_to : join(',', $Mailto{Admins},
						$Mailto{$grp_mailto}));
	 select($mh = $msg->open);
      }

      for my $key (keys %Files) {
	 my($name, $comment) = split(/$;/, $key);
	 if (defined($members{lc($name)}) || $name =~ /admin/i) {
	    print "By $name:\n";
	 } else {
	    print "By $name: (Wanderer)\n";
	 }
	 for my $line (sort @{$Files{$key}}) {
	    print "$Indent$line\n";
	 }
	 $comment =~ s/\n/\n$Indent> /gs;
	 print "\n$Indent> $comment\n\n\n";
      }

      $mh->close if $msg;
   }
}

# Compare the list of people who made changes against the list of people
# mentioned as members of groups (mailto's).  Give a warning if someone
# on the first list is not in the second.
{
   my @intruders;
   for my $changer (sort keys %Changers) {
      push(@intruders, $Changers{$changer}) unless $Members{$changer};
   }
   $" = "\n"; # print these names one per line
   warn "Warning: the following are not on any membership list:\n@intruders\n"
      if @intruders;
}

$CT->finish && die;
