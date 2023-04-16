use POSIX;
use List::Util qw[min max];

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult = ">> ./output_model_several_runs_".$datetime.".txt";
open(WF, $path_matrixmult) or die;

$average = 0;
$num_runs = 7;
for ($i = 0; $i < $num_runs; ++$i) {
    print WF `mpiexec --oversubscribe -n 10 ../build_linux/Release/matrixmult -size 300 -quantum_size 10 -cs 100`;
    $result = `python3 ../model/cache_statistic_model.py .`;
    print WF $result, "\n";
    $average += $result;
}

$average /= $num_runs;
print WF "average: $average \n";
print WF `mpiexec --oversubscribe -n 10 ../build_linux/Release/matrixmult -size 300 -quantum_size 10 -cs $average`;

close(WF);
