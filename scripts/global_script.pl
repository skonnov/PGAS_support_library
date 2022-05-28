use List::Util qw[min max];
use POSIX;

print "Run samples...\n";

$it_sov = 5000000;
$it_mv = 1500;
$it_mm = 300;
$it_d = 500;
$it_sov_step = 1000000, $it_mv_step = 1000, $it_mm_step = 100, $it_d_step = 500;
$it_sov_max = 150000000, $it_mv_max = 20000, $it_mm_max = 1800, $it_d_max = 5001;
$min_proc = 2, $max_proc = 32;
@procs_mm = (2, 5, 10, 17, 37);


$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_parallel_reduce_sum_of_vector = ">> ./output_parallel_reduce_sum_of_vector_".$datetime.".txt";
$path_matrixvector = ">> ./output_matrixvector_".$datetime.".txt";
$path_matrixmult = ">> ./output_matrixmult_".$datetime.".txt";
$path_dijkstra = ">> ./output_dijkstra_".$datetime.".txt";

while ($it_sov < $it_sov_max || $it_mv < $it_mv_max || $it_mm < $it_mm_max || $it_d < $it_d_max) {
    if ($it_sov < $it_sov_max) {
        open(WF, $path_parallel_reduce_sum_of_vector) or die;
        for ($j = $min_proc; $j <= $max_proc; $j = $j*2) {
            $result = 250000.0;
            for ($k = 0; $k < 3; $k++) {
                $tmp = `mpiexec -n $j ../build/Release/parallel_reduce_sum_of_vector $it_sov`;
                $result = min($result + 0.0, $tmp+0.0);
            }
            print WF $j, ": ", $result, " ";
        }
        print WF "\n";
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
            print WF2 $j, ": ", $result, " ";
        }
        print WF2 "\n";
        print "matrixvector for $it_mv\n";
        $it_mv += $it_mv_step;
        close(WF2);
    }
    if ($it_mm < $it_mm_max) {
        open(WF3, $path_matrixmult) or die;
        for ($proc = 0; $proc < @procs_mm; $proc++) {
            $result = 250000.0;
            if ($it_mm % int(sqrt(@procs_mm[$proc]-1)) == 0) {
                for ($k = 0; $k < 3; $k++) {
                    $tmp = `mpiexec -n @procs_mm[$proc] ../build/Release/matrixmult $it_mm`;
                    $result = min($result + 0.0, $tmp+0.0);
                }
                print WF3 @procs_mm[$proc], ": ", $result, " ";
                print "done matrix mult for @procs_mm[$proc] procs and $it_mm elems\n";
            }
        }
        print WF3 "\n";
        $it_mm += $it_mm_step;
        close(WF3);
    }
    if ($it_d < $it_d_max) {
        open(WF4, $path_dijkstra) or die;
        for ($j = $min_proc; $j <= $max_proc; $j = $j*2) {
            $result = 250000.0;
            for ($k = 0; $k < 3; $k++) {
                $tmp = `mpiexec -n $j ../build/Release/dijkstra -v $it_d -max 150`;
                $result = min($result + 0.0, $tmp+0.0);
            }
            print "dijkstra algorithm for v = $it_d, $j procs is done\n";
            print WF4 $result, " ";
        }
        print WF4 "\n";
        $it_d += $it_d_step;
        close(WF4);
    }
}

