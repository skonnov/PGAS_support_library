use List::Util qw[min max];
use POSIX;

$it_sov = 1000000;
$it_mv = 1000;
$it_sov_step = 1000000, $it_mv_step = 1000;
$it_sov_max = 150000000, $it_mv_max = 20000;
$min_proc = 2, $max_proc = 4;

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_parallel_reduce_sum_of_vector = ">> ./output_parallel_reduce_sum_of_vector_".$datetime.".txt";
$path_matrixvector = ">> ./output_matrixvector_".$datetime.".txt";

while($it_sov < $it_sov_max || $it_mv < $it_mv_max) {
    if ($it_sov < $it_sov_max) {
        open(WF, $path_parallel_reduce_sum_of_vector) or die;
        for ($j = $min_proc; $j <= $max_proc; $j = $j*2) {
            $result = 250000.0;
            for ($k = 0; $k < 3; $k++) {
                $tmp = `mpiexec -n $j ../build/Release/parallel_reduce_sum_of_vector $it_sov`;
                $result = min($result + 0.0, $tmp+0.0);
            }
            print WF $result;
            print WF " ";
        }
        print "parallel_reduce for $it_sov\n";
        $it_sov += $it_sov_step;
        close(WF);
    }
    if ($it_mv < $it_mv_max) {
        open(WF2, $path_matrixvector) or die;
        for ($j = $min_proc; $j <= $max_proc; $j = $j*2) {
            $result = 250000.0;
            for ($k = 0; $k < 3; $k++) {
                $tmp = `mpiexec -n $j ../build/Release/matrixvector $it_mv $it_mv`;
                $result = min($result + 0.0, $tmp+0.0);
            }
            print WF2 $result;
            print WF2 " ";
        }
        print WF2 "\n";
        print "matrixvector for $it_mv\n";
        $it_mv += $it_mv_step;
        close(WF2);
    }
}

