use List::Util qw[min max];

open(WF, "> ./output_vector_sum_of_vector.txt") or die;
for($i = 100000000; $i <= 250000000; $i+=50000000) {
    print WF $i;
    print WF " ";
    for($j = 2; $j <= 4; $j = $j*2) {
        $result = `mpiexec -n $j ../build/Release/main $i`;
		print WF $result;
		print WF " ";
	}
	print WF "\n";
}
close(WF);

print "Done parallel_vector\n";

open(WF2, "> ./output_reduce_sum_of_vector.txt") or die;
for($i = 100000000; $i <= 1000000000; $i+=10000000) {
    print WF2 $i;
    print WF2 " ";
    for($j = 2; $j <= 4; $j = $j*2) {
        if($i / $j <= 250000000) {
            $result = 250000.0;
            for($k = 0; $k < 5; $k++) {
                $tmp = `mpiexec -n $j ../build/Release/main_parallel_for $i`;
                $result = min($result + 0.0, $tmp+0.0);
            }
            print WF2 $result;
            print WF2 " ";
        }
        else {
            print WF2 "Too_much ";
        }
	}
	print WF2 "\n";
}
close(WF2);

print "Done parallel_for\n";

use List::Util qw[min max];

open(WF2, "> ./output_reduce_multiple_vector_matrix.txt") or die;

for($i = 10000; $i <= 25000; $i+=1000) {
    for($l = 5000; $l <= 25000; $l+=5000) {
        print WF2 $i, " ", $l, " ";
        for($j = 2; $j <= 4; $j = $j*2) {
            $result = 250000.0;
            for($k = 0; $k < 5; $k++) {
                $tmp = `mpiexec -n $j ../build/Release/matrixvector $i $l`;
                $result = min($result + 0.0, $tmp+0.0);
            }
            print WF2 $result;
            print WF2 " ";
        }
        print WF " | ";
    }
	print WF2 "\n";
}
close(WF2);

print "Done matrixvector\n";

