use POSIX;
use List::Util qw[min max];

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult = ">> ./output_model_matrixmult_queue_".$datetime.".txt";
open(WF, $path_matrixmult) or die;

$matrix_size = 300;
$cache_size_small = 100;
$cache_size_big = 50000;
$quantum_size = 20;
# $quantum_size = 10;
$number_of_processes = 8;
# $number_of_processes = 10;

@cache_sizes_coeffs = (0.5, 0.7, 0.9, 1.1, 1.3, 1.5);

$build_directory = "../../build_linux";
$tmp = -1;
$time = 100500;

print "mpiexec -n $number_of_processes $build_directory/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size_big\n";
for ($i = 0; $i < 3; ++$i) {
    $tmp = `mpiexec --oversubscribe -n $number_of_processes $build_directory/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size_big`;
    $time = min($tmp, $time);
}
print WF "cache_size_big: ", $cache_size_big, " matrix_size: ", $matrix_size, " quantum_size: ", $quantum_size, " number_of_processes: ", $number_of_processes, " time: ", $time, "\n";

$time = 100500;
print "mpiexec -n $number_of_processes $build_directory/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size_small\n";
for ($i = 0; $i < 3; ++$i) {
    $tmp = `mpiexec --oversubscribe -n $number_of_processes $build_directory/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size_small`;
    $time = min($tmp, $time);
}
print WF "cache_size_small: ", $cache_size_small, " matrix_size: ", $matrix_size, " quantum_size: ", $quantum_size, " number_of_processes: ", $number_of_processes, " time: ", $time, "\n";

print "python3 ../../model/cache_statistic_model.py -path ./statistics_output -pw 10 -aw 3\n";
$cache_size_from_model = `python3 ../../model/cache_statistic_model.py -path ./statistics_output -pw 10 -aw 3`;
print "done ../../model/cache_statistic_model.py, cache_size_from_model: $cache_size_from_model\n";

$time = 100500;
print "mpiexec --oversubscribe -n $number_of_processes $build_directory/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size_from_model\n";
for ($i = 0; $i < 3; ++$i) {
    $tmp = `mpiexec --oversubscribe -n $number_of_processes $build_directory/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size_from_model`;
    $time = min($tmp, $time);
}
print WF "cache_size_from_cache_statistic_model: ", $cache_size_from_model, " matrix_size: ", $matrix_size, " quantum_size: ", $quantum_size, " number_of_processes: ", $number_of_processes, " time: ", $time, "\n";


# $cache_size_from_model = int(1043);
for ($cache_size_coeff = 0; $cache_size_coeff < @cache_sizes_coeffs; $cache_size_coeff++) {
    # print @cache_sizes_coeffs[$cache_size_coeff], "!@\n";
    $cache_size = int(int($cache_size_from_model) * @cache_sizes_coeffs[$cache_size_coeff]);
    print "mpiexec --oversubscribe -n $number_of_processes $build_directory/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size\n";
    for ($i = 0; $i < 3; ++$i) {
        $tmp = `mpiexec --oversubscribe -n $number_of_processes $build_directory/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size`;
        $time = min($tmp, $time);
    }
    print WF "cache_size: ", $cache_size, " matrix_size: ", $matrix_size, " quantum_size: ", $quantum_size, " number_of_processes: ", $number_of_processes, " time: ", $time, "\n";
}

close(WF)
