for($i = 1; $i < 100; $i++) {
    print $i;
    for($j = 1; $j < 8; $j++) {
        `mpiexec -n $j ../build/Release/main_parallel_for`
    }
}