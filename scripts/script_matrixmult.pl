use List::Util qw[min max];
use POSIX;
# @procs = (2, 5, 10, 17, 37);
@procs = (2, 5, 10, 17);
@cache_size = (300, 500, 1500, 4500);
$it_min = 300;
$it_max = 1800;
$it_step = 300;

$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult = ">> ./output_matrixmult_".$datetime.".txt";
open(WF, $path_matrixmult) or die;


for ($cache_it = 0; $cache_it < @cache_size; $cache_it++) {
    print WF "cache_size = ", @cache_size[$cache_it], "\n";
    for ($elems = $it_min; $elems <= $it_max; $elems += $it_step) {
        for ($proc = 0; $proc < @procs; $proc++) {
            $result = 250000.0;
            if ($elems % int(sqrt(@procs[$proc]-1)) == 0) {
                print "mpiexec -n @procs[$proc] ../build/Release/matrixmult -size $elems -cs @cache_size[$cache_it]\n";
                for ($k = 0; $k < 3; $k++) {
                    $tmp = `mpiexec -n @procs[$proc] --oversubscribe ../build/Release/matrixmult -size $elems -cs @cache_size[$cache_it]`;
                    $result = min($result + 0.0, $tmp + 0.0);
                }
                print WF @procs[$proc], ": ", $result, " ";
                print "done matrix mult for @procs[$proc] procs, $elems elems and cache_size = ",@cache_size[$cache_it],"\n";
            }
        }
        print WF "\n";
    }
}

close(WF);
