use ExtUtils::MakeMaker;
# See lib/ExtUtils/MakeMaker.pm for details of how to influence
# the contents of the Makefile that is written.
WriteMakefile(
    'NAME'	=> 'IPC::ClearTool',
    'VERSION_FROM' => 'ClearTool.pm', # finds $VERSION
    'dist'		=> {
	    COMPRESS => 'gzip --force --best',
	    TAR	 => 'gtar',
	    TARFLAGS => '--mode=u+w -cvf',
    },
);
