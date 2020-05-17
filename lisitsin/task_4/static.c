#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <math.h>
#include <string.h>

int MPI_SIZE = 0;

typedef struct {
	int begin, end;
} range_s;

void graceful_abort(int signum)
{
	fprintf(stderr, "ERROR");
	MPI_Abort(MPI_COMM_WORLD, 1);
}

void add_one(int* arr, int size)
{
	int carry = 1;
	for (int i = size - 1; i >= 0; i--) {
		arr[i] += carry;
		carry = arr[i] > 999999999;
		arr[i] %= 1000000000;
	}
}

// It is responsibility of the caller to free result
int* sum_block(int* term1, int* term2, int size, int rank) {
	assert((rank <= MPI_SIZE - 1) && (rank >= 1));
	assert(size > 0);

	// We only speculate if we can not constrain
	// effect of carry from less significant digits
	int will_speculate = (rank != MPI_SIZE - 1) &&
	                     (term1[size-1] + term2[size-1] == 999999999);
#ifdef DEBUG_PRINT
	if (will_speculate)
		fprintf(stderr, "%d: will speculate\n", rank);
#endif

	int* result1 = (int*)malloc(size * sizeof(int));
	assert(result1);

	int* result2;
	if (will_speculate) {
		result2 = (int*)malloc(size * sizeof(int));
		assert(result2);
	}

	int carry1 = 0, carry2 = 1;

	for (int i = size - 1; i >= 0; i--) {

		result1[i] = term1[i] + term2[i] + carry1;
		carry1 = result1[i] > 999999999;
		result1[i] %= 1000000000;
#ifdef DEBUG_PRINT
		fprintf(stderr, "%d: sum = %09d\n", rank, result1[i]);
#endif
		if (will_speculate) {
			result2[i] = term1[i] + term2[i] + carry2;
			carry2 = result2[i] > 999999999;
			result2[i] %= 1000000000;
		}
	}

	int carry_in = 0;
	int* result = result1;
	int carry_out = carry1;

	if (rank != MPI_SIZE - 1) {
		MPI_Recv(&carry_in, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);

		if ((carry_in == 1) && (will_speculate)) {
			result = result2;
			free(result1); result1 = NULL;
			carry_out = carry2;
		} else {
			if (carry_in == 1)
				add_one(result1, size);

			result = result1;
			free(result2); result2 = NULL;
			carry_out = carry1;
		}
	}

#ifdef DEBUG_PRINT
	fprintf(stderr, "%d: carry_out = %d\n", rank, carry_out);
#endif
	assert(carry_out == 1 || carry_out == 0);
	MPI_Send(&carry_out, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
	return result;
}

void worker_func(int rank)
{
	range_s range;
	MPI_Recv(&range, sizeof(range)/sizeof(int), MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);

	int size = range.end - range.begin;

	int* term1 = (int*)malloc(size * sizeof(term1[0]));
	assert(term1);
	int* term2 = (int*)malloc(size * sizeof(term2[0]));
	assert(term2);

	MPI_Recv(term1, size, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);
	MPI_Recv(term2, size, MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);

#ifdef DEBUG_PRINT
	fprintf(stderr, "%d: term1 = ", rank);
	for (int i = 0; i < size; i++) {
		fprintf(stderr, "%09d", term1[i]);
	}
	fprintf(stderr, "\n%d: term2 = ", rank);
	for (int i = 0; i < size; i++) {
		fprintf(stderr, "%09d", term2[i]);
	}
	fprintf(stderr, "\n");
#endif

	// It is our responsibility to free result
	int* result = sum_block(term1, term2, size, rank);

	MPI_Send(&size, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(result, size, MPI_INT, 0, 0, MPI_COMM_WORLD);

	free(result);
	free(term1);
	free(term2);
}

void root_func(FILE* file1, FILE* file2)
{
	int r = 0;
	int rank = 0;
	int size1, size2;

	fscanf(file1, "%d ", &size1);
	fscanf(file2, "%d ", &size2);

	int array_size = (size1 > size2) ? size1 : size2;
	array_size = (array_size % 9) ? array_size / 9 + 1 : array_size / 9;
#ifdef DEBUG_PRINT
	fprintf(stderr, "ROOT: size1 = %d, size2 = %d\n", size1, size2);
#endif

	int* array1 = (int*)malloc(array_size * sizeof(array1[0]));
	assert(array1);
	int* array2 = (int*)malloc(array_size * sizeof(array2[0]));
	assert(array2);

	int i = 0;
	while (fscanf(file1, "%9d", &array1[i++]) == 1);

	i = 0;
	while (fscanf(file2, "%9d", &array2[i++]) == 1);

	int workers_num = MPI_SIZE - 1;
	int work_quota = array_size / workers_num;
	if (work_quota == 0) {
		fprintf(stderr, "Too many processes for current task\n");
		assert(array_size > workers_num);
	}
#ifdef DEBUG_PRINT
	fprintf(stderr, "arr_size = %d, work_num = %d, quota = %d\n", array_size, workers_num, work_quota);
#endif
	double start = MPI_Wtime();
	for (int i = MPI_SIZE - 1; i >= 1; i--) {
		range_s range;
		range.begin = work_quota * (i - 1);
		range.end   = range.begin + work_quota;
#ifdef DEBUG_PRINT
		fprintf(stderr, "%d:%d\n", range.begin, range.end);
#endif

		// Send info about amount of work to do
		MPI_Send(&range, sizeof(range)/sizeof(int), MPI_INT, i, 0, MPI_COMM_WORLD);
		// Send input for the work
		MPI_Send(array1 + range.begin, work_quota, MPI_INT, i, 0, MPI_COMM_WORLD);
		MPI_Send(array2 + range.begin, work_quota, MPI_INT, i, 0, MPI_COMM_WORLD);
	}

	int carry = 0;
	MPI_Recv(&carry, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, NULL);
#ifdef DEBUG_PRINT
	fprintf(stderr, "ROOT: carry_in = %d\n", carry);
#endif
	assert(carry == 0 || carry == 1);

	int* result = (int*)malloc(array_size * sizeof(int));
	assert(result);

	int work_completed = 0;
	int* ptr = result;
	for (int i = 1; i < MPI_SIZE; ptr += work_completed, i++) {
		MPI_Recv(&work_completed, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(ptr, work_completed, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
	}
	assert(ptr == result + array_size);

#ifdef DEBUG_PRINT
	printf("     %s", carry ? " " : "");
	for (int i = 0; i < array_size; i++)
		printf("%09d", array1[i]);
	printf("\n  +\n");
	printf("     %s", carry ? " " : "");
	for (int i = 0; i < array_size; i++)
		printf("%09d", array2[i]);
	printf("\n");
#endif
	printf("Sum: %s", carry ? "1" : "");
	for (int i = 0; i < array_size; i++)
		printf("%09d", result[i]);
	printf("\nTime elapsed: %lg\n", MPI_Wtime() - start);
	fflush(stdout);

	free(result);
	free(array1);
	free(array2);
}

int main(int argc, char* argv[])
{
	signal(SIGABRT, &graceful_abort);

	MPI_Init(&argc, &argv);
	if (argc != 3) {
		fprintf(stderr, "usage: %s file_with_number1 file_with_number2\n", argv[0]);
		return 1;
	}

	int r = 0;
	int rank = 0;

	MPI_Comm_size(MPI_COMM_WORLD, &MPI_SIZE);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	if (rank == 0) {
		FILE* file1 = fopen(argv[1], "r");
		assert(file1);
		FILE* file2 = fopen(argv[2], "r");
		assert(file2);

		root_func(file1, file2);

		fclose(file1);
		fclose(file2);
	} else
		worker_func(rank);

	MPI_Finalize();
	return 0;
}
