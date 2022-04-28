use List::Util qw[min max];
use POSIX;

@procs = (3, 4, 8, 16);
@divs = (1, 2, 3, 4, 5, 6);
$it_min = 100;
$it_max = 500;
$it_step = 100;

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult_queue = ">> ./output_matrixmult_queue_".$datetime.".txt";
open(WF, $path_matrixmult_queue) or die;

for ($elems = $it_min; $elems <= $it_max; $elems += $it_step) {
    for ($proc = 0; $proc < @procs; $proc++) {
        for ($div = 0; $div < @divs; $div++) {
            $result = 250000.0;
            print "mpiexec -n @procs[$proc] ../build/Release/matrixmult_queue -size $elems -d @divs[$div]\n";
            if ($elems % @divs[$div] == 0) {
                for ($k = 0; $k < 1; $k++) {
                    $tmp = `mpiexec -n @procs[$proc] ../build/Release/matrixmult_queue -size $elems -d @divs[$div]`;
                    $result = min($result + 0.0, $tmp+0.0);
                }
            }
            print WF "| procs:", $procs[$proc], " div: ", $divs[$div], ": ", $result, " |";
            print "done matrix mult with queue for @procs[$proc] procs and $elems elems\n";
        }
        print WF "\n";
    }
}

close(WF);