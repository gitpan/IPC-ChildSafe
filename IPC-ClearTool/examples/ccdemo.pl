#!/usr/local/bin/perl -w
# Demonstrates the use of the IPC::ClearTool.pm module with cleartool.
use IPC::ClearTool;

if ($#ARGV >= $[) {
   @allfiles = @ARGV;
} else {
   # Get the list of files in the current directory.
   opendir(DOT, '.') || die;
   @allfiles = readdir DOT;
   closedir(DOT) || die;
}

# Instantiate a ClearTool object (start the cleartool process).
$CT = IPC::ClearTool->new;

#$CT->dbglevel(2);

# In this loop we iterate over each file in the current directory,
# sending a 'describe' command to cleartool for each one.

foreach(@allfiles) {
   # Send a 'cleartool describe' down the pipe for each file
   my(%Results) = $CT->cmd("desc -s $_@@");
   next if !defined(%Results);

   # If the describe caused an error, the file is not a versioned element.
   if ($Results{status} == 0) {
      print "$_: is an element\n";
   } else {
      print "$_: is NOT an element\n";
   }
}

# End the coprocess.
$CT->finish && die;

exit 0;
