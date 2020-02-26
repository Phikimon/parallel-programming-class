#include <pthread.h>
#include <limits.h>
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>

struct thread_data {
	int       tid;
	pthread_t prev;
};

void* thr_func(void* arg);

int main(int argc, char* argv[])
{
	int r;

	if (argc != 2) {
		printf("Usage: ./main <number_of_threads>\n");
		return 1;
	}
	char *endptr = NULL;
	long N = strtol(argv[1], &endptr, 10);
	if (*endptr != '\0') {
		printf("\nFailed to convert argument to long integer\n");
		return 1;
	}
	if ((N == LONG_MIN) || (N == LONG_MAX)) {
		printf("\nOverflow/underflow of argument occured\n");
		return 1;
	}

	pthread_t* thr =(pthread_t*)calloc(N, sizeof(*thr));
	assert(thr);
	struct thread_data* thr_data = (struct thread_data*)calloc(N, sizeof(*thr_data));
	assert(thr_data);

	for (int i = 0; i < N; i++) {
		thr_data[i].tid = i;
		if (i > 0)
			thr_data[i].prev = thr[i - 1];
		r = pthread_create(&thr[i], NULL, thr_func, &thr_data[i]);
		assert((r == 0) && "pthread_create");
	}

	/* Algorithm guarantees that last pthread will exit last.
	 * Moreover, we cannot use loop with pthread_join() here
	 * since thread cannot be joined simultaneously by two
	 * threads and there could be race with created threads
	 * joining previous ones. */
	r = pthread_join(thr[N - 1], NULL);
	assert((r == 0) && "pthread_join(main)");

	return 0;
}

void* thr_func(void* arg)
{
	int r;
	struct thread_data* data = arg;
	/* If we are not the first thread to be
	 * created, wait for the previous one
	 * to exit */
	if (data->tid > 0) {
		r = pthread_join(data->prev, NULL);
		assert((r == 0) && "pthread_join(thr_func)");
	}
	/* We add one to the output to easily compare
	 * output of our program with `seq 1 N` */
	printf("%d %d\n", data->tid + 1, pthread_self());
	/* fflush() guarantees that thread
	 * won't exit until buffers are flushed,
	 * though usually putting \n causes printf()
	 * to flush buffers due to line buffering */
	fflush(stdout);

	return NULL;
}
