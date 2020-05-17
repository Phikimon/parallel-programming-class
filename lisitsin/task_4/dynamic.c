#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <assert.h>
#include <mpi.h>
#include <math.h>
#include <string.h>

// Number of blocks per worker
#define GRANULARITY 2

int MPI_SIZE = 0;

enum {
	TAG_OK = 0,
	TAG_FAIL,
	TAG_DATA,
};

typedef struct {
	int begin, end;
} range_s;

typedef struct {
	int origin;
	int size;
	int speculated;
	int* term1;
	int* term2;
	int* res1;
	int* res2;
	int carry1;
	int carry2;
} task_s;

void graceful_abort(int signum)
{
	fprintf(stderr, "ERROR");
	MPI_Abort(MPI_COMM_WORLD, 1);
}

void print_array(const char* prefix, const int* arr, int size)
{
	fprintf(stderr, "%s", prefix);
	for (int i = 0; i < size; i++) {
		fprintf(stderr, "%09d", arr[i]);
	}
	fprintf(stderr, "\n");
}

// It is responsibility of the caller to free result
int* perform_task(task_s* task) {
	int* term1 = task->term1;
	int* term2 = task->term2;
	int size = task->size;

	// We only speculate if we can not constrain
	// effect of carry from less significant digits
	int will_speculate = (term1[size-1] + term2[size-1] == 999999999);
#ifdef DEBUG_PRINT
	if (will_speculate)
		fprintf(stderr, "will speculate\n");
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
		fprintf(stderr, "sum[%d] = %09d\n", i, result1[i]);
#endif
		if (will_speculate) {
			result2[i] = term1[i] + term2[i] + carry2;
			carry2 = result2[i] > 999999999;
			result2[i] %= 1000000000;
		}
	}

	task->res1 = result1;
	task->carry1 = carry1;
	task->speculated = will_speculate;
	if (will_speculate) {
		task->res2 = result2;
		task->carry2 = carry2;
	}
}

void worker_func(int rank)
{
	assert((rank <= MPI_SIZE - 1) && (rank >= 1));
	int is_task_completed = 0;
	MPI_Send(&is_task_completed, 1, MPI_INT, 0, TAG_OK, MPI_COMM_WORLD);

	MPI_Status status = {};
	while (1) {
		int task_num;
		int size;

		MPI_Recv(&task_num, 1, MPI_INT,
			0, MPI_ANY_TAG,
			MPI_COMM_WORLD, &status);
		if (status.MPI_TAG == TAG_FAIL) {
			assert(task_num == -1);
			break;
		}
		assert(task_num >= 0 && status.MPI_TAG == TAG_OK);

		MPI_Recv(&size, 1, MPI_INT,
			0, TAG_DATA,
			MPI_COMM_WORLD, NULL);
		assert(size > 0);

		int* term1 = (int*)malloc(size * sizeof(term1[0]));
		assert(term1);
		int* term2 = (int*)malloc(size * sizeof(term2[0]));
		assert(term2);

		MPI_Recv(term1, size, MPI_INT,
			0, TAG_DATA,
			MPI_COMM_WORLD, NULL);
		MPI_Recv(term2, size, MPI_INT,
			0, TAG_DATA,
			MPI_COMM_WORLD, NULL);

		// Perform task
		task_s task = {
			.size = size,
			.term1 = term1,
			.term2 = term2,
		};

		// perform_task() sets 'res1' and probably 'res2'
		// to newly allocated buffers which we have
		// to free. Also it sets 'carry1', 'carry2' and
		// 'speculated'
		perform_task(&task);
#ifdef DEBUG_PRINT
		print_array("  ", term1,     size);
		print_array(" +", term2,     size);
		print_array(" =", task.res1, size);
		fprintf(stderr, "\n");
#endif

		is_task_completed = 1;
		MPI_Send(&is_task_completed, 1,    MPI_INT, 0, TAG_OK,   MPI_COMM_WORLD);
		MPI_Send(&task_num,          1,    MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD);
		MPI_Send(&task.speculated,   1,    MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD);
		MPI_Send(&task.carry1,       1,    MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD);
		MPI_Send(task.res1,          size, MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD);
		if (task.speculated) {
			MPI_Send(&task.carry2, 1,    MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD);
			MPI_Send(task.res2,    size, MPI_INT, 0, TAG_DATA, MPI_COMM_WORLD);
		}

		free(term1);
		free(term2);
		free(task.res1);
		if (task.speculated)
			free(task.res2);
	}

}

