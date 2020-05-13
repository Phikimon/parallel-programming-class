#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <math.h>
#include <string.h>

#define WORK_QUANT 9

int MPI_SIZE = 0;

void graceful_abort(int signum)
{
	fprintf(stderr, "ERROR");
	MPI_Abort(MPI_COMM_WORLD, 1);
}

typedef struct {
	int begin, end;
} range_s;

// It is responsibility of the caller to free result
char* sum_block(char* string1, char* string2, range_s rg, int rank) {
	int a, b;

	char tmp = string1[9];
	string1[9] = '\0';
	a = atoi(string1);
	string1[9] = tmp;

	tmp = string2[9];
	string2[9] = '\0';
	b = atoi(string2);
	string2[9] = tmp;

	assert((rank <= MPI_SIZE - 1) && (rank >= 1));
	int will_speculate = (rank != MPI_SIZE - 1);

	int carry1_in = 0, carry2_in = 1;
	int carry1_out, carry2_out;

	int digits_num = rg.end - rg.begin;
	char* result1 = (char*)malloc(digits_num + sizeof((char)'\0'));
	assert(result1);
	result1[digits_num] = '\0';

	char* result2;
	if (will_speculate) {
		result2 = (char*)malloc(digits_num + sizeof((char)'\0'));
		assert(result2);
		result2[digits_num] = '\0';
	}

	for (int i = 0; i < digits_num; i += WORK_QUANT) {
		int tmp1, tmp2;

		int end_of_block = i + WORK_QUANT;
		if (i + WORK_QUANT > digits_num)
			end_of_block = digits_num;

		tmp1 = string1[end_of_block];
		string1[end_of_block] = '\0';
		a = atoi(&string1[i]);
		string1[end_of_block] = tmp1;

		tmp2 = string2[end_of_block];
		string2[end_of_block] = '\0';
		b = atoi(&string2[i]);
		string2[end_of_block] = tmp2;

		int sum1 = a + b + carry1_in;
		sprintf(result1 + i, "%09d", sum1 % 1000000000);
		carry1_out = sum1 > 999999999;

		if (will_speculate) {
			int sum2 = a + b + carry2_in;
			sprintf(result2 + i, "%09d", sum2 % 1000000000);
			carry2_out = sum2 > 999999999;
		}

	}

	int carry_in = carry1_in;
	char* result = result1;
	int carry_out = carry1_out;

	if (rank < MPI_SIZE - 1) {
		MPI_Recv(&carry_in, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, NULL);

		if (carry_in) {
			result = result2;
			free(result1); result1 = NULL;
			carry_out = carry2_out;
		} else {
			result = result1;
			free(result2); result2 = NULL;
			carry_out = carry1_out;
		}
	}

	MPI_Send(&carry_out, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD);
	return result;
}

void root_func(FILE* file1, FILE* file2)
{
	int r = 0;
	int rank = 0;
	int size1, size2;

	fscanf(file1, "%d ", &size1);
	fscanf(file2, "%d ", &size2);

	int total_size = (size1 > size2) ? size1 : size2;
	if (total_size % 9 != 0)
		total_size += 9 - (total_size % 9);
	//fprintf(stderr, "size1 = %d, size2 = %d\n", size1, size2);

	char* string1 = (char*)malloc(total_size + sizeof((char)'\0'));
	assert(string1);
	char* string2 = (char*)malloc(total_size + sizeof((char)'\0'));
	assert(string2);

	memset(string1, '0', total_size); string1[total_size] = '\0';
	fread(string1 + (total_size - size1), 1, size1, file1);

	memset(string2, '0', total_size); string2[total_size] = '\0';
	fread(string2 + (total_size - size2), 1, size2, file2);

	int workers_num = MPI_SIZE - 1;
	// number of block to add
	int work_quota = total_size / (workers_num * WORK_QUANT);
	if (work_quota == 0) {
		fprintf(stderr, "Too many processes for current task\n");
		assert(work_quota > 0);
	}
	double start = MPI_Wtime();
	for (int i = MPI_SIZE - 1; i >= 1; i--) {
		range_s rg;
		rg.begin = work_quota * WORK_QUANT * (i - 1);
		rg.end   = rg.begin + work_quota * WORK_QUANT;
		if (i == MPI_SIZE  - 1)
			rg.end = total_size;

		// Send info about amount of work to do
		MPI_Send(&rg, sizeof(rg)/sizeof(int), MPI_INT, i, 0, MPI_COMM_WORLD);
		// Send input for the work
		MPI_Send(string1 + rg.begin, rg.end - rg.begin, MPI_CHAR, i, 0, MPI_COMM_WORLD);
		MPI_Send(string2 + rg.begin, rg.end - rg.begin, MPI_CHAR, i, 0, MPI_COMM_WORLD);
	}

	int carry = 0;
	MPI_Recv(&carry, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, NULL);
	assert(carry == 0 || carry == 1);

	char* result_string = (char*)malloc(total_size + sizeof((char)'\0'));
	assert(result_string);
	result_string[total_size] = '\0';
	int digits_num = 0;
	char* ptr = result_string;
	for(int i = 1; i < MPI_SIZE;
		ptr += digits_num, i++) {
		MPI_Recv(&digits_num, 1, MPI_INT, i, 0, MPI_COMM_WORLD, NULL);
		MPI_Recv(ptr, digits_num, MPI_CHAR, i, 0, MPI_COMM_WORLD, NULL);
	}
	assert(ptr == result_string + total_size);

	printf("Sum: %s%s\n", carry ? "1" : "", result_string);
	printf("Time elapsed: %lg\n", MPI_Wtime() - start);
	fflush(stdout);

	free(result_string);
	free(string1);
	free(string2);
}

void worker_func(int rank)
{
	range_s rg;
	MPI_Recv(&rg, sizeof(rg)/sizeof(int), MPI_INT, 0, 0, MPI_COMM_WORLD, NULL);

	int digits_num = rg.end - rg.begin;

	char* string1 = (char*)malloc(digits_num + 1);
	string1[digits_num] = '\0';
	assert(string1);
	char* string2 = (char*)malloc(digits_num + 1);
	string2[digits_num] = '\0';
	assert(string2);

	MPI_Recv(string1, digits_num, MPI_CHAR, 0, 0, MPI_COMM_WORLD, NULL);
	MPI_Recv(string2, digits_num, MPI_CHAR, 0, 0, MPI_COMM_WORLD, NULL);
	//fprintf(stderr, "string1 = %s, string2 = %s\n", string1, string2);

	// It is our responsibility to free result
	char* result = sum_block(string1, string2, rg, rank);

	MPI_Send(&digits_num, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
	MPI_Send(result, digits_num, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

	free(result);
	free(string1);
	free(string2);
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
