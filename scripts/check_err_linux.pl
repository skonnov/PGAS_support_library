
$output_file = ">> ./err_output.txt";

open(WF, $output_file) or die;

for ($i = 0; $i < 50000; $i++) {
    $output = `printf q | mpiexec -n 4 gdb -x script.gdb --args ../build2/Debug/sum_of_vector 50`;
    if (index($output, "SIGSEGV") != -1 || $? != 0) {
        print WF $output;
        last;
    }
}