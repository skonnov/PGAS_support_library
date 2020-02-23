use List::Util qw[min max];

open(WF2, ">> ./output_single_sum_of_vector2.txt") or die;
for($i = 100000000; $i <= 200000000; $i+=10000000) {
    print WF2 $i;
    print WF2 " ";
    $result = 250000.0;
    for($k = 0; $k < 5; $k++) {
        $tmp = `mpiexec -n 1 ../build/Release/single_sum_of_vector $i`;
        $result = min($result + 0.0, $tmp+0.0);
    }
    print WF2 $result;
    print WF2 " ";
	print WF2 "\n";
}
close(WF2);