use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'	=> 'IPC::ChildSafe',
    'OBJECT'  	=> 'childsafe.o ChildSafe_wrap.o',
    'VERSION_FROM' => 'ChildSafe.pm', # finds $VERSION
    'dist' => { SUFFIX => ".gz", DIST_DEFAULT => 'all tardist',
    'COMPRESS' => "gzip -f" },
);
