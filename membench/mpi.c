#include <string.h>
#include <stdio.h>
#include "mpi.h"

#define NITER 4096*1024

void producer() {
	int i;
	int buf[32];
	for(i = 0; i < NITER; i++) {
		memset(buf, i, sizeof(int)*32);
		MPI_Send(buf, 32, MPI_INT, 1, 0, MPI_COMM_WORLD);
	}

	send x
}

void consumer() {
	int i;
	int buf[32];
	for(i = 0; i < NITER; i++) {
		MPI_Recv(buf, 32, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		fprintf(stdout, "received %d %d %d ...\n", buf[0], buf[5], buf[10]);
		fflush(stdout);
	}

	get x cmp y
}

int main(int argc, char **argv) {
	int rank;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	// producer
	if (!rank)
		producer();
	else
		consumer();

	MPI_Finalize();

	return 0;
}
