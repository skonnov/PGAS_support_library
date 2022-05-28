use Cwd qw(abs_path);
my $path = abs_path($0);

print "Build library...\n";

chdir('..') or die "$!";
if (-d "build") {
    if ($^O eq "linux") {
        `rm -r build`;
    } elsif ($^O eq "MSWin32") {
        `rmdir build /s /q`;
    }
}
`mkdir build`;
chdir('build') or die "$!";
`cmake ..`;
`cmake --build . --config release`;
chdir('../scripts') or die "$!";

print "Library has been built successfully!\n";