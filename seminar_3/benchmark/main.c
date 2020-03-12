// for usleep()
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
// for clock_gettime()
#define _POSIX_C_SOURCE 199309L

#include <sys/sysinfo.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <limits.h>
#include <stdint.h>
#include <unistd.h>

#include "lock.h"

// Global lock is the spin/ticket-lock being benchmarked
void* global_lock;
// Global counter is incremented by each thread that
// is in the critical section protected by global_lock.
// main() verifies that at the end of the program global_cnt
// is equal to iter_num*thread_num. So global_cnt is used to verify
// mutual exclusion guaranteed by global_lock.
volatile long  global_cnt = 0;

#define atomic_store(ptr, val)    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_load(ptr)          __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_fetch_and_dec(ptr) ((int32_t)(__atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST)))
#define atomic_fetch_and_inc(ptr) ((int32_t)(__atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST)))

// Atomic that is used to implement barrier.
// pthread_create() makes pretty heavy syscall, so first
// threads experience milder contention than next ones,
// because their contenders are not spawned yet at the
// moment of lock acquisition attempt.
// This barrier attempts to equalize threads: they may
// start contending for global_lock only when all
// threads have been started and main thread releases
// this barrier.
volatile int32_t global_barrier;
// Atomic that is used to enhance accuracy of overall
// execution time measurement. Instead of waiting for
// threads' resources free in pthread_join() we will
// wait for all threads to increment global_atomic_cnt
// after finishing their job and immediately after that
// note finish time.
volatile int32_t global_atomic_cnt;

void* thread_work(void* arg);
long read_long(long* res, const char* str);

#define DELTA_TIMESPEC_US(END, START) \
        ((int)(((END).tv_sec  - (START).tv_sec ) * 1000000 + \
               ((END).tv_nsec - (START).tv_nsec) / 1000))

struct thread_arg {
	long id;
	long iters;
	long wait_metric;
};

int main(int argc, char* argv[])
{
	int r;
	// Read command line arguments
	long thread_num, iter_num;
	if (argc != 3) {
		fprintf(stderr, "Usage: ./main <thread_num> <iter_num_per_thread>\n");
		return 1;
	}
	if (read_long(&thread_num, argv[1]) != 0)
		return 1;
	if (read_long(&iter_num, argv[2]) != 0)
		return 1;
	if ((thread_num < 0) || (iter_num < 0)) {
		fprintf(stderr, "Both arguments shall be > 0\n");
		return 1;
	}

	// Allocate resources for global_lock
	global_lock = lock_alloc();
	if (global_lock == (void*)(-1)) {
		fprintf(stderr, "[MAIN] Error in lock_alloc()\n", r);
		return 1;
	}

	// Allocate memory for threads' stuff
	pthread_t* thr = (pthread_t*)calloc(thread_num, sizeof(*thr));
	assert(thr);
	struct thread_arg* thr_args = (struct thread_arg*)calloc(thread_num, sizeof(*thr_args));
	assert(thr_args);

	atomic_store(&global_atomic_cnt, 0);
	// Force threads to wait until all of them are spawned
	atomic_store(&global_barrier, 1);

	// Start threads
	for (long i = 0; i < thread_num; i++) {
		thr_args[i].id = i;
		thr_args[i].wait_metric = 0;
		thr_args[i].iters = iter_num;
		r = pthread_create(&thr[i], NULL, thread_work, &thr_args[i]);
		assert((r == 0) && "pthread_create");
	}

	struct timespec overall_start, overall_end;
	// Ensure that all threads make it to the barrier.
	usleep(100);
	// Release bullhead! (Vipuskayte bichka! (C))
	clock_gettime(CLOCK_MONOTONIC_RAW, &overall_start);
	atomic_fetch_and_dec(&global_barrier);

	// Wait for all threads to finish their job
	while (atomic_load(&global_atomic_cnt) != thread_num);
	clock_gettime(CLOCK_MONOTONIC_RAW, &overall_end);

	// Wait until all threads exit
	for (long i = 0; i < thread_num; i++) {
		void* retval;
		r = pthread_join(thr[i], &retval);
		assert((r == 0) && "pthread_join");
		if (retval != NULL) {
			// We are aborting since bad retval from one thread
			// means that some other thread can hang indefinitely.
			fprintf(stderr, "[MAIN] Bad retval from thread %ld, aborting\n",
			        thr_args[i].id);
			return 1;
		}
	}

	// Check lock validity
	if (global_cnt != iter_num*thread_num) {
		fprintf(stderr, "Global thread-iter counter did not add up(%ld vs. %ld), "
		                "probably your lock is compromised \n",
		                global_cnt, iter_num*thread_num);
		return 1;
	}

	// Compute overall execution time
	long overall_us = DELTA_TIMESPEC_US(overall_end, overall_start);
	// Compute average execution time of a thread
	long sum = 0;
	for (long i = 0; i < thread_num; i++)
		sum += thr_args[i].wait_metric;
	// Print performance metrics in the following format:
	// thread_num <overall_exec_time_in_ms> <average_wait_metric_per_iteration>
	printf("%ld\t%ld\t%ld\n", thread_num, overall_us/1000, sum/(thread_num*iter_num));

	r = lock_free(global_lock);
	if (r != 0) {
		fprintf(stderr, "[MAIN] Error in lock_free(): %d\n", r);
		return 1;
	}
	return 0;
}

