use List::Util qw[min max];

open(WF2, ">> ./output_reduce_multiple_vector_matrix.txt") or die;

for($i = 10000; $i <= 25000; $i+=1000) {
    for($l = 5000; $l <= 25000; $l+=5000) {
        print WF2 $i, " ", $l, " ";
        for($j = 2; $j <= 4; $j = $j*2) {
            $result = 250000.0;
            for($k = 0; $k < 5; $k++) {
                $tmp = `mpiexec -n $j ../build/Release/matrixvector $i $l`;
                $result = min($result + 0.0, $tmp + 0.0);
            }
            print WF2 $result;
            print WF2 " ";
        }
        print " | ";
    }
	print WF2 "\n";
}
close(WF2);

print "Done matrixvector\n";