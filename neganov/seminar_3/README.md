### Afterthought

I have rethought the way spinlocks should be used and benchmarked, after reading [this blogpost](https://probablydance.com/2019/12/30/measuring-mutexes-spinlocks-and-how-bad-the-linux-scheduler-really-is/) and, more importantly, [Linus' answer](https://www.realworldtech.com/forum/?threadid=189711&curpostid=189723) to it. So it would be fair to consider contents of this directory garbage.

However this is just an educational excercise, so it was worth trying and failing.

> This has absolutely nothing to do with cache coherence latencies or anything like that. It has everything to do with badly implemented locking.

## Spinlocks

### Task

Implement spinlock/ticketlock with possible use of yield/sleep along with benchmarks for your algorithm.

### Result

Tests were performed on [Mid 2015 2,2 GHz Intel Core i7 MacBook Pro](https://support.apple.com/kb/SP719) with 4 physical cores and 8 threads.

It is obvious that queuing implementations' performance suffers, once number of threads reaches number of cpu threads, this is because they start to be inevitably scheduled off the cpu, thus slowing down whole queue. This leads to the conclusion that queuing algorithms should be used for threads that are not very likely or even cannot be scheduled at all.

#### How to read plots
* Plots of one style represent consequent executions of the same algorithm. These are used to see approximate margin of error.
* X axis is number of threads launched and Y axis is number of seconds exceeded.
* Bottom plot contains only queue locks properly scaled.

![implementation performance](https://github.com/phikimon/parallel-programming-class/raw/master/neganov/seminar_3/results/performance.png "Implementations performance")
![queue implementation performance](https://github.com/phikimon/parallel-programming-class/raw/master/neganov/seminar_3/results/queue_performance.png "Queue locks implementations performance")

#### Notes on legend
* tas - [test-and-set](https://en.wikipedia.org/wiki/Test-and-set)
* ttas - [test and test-and-set](https://en.wikipedia.org/wiki/Test_and_test-and-set)
* ttas pause - "test and test-and-set" with pause in "test" loop
* ttas wait - "test and test-and-set" with randomized usleep in "test" loop
* ticket - [ticket lock](https://en.wikipedia.org/wiki/Ticket_lock)
* queue shared - [Array Based Queuing Lock](https://en.wikipedia.org/wiki/Array_Based_Queuing_Locks) with 64 threads' sharing each cache line
* queue excl - ABQL with single thread owning each cache line
* queue excl no deref - ABQL with single thread owning each cache line, some pointer dereferences are optimized out
* queue pause - ABQL with pause in spinning loop
* queue pause atom - replaces memory barriers protecting `ticket_serving`, uses `ticket_serving` as atomic

#### TODO
* implement exponential backoff for ABQL and ttas

## How to use my benchmarks

#### API

To bring your code in compliance with my benchmarking 'framework',
you should create pull request that:
1. Creates subdirectory under 'implementations/' directory and puts
   lock.{c,cpp,asm} file there. This file shall contain implementations
   of functions that I will be able to call using prototypes stored in
   `benchmark/lock.h`. Also it should contain definition of struct lock
   also mentioned in `benchmark/lock.h`.
2. Adds name of your implementation to `IMPLS` variable in Makefile.

#### C language

`lock.c` file put in newly created directory under `implementations/`
directory will be automatically compiled to `implementations/%/lock.o`.

#### C++

Most probably some of you would want to use *C++*, in this case
for the code to be linked successfully declarations of your
functions shall look like this:

```c++
extern "C"
{
	void* lock_alloc(long unsigned n_threads);
	int lock_acquire(void* arg);
	int lock_release(void* arg);
	int lock_free(void* arg);
}
```

Alternatively you may just include "lock.h" file and declare your
functions in conformance with it.

#### Assembly

*TODO*
