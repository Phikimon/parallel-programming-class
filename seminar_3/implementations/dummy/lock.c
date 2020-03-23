#include <stddef.h> // for NULL

#include "lock.h"

lock_t* lock_alloc(long unsigned n_threads) {
	return NULL;
}

int lock_acquire(lock_t* arg) {
	return 1;
}

int lock_release(lock_t* arg) {
	return 1;
}

int lock_free(lock_t* arg) {
	return 1;
}

