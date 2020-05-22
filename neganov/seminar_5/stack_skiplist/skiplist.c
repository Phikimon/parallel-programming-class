#include "skiplist.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static int hash(int val)
{
	return (int)val;
}

static uint8_t least_significant_bit(uint32_t val)
{
	for (uint8_t i = 0; i < 32; ++i) {
		if ((val >> i) & (uint32_t) 1)
			return i;
	}
	return 32;
}

void sl_node_init(sln_s* node, int val)
{
	for (int i = 0; i <= MAX_LEVEL; i++)
		node->next[i] = NULL;
	node->val = val;
	if ((val == INT_MIN) || (val == INT_MAX))
		node->key = val;
	else
		node->key = hash(val);
	node->top_level = least_significant_bit(rand() % ((1 << MAX_LEVEL) - 1) + 1);
}


void sl_init(sl_s* skiplist)
{
	skiplist->head = (sln_s*)calloc(1, sizeof(sln_s));
	sl_node_init(skiplist->head, INT_MIN);
	assert(skiplist->head);
	skiplist->tail = (sln_s*)calloc(1, sizeof(sln_s));
	sl_node_init(skiplist->tail, INT_MAX);
	assert(skiplist->tail);

	for (int i = 0; i <= MAX_LEVEL; ++i)
		skiplist->head->next[i] = skiplist->tail;

	stack_init(&skiplist->freelist);
}

void sl_deinit(sl_s* skiplist)
{
	// Free nodes still in skiplist
	sln_s* curr = skiplist->head;
	while(curr) {
		assert(!IS_MARKED(curr));
		free(curr);
		curr = (sln_s*)curr->next[0];
	}

	// Free nodes from freelist
	sln_s* node;
	while ((node = stack_pop(&skiplist->freelist))) {
		assert(IS_MARKED(node));
		free(POINTER(node));
	}
}

int sl_find(sl_s* sl, int val, sln_s** preds, sln_s** succs)
{
	int key = hash(val);
	int bottom_level = 0;
	int top_level = MAX_LEVEL;
	sln_s* pred = NULL;
	sln_s* curr = NULL;
	sln_s* succ = NULL;
retry:
	while (1) {
		pred = sl->head;
		for (int level = top_level; level >= bottom_level; --level) {
			// There is a guarantee that curr is not marked, and still...
			curr = POINTER((sln_s*)atomic_load(&pred->next[level]));
			while (1) {
				succ = (sln_s*)atomic_load(&curr->next[level]);
				int is_curr_marked = IS_MARKED(succ);
				succ = POINTER(succ);
				while (is_curr_marked) {
					// try snip
					assert(!IS_MARKED(curr)); //< Assure that we don't unmark one's ->next
					if (atomic_cas(&pred->next[level], (volatile sln_s**)&curr, succ) == 0)
						goto retry;
					curr = POINTER((sln_s*)atomic_load(&pred->next[level])); // < Subject to deletion
					succ = (sln_s*)atomic_load(&curr->next[level]); // < Subject to deletion
					is_curr_marked = IS_MARKED(succ);
					succ = POINTER(succ);
				}
				// Curr is unmarked
				if (curr->key < key) {
					// Step back
					pred = curr; curr = succ;
				} else
					break;
			}
			preds[level] = pred;
			succs[level] = curr;
		}
		// It is guaranteed that curr is unmarked
		return (curr->key == key);
	}
}

int sl_contains(sl_s* sl, int val)
{
	int bottom_level = 0;
	int top_level = MAX_LEVEL;
	int key = hash(val);
	sln_s* pred = sl->head;
	sln_s* curr = NULL;
	sln_s* succ = NULL;
	for (int level = top_level; level >= bottom_level; --level) {
		curr = POINTER((sln_s*)atomic_load(&pred->next[level]));
		while (1) {
			succ = POINTER((sln_s*)atomic_load(&curr->next[level]));
			while (IS_MARKED(succ)) {
				curr = POINTER((sln_s*)atomic_load(&pred->next[level]));
				succ = (sln_s*)atomic_load(&curr->next[level]);
				pred = curr;
			}
			succ = POINTER(succ);
			curr = POINTER(curr);
			if (curr->key < key) {
				pred = curr;
				curr = succ;
			} else
				break;
		}
	}
	return (curr->key == key);
}

