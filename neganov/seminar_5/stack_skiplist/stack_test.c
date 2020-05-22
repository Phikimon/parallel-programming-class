#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "helpers.h"
#include "stack.h"

#define MAGIC ((int)0xBADF00D)

#define ITER_NUM 100002 // must be divisible by POPPING_THREADS and PUSHING_THREADS
#define POPPING_THREADS 6
#define PUSHING_THREADS 6

#define TOTAL_THREADS (POPPING_THREADS + PUSHING_THREADS)
#define ITER_PER_POPPER (ITER_NUM / POPPING_THREADS)
#define ITER_PER_PUSHER (ITER_NUM / PUSHING_THREADS)

struct stack* stack;

//#define PRINT_PROGRESS
//#define DEBUG_DATA

#ifdef PRINT_PROGRESS
volatile int popper_counter;
volatile int pusher_counter;
#endif

#ifdef DEBUG_DATA
volatile int popped_1;
volatile int popped_2;
#endif

void* popper(void* arg) {
	struct stack* stack = (struct stack*)arg;
	void* data;
	for (int i = 0; i < ITER_PER_POPPER; i++) {
#ifdef PRINT_PROGRESS
		int global_iteration_num = atomic_fetch_inc(&popper_counter);
		printf("pop: %d\n", global_iteration_num);
#endif
		data = stack_pop(stack, 1); //< wait for data
#ifdef DEBUG_DATA
		if (data == (void*)(0))
			atomic_fetch_inc(&popped_1);
		else if (data == (void*)(1))
			atomic_fetch_inc(&popped_2);
		else
			assert(!"invalid input");
#else
		assert(data == (void*)(MAGIC));
#endif
	}
	return NULL;
}

void* pusher(void* arg) {
	struct stack* stack = (struct stack*)arg;
	void* data = (void*)(MAGIC);
	for (int i = 0; i < ITER_PER_PUSHER; i++) {
#ifdef PRINT_PROGRESS
		printf("push: %d\n", atomic_fetch_inc(&pusher_counter));
#endif
#ifdef DEBUG_DATA
		data = (void*)(i % 2);
#endif
		stack_push(stack, data);
	}
	return NULL;
}

int main(void)
{
	stack = calloc(1, sizeof(struct stack));
	assert(stack);
	stack_init(stack);

#ifdef DEBUG_DATA
	atomic_store(&popped_1, 0);
	atomic_store(&popped_2, 0);
#endif
#ifdef PRINT_PROGRASS
	atomic_store(&popper_counter, 0);
	atomic_store(&pusher_counter, 0);
#endif

	int r;

	pthread_t thr[TOTAL_THREADS];
	// Spawn writers
	for (int i = 0; i < TOTAL_THREADS; i++) {
		r = pthread_create(&thr[i], NULL,
		                   (i < POPPING_THREADS) ? popper : pusher,
		                   (void*)(stack));
		assert((r == 0) && "pthread_create");
	}
	for (int i = 0; i < TOTAL_THREADS; i++) {
		r = pthread_join(thr[i], NULL);
		assert((r == 0) && "pthread_join");
	}

#ifndef ABA_VULNERABLE
	assert(stack->gen == (void*)ITER_NUM);
#endif
	assert(stack->head == NULL);
	free(stack);
	return 0;
}
