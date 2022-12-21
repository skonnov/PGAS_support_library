use POSIX;

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult = ">> ./output_model_".$datetime.".txt";
open(WF, $path_matrixmult) or die;

$matrix_size = 100;
$cache_size_small = 10;
$cache_size_big = 4500;
$quantum_size = 10;

$build_directory = "../build_linux";
$tmp = -1;

print "mpiexec -n 17 $build_directory/Release/matrixmult -size $matrix_size -quantum_size $quantum_size -cs $cache_size_big\n";
$tmp = `mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -quantum_size $quantum_size -cs $cache_size_big`;
print WF "cache_size_big: ", $cache_size_big, " time: ", $tmp, " matrix_size: ", $matrix_size, "\n";

print "mpiexec -n 17 $build_directory/Release/matrixmult -size $matrix_size -quantum_size $quantum_size -cs $cache_size_small\n";
$tmp = `mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -quantum_size $quantum_size -cs $cache_size_small`;
print WF "cache_size_small: ", $cache_size_small, " time: ", $tmp, " matrix_size: ", $matrix_size, "\n";

print "python3 cache_statistic_model.py $build_directory\n";
$cache_size = `python3 cache_statistic_model.py $build_directory`;
print "done cache_statistic_model.py, cache_size: $cache_size\n";

print "mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -quantum_size $quantum_size -cs $cache_size\n";
$tmp = `mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -quantum_size $quantum_size -cs $cache_size`;
print WF "cache_size_from_cache_statistic_model: ", $cache_size, " time: ", $tmp, " matrix_size: ", $matrix_size, "\n";

$cache_size_with_gap = int(int($cache_size) * 1.1);
print "mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -quantum_size $quantum_size -cs $cache_size_with_gap\n";
$tmp = `mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -quantum_size $quantum_size -cs $cache_size_with_gap`;
print WF "cache_size_from_cache_statistic_model_with_gap: ", $cache_size_with_gap, " time: ", $tmp, " matrix_size: ", $matrix_size, "\n";

close(WF)
