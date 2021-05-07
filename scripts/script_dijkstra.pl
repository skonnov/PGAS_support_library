use List::Util qw[min max];
use POSIX;

$it_min = 500;
$it_max = 5001;
$it_step = 500;

$min_proc = 2, $max_proc = 32;

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_dijkstra = ">> ./output_dijkstra_".$datetime.".txt";

open(WF, $path_dijkstra) or die;

for ($i = $it_min; $i < $it_max; $i += $it_step) {
    $n = $i;
    $m = $n * ($n-1) / 2;
    for ($j = $min_proc; $j <= $max_proc; $j = $j*2) {
        $result = 250000.0;
        for ($k = 0; $k < 3; $k++) {
            $tmp = `mpiexec -n $j ../build/Debug/dijkstra $n $m 150 0`;
            $result = min($result + 0.0, $tmp+0.0);
        }
        print "dijkstra algorithm for n = $n, m = $m, $j procs is done\n";
        print WF $result, " ";
    }
    print WF "\n";
}

close(WF);
