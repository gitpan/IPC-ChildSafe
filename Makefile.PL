use ExtUtils::MakeMaker;

# This module uses an XSUB on Unix. On Windows the XSUB wouldn't
# work. However, the hack below allows the module to be installed
# on Windows anyway; this is because it's possible to subclass it
# such that it DOES work. See ClearTool.pm.

if ($^O !~ /win32/i) {
    WriteMakefile(
	'NAME'	=> 'IPC::ChildSafe',
	'OBJECT'  	=> 'childsafe.o ChildSafe_wrap.o npoll.o',
	'VERSION_FROM' => 'ChildSafe.pm', # finds $VERSION
	'dist' => { SUFFIX => ".gz", DIST_DEFAULT => 'all tardist',
	'COMPRESS' => "gzip -f" },
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
