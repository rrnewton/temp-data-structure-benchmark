MPICC=/home/fernandes/projects/mpi/install/mvapich2-1.7rc2/bin/mpicc
CPP=g++
CC=gcc

TBB_LIB=-L/home/fernandes/tbb40_233oss/lib/intel64/cc4.1.0_libc2.4_kernel2.6.16.21 -lrt -ltbb
TBB_INC=-I/home/fernandes/tbb40_233oss/include

CFLAGS=-g -O2 -march=corei7-avx

BENCH_FLAGS=-DBENCH_N=128000000 -DNTHREADS=2

all: tbb_serial_queue tbb_queue tbb_queue_cacheline stream stream_omp

mpi: mpi.c
	$(MPICC) $(CFLAGS) -o mpi mpi.c

tbb_queue: Makefile modstream.c tbb_queue.cpp
	$(CPP) $(CFLAGS) $(TBB_INC) $(TBB_LIB) $(BENCH_FLAGS) -o tbb_queue modstream.c tbb_queue.cpp

tbb_queue_cacheline: Makefile modstream.c tbb_queue_cacheline.cpp
	$(CPP) $(CFLAGS) $(TBB_INC) $(TBB_LIB) $(BENCH_FLAGS) -o tbb_queue_cacheline modstream.c tbb_queue_cacheline.cpp

tbb_serial_queue: Makefile modstream.c tbb_serial_queue.cpp
	$(CPP) $(CFLAGS) $(TBB_INC) $(TBB_LIB) $(BENCH_FLAGS) -o tbb_serial_queue modstream.c tbb_serial_queue.cpp

stream: Makefile stream.c
	$(CC) $(CFLAGS) $(BENCH_FLAGS) -o stream stream.c

stream_omp: Makefile stream.c
	$(CC) $(CFLAGS) -fopenmp $(BENCH_FLAGS) -o stream_omp stream.c

clean:

