#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>

#define SOFTWARE_BARRIER do {                  \
	__asm__ __volatile__("" ::: "memory"); \
} while(0)

#define mfence()                  __sync_synchronize()
#define atomic_store(ptr, val)    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST)
#define atomic_load(ptr)          __atomic_load_n(ptr, __ATOMIC_SEQ_CST)
#define atomic_fetch_inc(ptr)     (__atomic_fetch_add(ptr, 1, __ATOMIC_SEQ_CST))
#define atomic_fetch_dec(ptr)     (__atomic_fetch_sub(ptr, 1, __ATOMIC_SEQ_CST))
#define atomic_exchange(ptr, val) (__atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST))
#define atomic_cas(ptr, oldptr, newval) (__atomic_compare_exchange_n(ptr, oldptr, newval, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
#define atomic_cas_ptr(ptr, oldptr, newptr) (__atomic_compare_exchange(ptr, oldptr, newptr, 0, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))

#endif
