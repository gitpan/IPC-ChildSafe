use ExtUtils::MakeMaker;

# This module uses XS on Unix. On Windows the XS code wouldn't
# work. However, the hack below allows the module to be installed
# on Windows anyway; this is done because it's possible to subclass it
# such that it DOES work (see ClearTool.pm).

if ($^O !~ /win32|Windows_NT|cygwin/i) {
    WriteMakefile(
	NAME		=> 'IPC::ChildSafe',
	OBJECT  	=> 'childsafe.o ChildSafe_wrap.o npoll.o',
	VERSION_FROM	=> 'ChildSafe.pm',
	dist		=> {
	    COMPRESS	=> 'gzip --force --best',
	    TAR		=> 'gtar',
	    TARFLAGS	=> '--mode=u+w -cvf',
	},
    );
} else {
    # This is an awful hack but I stole it from Data::Dumper by
    # Sarathy so at least I'm in good company!
    use File::Copy qw();
    for (qw(MANIFEST childsafe.c ChildSafe_wrap.c npoll.c)) {
	File::Copy::move($_, "$_.orig") if -e $_;
    }
    File::Copy::copy('MANIFEST.WIN', 'MANIFEST');
    WriteMakefile(
	'NAME'	=> 'IPC::ChildSafe',
	'VERSION_FROM' => 'ChildSafe.pm',
    );
}
