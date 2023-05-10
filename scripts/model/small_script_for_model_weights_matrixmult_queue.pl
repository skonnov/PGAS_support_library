use POSIX;
use List::Util qw[min max];

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult = ">> ./output_model_matrixmult_queue_several_runs_weights_".$datetime.".txt";
open(WF, $path_matrixmult) or die;

$average = 0;
$num_runs = 7;
$matrix_size = 300;
$process_count = 8;
# $process_count = 10;
# $matrix_size = 240;
print "mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs 100\n";
print WF `mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs 100`;

$x = `python3 ../../model/cache_statistic_model.py -path ./statistics_output -pw 1 -aw 0`;
print "!!!! ", $x, " !!!!\n";
$result = int($x);
print WF $result, " -pw 1 -aw 0\n";
print "mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result\n";
print WF `mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result`;

$x = `python3 ../../model/cache_statistic_model.py -path ./statistics_output -pw 5 -aw 1`;
print "!!!! ", $x, " !!!!\n";
$result = int($x);
print WF $result, " -pw 5 -aw 1\n";
print "mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result\n";
print WF `mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result`;

$x = `python3 ../../model/cache_statistic_model.py -path ./statistics_output -pw 5 -aw 3`;
print "!!!! ", $x, " !!!!\n";
$result = int($x);
print WF $result, " -pw 5 -aw 3\n";
print "mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result\n";
print WF `mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result`;

$x = `python3 ../../model/cache_statistic_model.py -path ./statistics_output -pw 10 -aw 3`;
print "!!!! ", $x, " !!!!\n";
$result = int($x);
print WF $result, " -pw 10 -aw 3\n";
print "mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result\n";
print WF `mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result`;

$x = `python3 ../../model/cache_statistic_model.py -path ./statistics_output -pw 1 -aw 1`;
print "!!!! ", $x, " !!!!\n";
$result = int($x);
print WF $result, " -pw 1 -aw 1\n";
print "mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result\n";
print WF `mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result`;

$x = `python3 ../../model/cache_statistic_model.py -path ./statistics_output -pw 100 -aw 1`;
print "!!!! ", $x, " !!!!\n";
$result = int($x);
print WF $result, " -pw 100 -aw 1\n";
print "mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result\n";
print WF `mpiexec --oversubscribe -n $process_count ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $result`;

close(WF);
