#include <mpi.h>
#include <float.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

#define min(x, y) ((x > y) ? (y) : (x))
#define max(x, y) ((x > y) ? (x) : (y))

#define BUF_SIZE 4
#define ITER_NUM 1000

double WTICK;

#define test_func(NAME, CODE)                                                     \
void test_##NAME(int mpi_rank, int mpi_size)                                      \
{                                                                                 \
    double times[ITER_NUM] = {};                                                  \
                                                                                  \
    { CODE };                                                                     \
                                                                                  \
    if(!mpi_rank) {                                                               \
        double sum = 0;                                                           \
        for(int i = 0; i < ITER_NUM; i++)                                         \
            sum += times[i];                                                      \
        double aver = sum/ITER_NUM;                                               \
        double stdderiv = 0;                                                      \
        for(int i = 0; i < ITER_NUM; i++)                                         \
            stdderiv += (times[i] - aver) * (times[i] - aver);                    \
        stdderiv = sqrt(stdderiv/ITER_NUM);                                       \
        printf("%-12s| average elapsed %12lg | stdderiv = %12lg = %4d * Wtick\n", \
                     "MPI_"#NAME, aver, stdderiv, (int)(stdderiv/WTICK));         \
    }                                                                             \
}

test_func(Bcast,
{
    int array[BUF_SIZE] = {};
    if(!mpi_rank) {
        for(int i = 0; i < BUF_SIZE; i++) {
            array[i] = i;
        }
    }

    for(int i = 0; i < ITER_NUM; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        times[i] -= MPI_Wtime();
        MPI_Bcast(array, BUF_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        times[i] += MPI_Wtime();
    }
})

test_func(Reduce,
{
    int array[BUF_SIZE] = {};
    for(int i = 0; i < BUF_SIZE; i++) {
        array[i] = mpi_rank + i;
    }

    int result[BUF_SIZE] = {};
    for(int i = 0; i < ITER_NUM; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        times[i] -= MPI_Wtime();
        MPI_Reduce(array, result, BUF_SIZE, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        times[i] += MPI_Wtime();
    }
})

test_func(Scatter,
{
    int send_buf[BUF_SIZE * mpi_size];
    if(0 == mpi_rank) {
        for(int i = 0; i < sizeof(send_buf) / sizeof(int); i++) {
            send_buf[i] = i;
        }
    }
    int recv_buf[BUF_SIZE] = {};

    for(int i = 0; i < ITER_NUM; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        times[i] -= MPI_Wtime();
        MPI_Scatter(send_buf, BUF_SIZE, MPI_INT, recv_buf, BUF_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        times[i] += MPI_Wtime();
    }
})

test_func(Gather,
{
    int send_buf[BUF_SIZE] = {};
    for(int i = 0; i < sizeof(send_buf) / sizeof(int); i++) {
        send_buf[i] = mpi_rank + i;
    }
    int recv_buf[BUF_SIZE * mpi_size];

    for(int i = 0; i < ITER_NUM; i++) {
        MPI_Barrier(MPI_COMM_WORLD);
        times[i] -= MPI_Wtime();
        MPI_Gather(send_buf, BUF_SIZE, MPI_INT, recv_buf, BUF_SIZE, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        times[i] += MPI_Wtime();
    }
})

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int mpi_rank = 0;
    int mpi_size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    if (mpi_size != BUF_SIZE) {
        if (!mpi_rank)
            fprintf(stderr, "\nError: BUF_SIZE = %d != %d = mpi_size\n",
                    BUF_SIZE, mpi_size);
        return 1;
    }

    if (!mpi_rank) {
        WTICK = MPI_Wtick();
        printf("MPI_Wtick() = %lg\n\n", WTICK);
    }

    test_Bcast  (mpi_rank, mpi_size);
    test_Reduce (mpi_rank, mpi_size);
    test_Scatter(mpi_rank, mpi_size);
    test_Gather (mpi_rank, mpi_size);

    MPI_Finalize();
    return 0;
}
