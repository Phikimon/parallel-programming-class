#ifdef __linux__
// for usleep()
#  define _DEFAULT_SOURCE
#  define _BSD_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <time.h>

#include "lock.h"

struct lock {
	volatile int32_t val;
};

#define atomic_store(ptr, val)    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_load(ptr)          __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_fetch_and_dec(ptr) ((int32_t)(__atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST)))
#define atomic_cas(ptr, old, new) __atomic_compare_exchange_n(ptr, old, new, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

lock_t* lock_alloc(long unsigned n_threads) {
	static volatile lock_t ilock;
	atomic_store(&ilock.val, 0);
	srand(time(NULL));

	return (lock_t*)&ilock;
}

int lock_acquire(lock_t* arg) {
	lock_t* lock_ptr = (lock_t*)arg;

	int32_t old;
	for (;;) {
		old = 0;
		while (atomic_load(&lock_ptr->val) == 1) {
			if (rand() % 5 == 0)
				usleep(1);
		}
		atomic_cas(&lock_ptr->val, &old, 1);
		if (old == 0) {
			break;
		} else if (old == 1) {
		}
		else {
			fprintf(stderr, "Lock is inconsistent(%d)\n", old);
			return 1;
		}
	}
	return 0;
}

int lock_release(lock_t* arg) {
	lock_t* lock_ptr = (lock_t*)arg;
	if (atomic_fetch_and_dec(&lock_ptr->val) != 1) {
		fprintf(stderr, "Lock was not acquired before releasing\n");
		return 1;
	}
	return 0;
}

int lock_free(lock_t* arg) {
	lock_t* lock_ptr = (lock_t*)arg;
	if (atomic_load(&lock_ptr->val) != 0) {
		fprintf(stderr, "Lock was not released before freeing\n");
		return 1;
	}
	return 0;
}
