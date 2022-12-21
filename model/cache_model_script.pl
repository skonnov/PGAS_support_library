use POSIX;

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult = ">> ./output_model_".$datetime.".txt";
open(WF, $path_matrixmult) or die;

$matrix_size = 100;
$cache_size_small = 10;
$cache_size_big = 4500;

$build_directory = "../build_linux";
$tmp = -1;

# print "mpiexec -n 17 $build_directory/Release/matrixmult -size $matrix_size -cs $cache_size_big\n";
# $tmp = `mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -cs $cache_size_big`;
# print WF "cache_size_big: ", $cache_size_big, "time: ", $tmp, " matrix_size: ", $matrix_size;


# print "mpiexec -n 17 $build_directory/Release/matrixmult -size $matrix_size -cs $cache_size_small\n";
# $tmp = `mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -cs $cache_size_small`;
# print WF "cache_size_small: ", $cache_size_small, "time: ", $tmp, " matrix_size: ", $matrix_size;

$cache_size = `python3 cache_statistic_model.py $build_directory`;
print("done cache_statistic_model.py");

$tmp = `mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -cs $cache_size`;
print WF "cache_size_from_cache_statistic_model: ", $cache_size, "time: ", $tmp, " matrix_size: ", $matrix_size;

$cache_size_with_gap = int($cache_size) * 0.1;
$tmp = `mpiexec --oversubscribe -n 17 $build_directory/Release/matrixmult -size $matrix_size -cs $cache_size_with_margin`;
print WF "cache_size_from_cache_statistic_model_with_gap: ", $cache_size_with_gap, "time: ", $tmp, " matrix_size: ", $matrix_size;

close(WF)
