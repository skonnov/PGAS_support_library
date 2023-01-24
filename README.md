![CI status](https://github.com/skonnov/PGAS_support_library/actions/workflows/ci.yml/badge.svg)

# PGAS Suppor Library (PGAS_SL)

**PGAS_SL** is the library that helps to implement parallel algorithms using [Partitioned Global Address Space (PGAS)](https://en.wikipedia.org/wiki/Partitioned_global_address_space) paradigm.

## Requirements:
* CMake >= 3.0
* MPI - MPICH, OpenMPI or Intel MPI. MSMPI may be unstable
* C++
### To run scripts:
* perl
* python 3+

## How to build:
* mkdir build
* cd build
* cmake ..
* cmake --build . [--config Debug/Release] [-j number_of_threads]

## How to run samples:
* mpiexec -n \<number_of_processes\> \<path_to_sample\>/\<sample_name\> \<arguments\>

#
![logo](./PGAS.png)
Many thanks to [@Left841](https://github.com/left841) for logo!
