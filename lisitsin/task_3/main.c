#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <limits.h>

#define M_PI 3.14159265358979323846

struct range {
	int start;
	int end;
};

int read_int(const char* str, int* res);
double calculate(int start, int end);
int range_start(int range_overall, int size, int num);
int range_end(int range_overall, int size, int num);

int main(int argc, char* argv[])
{
	MPI_Init(&argc, &argv);

	int mpi_rank = 0;
	int mpi_size = 0;
	MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

	if (argc != 2) {
		fprintf(stderr, "Usage: mpirun %s <N>\n", argv[0]);
		return 1;
	}

	int range_overall;
	if (read_int(argv[1], &range_overall) != 0)
		return 1;

	struct range* ranges = NULL;
	double* results = NULL;
	if (mpi_rank == 0) {
		results = (double*)calloc(mpi_size, sizeof(*results));
		assert(results);
		ranges = (struct range*)calloc(mpi_size, sizeof(*ranges));
		assert(ranges);

		for (int i = 0; i < mpi_size; i++) {
			ranges[i].start = range_start(range_overall, mpi_size, i) + 1;
			ranges[i].end   = range_end  (range_overall, mpi_size, i) + 1;
		}
	}

	MPI_Barrier(MPI_COMM_WORLD);

	double start_time = 0;
	if (mpi_rank == 0)
		start_time = MPI_Wtime();

	struct range my_range;
	MPI_Scatter(ranges, 2, MPI_INT, &my_range, 2, MPI_INT, 0, MPI_COMM_WORLD);

	double r;
	r = calculate(my_range.start, my_range.end);

	MPI_Gather(&r, 1, MPI_DOUBLE, results, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

	MPI_Barrier(MPI_COMM_WORLD);

	double sum = 0;
	if (mpi_rank == 0) {
		printf("Time elapsed: %lg\n", MPI_Wtime() - start_time);

		// Values at the end of the array are smallest
		for (int i = 0; i < mpi_size; i++)
			sum += results[mpi_size - 1 - i];

		printf("Sum of series: %lg\n", sum);

		free(results);
		free(ranges);
	}


	MPI_Finalize();
	return 0;
}

double calculate(int start, int end)
{
	assert(end >= start);
	double val = 0;
	int i;
	// Start from smallest values
	for (i = end; i >= start; i--)
		val += 6. / M_PI / M_PI / i / i;
	return val;
}

int range_start(int range_overall, int size, int num)
{
	if (num < range_overall % size)
		return num * (range_overall / size + 1);
	else
		return (range_overall % size) * (range_overall / size + 1) +
		       (num - range_overall % size) * (range_overall / size);
}

int range_end(int range_overall, int size, int num)
{
	int start = range_start(range_overall, size, num);
	int len;
	if (num < range_overall % size)
		len = range_overall / size + 1;
	else
		len = range_overall / size;
	return start + len - 1;
}

int read_int(const char* str, int* res)
{
	long val;
	char* endptr;
	val = strtol(str, &endptr, 10);
	if ((*str == '\0') || (*endptr != '\0')) {
		fprintf(stderr, "\nFailed to convert string to long integer\n");
		return 1;
	}
	if ((val == LONG_MIN) || (val == LONG_MAX)) {
		fprintf(stderr, "\nOverflow/underflow occured\n");
		return 1;
	}
	if (val < 0) {
		fprintf(stderr, "\nArgument shall be greater than zero(val = %ld)\n", val);
		return 1;
	}
	*res = (int)val;
	return 0;
}
