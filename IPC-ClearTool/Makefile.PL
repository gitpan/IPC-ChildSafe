use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'	=> 'IPC::ClearTool',
    'VERSION_FROM' => 'ClearTool.pm', # finds $VERSION
    'LIBS'	=> [''],   # e.g., '-lm' 
    'DEFINE'	=> '',     # e.g., '-DHAVE_SOMETHING' 
    'INC'	=> '',     # e.g., '-I/usr/include/other' 
    'dist'	=> { SUFFIX => ".gz", DIST_DEFAULT => 'all tardist',
    'COMPRESS' => "gzip -f" },
);
