#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	volatile int32_t val;
} LOCK;

#define atomic_store(ptr, val)    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_load(ptr)          __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_fetch_and_dec(ptr) ((int32_t)(__atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST)))
#define atomic_cas(ptr, old, new) __atomic_compare_exchange_n(ptr, old, new, 1, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)

void* lock_alloc(void) {
	static volatile LOCK ilock;
	atomic_store(&ilock.val, 0);

	return (void*)&ilock;
}

int lock_acquire(void* arg) {
	LOCK* lock_ptr = (LOCK*)arg;

	int32_t old;
	for (;;) {
		old = 0;
		atomic_cas(&lock_ptr->val, &old, 1);
		if (old == 0)
			break;
		if (old != 1) {
			fprintf(stderr, "Lock is inconsistent(%d)\n", old);
			return 1;
		}
	}
	return 0;
}

int lock_release(void* arg) {
	LOCK* lock_ptr = (LOCK*)arg;
	if (atomic_fetch_and_dec(&lock_ptr->val) != 1) {
		fprintf(stderr, "Lock was not acquired before releasing\n");
		return 1;
	}
	return 0;
}

int lock_free(void* arg) {
	LOCK* lock_ptr = (LOCK*)arg;
	if (atomic_load(&lock_ptr->val) != 0) {
		fprintf(stderr, "Lock was not released before freeing\n");
		return 1;
	}
	return 0;
}
