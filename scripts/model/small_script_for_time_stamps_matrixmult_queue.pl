use POSIX;
use List::Util qw[min max];

@matrix_sizes = (240, 300);
# @matrix_sizes = (240, 300, 360, 600);

@cache_sizes = (200, 500, 1000, 10000);

# @procs = (2, 5, 10);
@procs = (3, 4, 8);
# @procs = (2, 5, 10, 17, 37);


$datetime = strftime "%Y-%m-%d-%H-%M-%S", localtime time;
$path_matrixmult = ">> ./output_model_time_stamps_".$datetime.".txt";
open(WF, $path_matrixmult) or die;


print WF "procs_it matrix_it cache_it: result_lru result_clusters\n";
for ($matrix_it = 0; $matrix_it < @matrix_sizes; $matrix_it++) {
    for ($cache_it = 0; $cache_it < @cache_sizes; $cache_it++) {
        for ($procs_it = 0; $procs_it < @procs; $procs_it++) {
            $result_lru = 100000., $result_clusters = 100000.;
            $matrix_size = @matrix_sizes[$matrix_it];
            $cache_size = @cache_sizes[$cache_it];
            $proc_num = @procs[$procs_it];
            for ($i = 0; $i < 3; $i++) {
                print "mpiexec --oversubscribe -n $proc_num ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size\n";
                $tmp = `mpiexec --oversubscribe -n $proc_num ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size`;
                print $tmp "\n";
                $result_lru = min($result_lru + 0.0, $tmp + 0.0);
                $output_timestamps_file = "statistics_output/output_timestamps_matrixmult_queue_proc_$proc_num\_proc_num_$matrix_size\_cache_size_$cache_size.txt";
                if ($i == 0) {
                    print "python ../../model/time_stamps.py -path ./statistics_output -output $output_timestamps_file\n";
                    `python ../../model/time_stamps.py -path ./statistics_output -output $output_timestamps_file`;
                }
                print "mpiexec --oversubscribe -n $proc_num ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size -stat 0 $output_timestamps_file\n";
                $tmp = `mpiexec --oversubscribe -n $proc_num ../../build_linux/Release/matrixmult_queue -size $matrix_size -quantum_size 20 -d 5 -cs $cache_size -stat 0 $output_timestamps_file`;
                print $tmp "\n";
                $result_clusters = min($result_clusters + 0.0, $tmp + 0.0);
            }
            print WF $proc_num, " ", $matrix_size, " ", $cache_size, ": ", $result_lru, " ", $result_clusters, "\n";
        }
    }
}
