#ifndef LOCK_H
#define LOCK_H

// Some syntactic sugar to be used instead of void* according to
// https://www.kernel.org/doc/html/v4.10/process/coding-style.html#typedefs
// lock_t cannot be dereferenced and is semantically similar to void.
struct lock;
typedef struct lock lock_t;

/*
 * Allocates resources for the lock.
 *
 * n_threads is number of threads that will
 * share this lock. lock_alloc() may ignore
 * this argument.
 *
 * It is guaranteed to be called only once from a
 * calling program, so allocation in static memory is OK.
 *
 * Returns pointer to the lock in case
 * of success and (void*)(NULL) otherwise.
 */
lock_t* lock_alloc(long unsigned n_threads);

/*
 * Acquires the lock.
 *
 * 'arg' is the value returned by lock_alloc() function.
 *
 * Returns zero in case of success and nonzero value
 * otherwise, e.g. if lock is in inconsistent state.
 */
int lock_acquire(lock_t* arg);

/*
 * Releases the lock.
 *
 * 'arg' is the value returned by lock_alloc() function.
 *
 * Returns zero in case of success and nonzero value
 * otherwise, e.g. if lock was not acquired by the caller.
 */
int lock_release(lock_t* arg);

/*
 * Frees resources associated with the lock.
 *
 * 'arg' is the value returned by lock_alloc() function.
 *
 * It is guaranteed to be called only once for the
 * successfully alloc()-ed lock.
 *
 * Returns zero in case of success and nonzero value
 * otherwise, e.g. if lock was not released beforehand.
 */
int lock_free(lock_t* arg);

#endif