void root_func(FILE* file1, FILE* file2)
{
	int r = 0;
	int rank = 0;
	int size1, size2;

	fscanf(file1, "%d ", &size1);
	fscanf(file2, "%d ", &size2);

	int term_size = (size1 > size2) ? size1 : size2;
	term_size = (term_size % 9) ? term_size / 9 + 1 : term_size / 9;
#ifdef DEBUG_PRINT
	fprintf(stderr, "ROOT: size1 = %d, size2 = %d\n", size1, size2);
#endif

	int* term1 = (int*)malloc(term_size * sizeof(term1[0]));
	assert(term1);
	int* term2 = (int*)malloc(term_size * sizeof(term2[0]));
	assert(term2);

	int i = 0;
	while (fscanf(file1, "%9d", &term1[i++]) == 1);

	i = 0;
	while (fscanf(file2, "%9d", &term2[i++]) == 1);

	// Distribute tasks
	int workers_num = MPI_SIZE - 1;
	int block_size = term_size / (GRANULARITY * workers_num);
	if (block_size == 0) {
		fprintf(stderr, "Too many processes for current task\n");
		assert(block_size > 0);
	}
	double start = MPI_Wtime();

	int* buf1 = (int*)malloc(term_size * sizeof(buf1[0]));
	assert(buf1);
	int* buf2 = (int*)malloc(term_size * sizeof(buf1[0]));
	assert(buf1);

	// Initialize tasks
	int tasks_num = term_size / block_size;
#ifdef DEBUG_PRINT
	fprintf(stderr, "term_size = %d(%d digits), workers_num = %d, tasks_num = %d, block_size = %d, block_num_per_worker = %d\n",
			term_size, term_size * 9, workers_num, tasks_num, block_size, GRANULARITY);
#endif
	task_s* task_pool = (task_s*)calloc(tasks_num, sizeof(*task_pool));
	for (int i = 0; i < tasks_num; i++) {
		task_pool[i].origin = i * block_size;
		task_pool[i].speculated = 0;
		task_pool[i].size   = block_size;
		task_pool[i].term1  = &term1[i * block_size];
		task_pool[i].term2  = &term2[i * block_size];
		task_pool[i].res1   = &buf1[i * block_size];
		task_pool[i].res2   = &buf2[i * block_size];
		task_pool[i].carry1 = 0;
		task_pool[i].carry2 = 0;
	}

	int assigned_tasks = 0;
	int completed_tasks = 0;
	int cur_worker = 0;
	MPI_Status status = {};
	while (completed_tasks < tasks_num) {
		int is_task_completed;
		// Worker either just entered or
		// already accomplished some task
		MPI_Recv(&is_task_completed, 1, MPI_INT,
		         MPI_ANY_SOURCE, TAG_OK,
			 MPI_COMM_WORLD, &status);
		cur_worker = status.MPI_SOURCE;

		if (is_task_completed) {
			int task_num;
			// Get results
			MPI_Recv(&task_num, 1, MPI_INT,
				cur_worker, TAG_DATA,
				MPI_COMM_WORLD, NULL);

			task_s* t = &task_pool[task_num];

			MPI_Recv(&t->speculated, 1, MPI_INT,
				cur_worker, TAG_DATA,
				MPI_COMM_WORLD, NULL);
			MPI_Recv(&t->carry1, 1, MPI_INT,
				cur_worker, TAG_DATA,
				MPI_COMM_WORLD, NULL);
			MPI_Recv(t->res1, t->size, MPI_INT,
				cur_worker, TAG_DATA,
				MPI_COMM_WORLD, NULL);
			if (task_pool[task_num].speculated) {
				MPI_Recv(&t->carry2, 1, MPI_INT,
					cur_worker, TAG_DATA,
					MPI_COMM_WORLD, NULL);
				MPI_Recv(t->res2, task_pool[task_num].size, MPI_INT,
					cur_worker, TAG_DATA,
					MPI_COMM_WORLD, NULL);
			}
			completed_tasks++;
		}

		if (assigned_tasks < tasks_num) {
			int task_num = assigned_tasks;
			task_s* t = &task_pool[task_num];
			// Assign new task
			MPI_Send(&task_num, 1, MPI_INT,
				cur_worker, TAG_OK,
				MPI_COMM_WORLD);
			MPI_Send(&t->size, 1, MPI_INT,
				cur_worker, TAG_DATA,
				MPI_COMM_WORLD);
			MPI_Send(t->term1, t->size, MPI_INT,
				cur_worker, TAG_DATA,
				MPI_COMM_WORLD);
			MPI_Send(t->term2, t->size, MPI_INT,
				cur_worker, TAG_DATA,
				MPI_COMM_WORLD);
			assigned_tasks++;
		} else {
			int placeholder = -1;
			MPI_Send(&placeholder, 1, MPI_INT,
				cur_worker, TAG_FAIL,
				MPI_COMM_WORLD);
		}
	}

	int* result = (int*)malloc(term_size * sizeof(result[0]));
	assert(result);

	int carry = 0;
	for (int i = tasks_num - 1; i >= 0; i--) {
		task_s* t = &task_pool[i];

		if ((carry == 1) && (t->speculated)) {
			memcpy(&result[t->origin], t->res2,
			       t->size * sizeof(int));
			carry = t->carry2;
			continue;
		}

		memcpy(&result[t->origin], t->res1,
		       t->size * sizeof(int));
		if (carry == 1) {
			// Our algorithm guarantees that if
			// t%d: here was no speculation then
			// effect of carry being equal to one
			// is constrained to the rightmost int
			result[t->origin + t->size - 1]++;
		}
		carry = t->carry1;
	}

#ifdef DEBUG_PRINT
	fprintf(stderr, "ROOT: carry = %d\n", carry);
#endif

#ifdef DEBUG_PRINT
	printf("     %s", carry ? " " : "");
	for (int i = 0; i < term_size; i++)
		printf("%09d", term1[i]);
	printf("\n  +\n");
	printf("     %s", carry ? " " : "");
	for (int i = 0; i < term_size; i++)
		printf("%09d", term2[i]);
	printf("\n");
#endif
	printf("Sum: %s", carry ? "1" : "");
	for (int i = 0; i < term_size; i++)
		printf("%09d", result[i]);
	printf("\nTime elapsed: %lg\n", MPI_Wtime() - start);
	fflush(stdout);

	free(result);
	free(term1);
	free(term2);
	free(buf1);
	free(buf2);
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