/* https://en.wikipedia.org/wiki/Time_Stamp_Counter */
uint64_t rdtscl(void)
{
	uint32_t lo, hi;
	__asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
	return ( (uint64_t)lo)|( ((uint64_t)hi)<<32 );
}

#define WAIT_METRIC_TSC

void* thread_work(void* arg)
{
	int r;
	struct thread_arg* targ = (struct thread_arg*)arg;
	// Wait until all other threads are spawned
	while (atomic_load(&global_barrier) == 1);

	for (long i = 0; i < targ->iters; i++) {
#ifdef WAIT_METRIC_US
		struct timespec start, end;
#elif defined(WAIT_METRIC_TSC)
		uint64_t start, end;
#endif

#ifdef WAIT_METRIC_US
		clock_gettime(CLOCK_MONOTONIC_RAW, &start);
#elif defined(WAIT_METRIC_TSC)
		start = rdtscl();
#endif

		r = lock_acquire(global_lock);
		if (r != 0) {
			fprintf(stderr, "[%ld] Error in lock_acquire(): %d\n", r);
			exit(1);
		}

		global_cnt++;
		volatile int j = 0;
		while (j++ < 4096);

		r = lock_release(global_lock);
		if (r != 0) {
			fprintf(stderr, "[%ld] Error in lock_release(): %d\n", r);
			exit(1);
		}

#ifdef WAIT_METRIC_US
		clock_gettime(CLOCK_MONOTONIC_RAW, &end);
#elif defined(WAIT_METRIC_TSC)
		end = rdtscl();
#endif

#ifdef WAIT_METRIC_US
		targ->wait_metric += DELTA_TIMESPEC_US(end, start);
#elif defined(WAIT_METRIC_TSC)
		targ->wait_metric += end - start;
#endif
	}

	atomic_fetch_and_inc(&global_atomic_cnt);

	return NULL;
}

// strtol() wrapper
long read_long(long* res, const char* str)
{
	long val;
	char* endptr;
	val = strtol(str, &endptr, 10);
	if ((*str == '\0') || (*endptr != '\0')) {
		fprintf(stderr, "\nFailed to convert string %s to long integer\n", str);
		return 1;
	}
	if ((val == LONG_MIN) || (val == LONG_MAX)) {
		fprintf(stderr, "\nOverflow/underflow occured(%s)\n", str);
		return 1;
	}
	*res = val;
	return 0;
}
