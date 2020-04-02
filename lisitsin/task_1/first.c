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
    char buf[257] = "PHIL";

    if (rank != 0) {
        // Since sizeof(buf) is big enough, MPI_Send will block until
        // other side reads message
        MPI_Send(buf, sizeof(buf), MPI_CHAR, rank - 1, 0, MPI_COMM_WORLD);
    }

    fprintf(stderr, "%d\n", rank);

    if (rank + 1 != size)
        MPI_Recv(buf, sizeof(buf), MPI_CHAR, rank + 1, MPI_ANY_TAG, MPI_COMM_WORLD, NULL);

    MPI_Finalize();
    return 0;
}
