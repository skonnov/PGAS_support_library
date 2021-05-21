use List::Util qw[min max];
use POSIX;

$it_min = 2500;
$it_max = 10001;
$it_step = 500;

$min_proc = 2, $max_proc = 32;

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_dijkstra = ">> ./output_dijkstra_".$datetime.".txt";

open(WF, $path_dijkstra) or die;

for ($i = $it_min; $i < $it_max; $i += $it_step) {
    $v = $i;
    for ($j = $min_proc; $j <= $max_proc; $j = $j*2) {
        $result = 250000.0;
        for ($k = 0; $k < 3; $k++) {
            $tmp = `mpiexec -n $j ../build/Release/dijkstra -v $v -max 150`;
            $result = min($result + 0.0, $tmp+0.0);
        }
        print "dijkstra algorithm for v = $v, $j procs is done\n";
        print WF $result, " ";
    }
    print WF "\n";
}

close(WF);
