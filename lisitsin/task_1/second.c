#include <stdio.h>
#include <mpi.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int size = 0;
    int rank = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    char buf[5] = "PHIL";

    if (rank != 0) {
        MPI_Recv(buf, sizeof(buf), MPI_CHAR, rank - 2, MPI_ANY_TAG, MPI_COMM_WORLD, NULL);
        //fprintf(stderr, "%d/%d recv\n", rank, size);
    }

    fprintf(stderr, "%d\n", rank);

    if (rank + 1 != size) {
        //fprintf(stderr, "%d/%d send\n", rank, size);
        MPI_Send(buf, sizeof(buf), MPI_CHAR, rank + 2, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();
    return 0;
}
