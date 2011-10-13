MPICC=/home/fernandes/projects/mpi/install/mvapich2-1.7rc2/bin/mpicc
CPP=g++
CC=gcc

TBB_LIB=-L/home/fernandes/projects/dsl/src/tbb40_233oss/build/linux_intel64_gcc_cc4.4.6_libc2.13_kernel2.6.38.4_release -lrt -ltbb
TBB_INC=-I/home/fernandes/projects/dsl/src/tbb40_233oss/include

CFLAGS=-g -O2

all: tbb_queue tbb_queue_cacheline stream stream_omp

mpi: mpi.c
	$(MPICC) $(CFLAGS) -o mpi mpi.c

tbb_queue: modstream.c tbb_queue.cpp
	$(CPP) $(CFLAGS) $(TBB_INC) $(TBB_LIB) -DBENCH_N=4000000 -o tbb_queue modstream.c tbb_queue.cpp

tbb_queue_cacheline: modstream.c tbb_queue_cacheline.cpp
	$(CPP) $(CFLAGS) $(TBB_INC) $(TBB_LIB) -DBENCH_N=4000000 -o tbb_queue_cacheline modstream.c tbb_queue_cacheline.cpp
	
stream: stream.c
	$(CC) $(CFLAGS) -DN=4000000 -o stream stream.c

stream_omp: stream.c
	$(CC) $(CFLAGS) -fopenmp -DN=4000000 -o stream_omp stream.c
