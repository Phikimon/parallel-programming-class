#ifndef STACK_H
#define STACK_H

#include <stdint.h>
#include <stddef.h>

// If the following define
// is set then stack->gen
// is meaningless
#define ABA_VULNERABLE

struct stack_entry {
	volatile struct stack_entry *next;
	volatile struct stack_entry *freelist_next; //< Used to silence ThreadSanitizer
	volatile void* data;
};

struct stack {
	volatile struct stack_entry *head;
	volatile char *gen;
	volatile struct stack_entry* freelist;
	volatile int ref_cnt;
};

void stack_init(struct stack *stack);

void stack_push(struct stack *stack, void* data);

// returns data stored in entry. if force is set,
// waits for valid entry
void* stack_pop(struct stack *stk, int force);

#endif