int sl_add(sl_s* sl, int val)
{
	int bottom_level = 0;
	sln_s* preds[MAX_LEVEL + 1] = {NULL};
	sln_s* succs[MAX_LEVEL + 1] = {NULL};
	sln_s* new_node = (sln_s*)calloc(1, sizeof(sln_s));
	assert(new_node);
	sl_node_init(new_node, val);

	while (1) {
		if (sl_find(sl, val, preds, succs) == 1) {
			free(new_node);
			return 0;
		}
		for (int level = bottom_level; level <= new_node->top_level; level++)
			new_node->next[level] = POINTER(succs[level]); //< POINTER is subject to deletion
		sln_s* pred = POINTER(preds[bottom_level]); // as well as here and below
		sln_s* succ = POINTER(succs[bottom_level]);
		assert(!IS_MARKED(succ)); //< Assure that we don't unmark one's ->next
		if (atomic_cas(&pred->next[bottom_level], (volatile sln_s**)&succ, new_node) == 0)
			continue;
		for (int level = bottom_level + 1; level <= new_node->top_level; ++level) {
			while (1) {
				pred = POINTER(preds[level]);
				succ = POINTER(succs[level]);
				assert(!IS_MARKED(succ)); //< Assure that we don't unmark one's ->next
				if (atomic_cas(&pred->next[level], (volatile sln_s**)&succ, new_node) == 1)
					break;
				sl_find(sl, val, preds, succs);
			}
		}
		return 1;
	}
}

int sl_remove(sl_s* sl, int val)
{
	int bottom_level = 0;
	sln_s* preds[MAX_LEVEL + 1] = {NULL};
	sln_s* succs[MAX_LEVEL + 1] = {NULL};
	sln_s* succ;
	while (1) {
		if (!sl_find(sl, val, preds, succs))
			return 0;
		sln_s* victim = succs[bottom_level];
		for (int level = victim->top_level; level >= bottom_level + 1; --level) {
			succ = (sln_s*)atomic_load(&victim->next[level]);
			while (!IS_MARKED(succ)) {
				succ = UNMARKED(succ);
				assert(!IS_MARKED(succ)); //< Assure that we don't unmark one's ->next
				atomic_cas(&victim->next[level], (volatile sln_s**)&succ, MARKED(succ));
				succ = (sln_s*)atomic_load(&victim->next[level]);
			}
		}
		succ = (sln_s*)atomic_load(&victim->next[bottom_level]);
		while (1) {
			succ = POINTER(succ);
			assert(!IS_MARKED(succ)); //< Assure that we don't unmark one's ->next
			int i_marked_it = atomic_cas(&victim->next[bottom_level], (volatile sln_s**)&succ, MARKED(succ));
			succ = (sln_s*)atomic_load(&succs[bottom_level]->next[bottom_level]);
			if (i_marked_it) {
				sl_find(sl, val, preds, succs);
				stack_push(&sl->freelist, victim);
				return 1;
			} else if (IS_MARKED(succ))
				return 0;
		}
	}
}

void sl_debug_show_all_keys(sl_s* sl)
{
	sln_s* curr = sl->head;
	do {
		fprintf(stderr, "key = %d, marked = %d\n", curr->key, IS_MARKED(curr->next[0]));
		curr = POINTER(curr->next[0]);
	} while(curr);
}

int sl_debug_get_number_of_keys(sl_s* sl)
{
	int cnt = 0;
	sln_s* curr = sl->head;
	do {
		curr = POINTER(curr->next[0]);
		cnt++;
	} while(curr);
	return cnt;
}
