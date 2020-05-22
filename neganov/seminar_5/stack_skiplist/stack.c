#include "stack.h"
#include "helpers.h"

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

void stack_init(struct stack *stack)
{
	stack->head = NULL;
	stack->gen = NULL;
	stack->freelist = NULL;
	stack->ref_cnt = 0;
}

void stack_push(struct stack *stack, void *data)
{
	struct stack_entry* entry = (struct stack_entry*)calloc(1, sizeof(*entry));
	assert(entry);

	atomic_store(&entry->data, data);
	atomic_store(&entry->next, atomic_load(&stack->head));

	while (atomic_cas(&stack->head, &entry->next, entry) == 0);
}

static void free_entries(struct stack_entry* freelist)
{
	struct stack_entry* c = freelist;
	struct stack_entry* n;
	while (c) {
		n = (struct stack_entry*)c->freelist_next;
		free(c);
		c = n;
	}
}

static struct stack_entry* get_last(struct stack_entry* first)
{
	struct stack_entry* last = first;
	assert(last);
	while (last->freelist_next)
		last = (struct stack_entry*)last->freelist_next;
	return last;
}

static void put_to_freelist(struct stack* stack,
                            struct stack_entry* first,
                            struct stack_entry* last)
{
	atomic_store(&last->freelist_next, atomic_load(&stack->freelist));
	while (atomic_cas(&stack->freelist, &last->freelist_next, first) == 0);
}

static void try_cleanup(struct stack* stack,
                        struct stack_entry* entry)
{
	if (atomic_load(&stack->ref_cnt) == 1)
	{
		// Try to do dirty work for other threads
		struct stack_entry* stolen_freelist;
		stolen_freelist = atomic_exchange((struct stack_entry**)&stack->freelist, NULL);
		if (atomic_fetch_dec(&stack->ref_cnt) == 1) {
			// We are the only one owning these entries
			free_entries(stolen_freelist);
		} else {
			// Somebody interrupted our attempt to
			// free entries, put them back
			if (stolen_freelist)
				put_to_freelist(stack, stolen_freelist,
				                get_last(stolen_freelist));
		}
		// At least we own our entry, since we did not
		// put it into the freelist
		free(entry);
	} else {
		put_to_freelist(stack, entry, entry);
		atomic_fetch_dec(&stack->ref_cnt);
	}
}

void* stack_pop(struct stack *stk, int force)
{
	struct stack old, new;
	void* result = NULL;

	atomic_fetch_inc(&stk->ref_cnt);

	old.gen  = atomic_load(&stk->gen);
	old.head = atomic_load(&stk->head);
	if ((old.head == NULL) && (!force)) {
		return NULL;
	}
	while (old.head == NULL) {
		old.gen  = atomic_load(&stk->gen);
		old.head = atomic_load(&stk->head);
	}

	new.gen  = old.gen + 1;
	new.head = atomic_load(&old.head->next);

	while (atomic_cas_ptr((__int128*)stk, (__int128*)&old, (__int128*)&new) == 0) {
		if ((old.head == NULL) && (!force))
			goto cleanup;
		while (old.head == NULL) {
			old.gen  = atomic_load(&stk->gen);
			old.head = atomic_load(&stk->head);
		}

		new.gen = old.gen + 1;
		new.head = atomic_load(&old.head->next);
	}

	// We will try to free() old.head, so
	// save result for caller
	result = (void*)atomic_load(&old.head->data);

	// refcnt is decremented in try_cleanup()
cleanup:
	try_cleanup(stk, (struct stack_entry*)old.head);

	return result;
}
