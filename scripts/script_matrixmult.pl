use List::Util qw[min max];
use POSIX;
@procs = (2, 5, 10, 17, 37);

$it_min = 300;
$it_max = 1800;
$it_step = 100;

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult = ">> ./output_matrixmult_".$datetime.".txt";
open(WF, $path_matrixmult) or die;

for ($elems = $it_min; $elems <= $it_max; $elems += $it_step) {
    for ($proc = 0; $proc < @procs; $proc++) {
        $result = 250000.0;
        if ($elems % int(sqrt(@procs[$proc]-1)) == 0) {
            print "mpiexec -n @procs[$proc] ../build/Release/matrixmult -size $elems";
            for ($k = 0; $k < 3; $k++) {
                $tmp = `mpiexec -n @procs[$proc] ../build/Release/matrixmult -size $elems`;
                $result = min($result + 0.0, $tmp+0.0);
            }
            print WF $result;
            print WF " ";
            print "done matrix mult for @procs[$proc] procs and $elems elems\n";
        }
    }
    print WF "\n";
}

close(WF);
