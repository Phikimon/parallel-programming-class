## Spinlocks

### Task

Implement spinlock/ticketlock with possible use of yield/sleep along with benchmarks for your algorithm.

### Result

![implementation performance](https://github.com/phikimon/parallel-programming-class/raw/master/neganov/seminar_3/results/performance.png "Implementations performance")
![queue implementation performance](https://github.com/phikimon/parallel-programming-class/raw/master/neganov/seminar_3/results/queue_performance.png "Queue locks implementations performance")


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
