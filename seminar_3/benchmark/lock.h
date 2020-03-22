#ifndef LOCK_H
#define LOCK_H

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
 * of success and (void*)(-1) otherwise.
 */
void* lock_alloc(long unsigned n_threads);

/*
 * Acquires the lock.
 *
 * 'arg' is the value returned by lock_alloc() function.
 *
 * Returns zero in case of success and nonzero value
 * otherwise, e.g. if lock is in inconsistent state.
 */
int lock_acquire(void* arg);

/*
 * Releases the lock.
 *
 * 'arg' is the value returned by lock_alloc() function.
 *
 * Returns zero in case of success and nonzero value
 * otherwise, e.g. if lock was not acquired by the caller.
 */
int lock_release(void* arg);

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
int lock_free(void* arg);

#endif
