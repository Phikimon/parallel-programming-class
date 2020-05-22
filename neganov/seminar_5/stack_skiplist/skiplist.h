#ifndef SKIPLIST_H
#define SKIPLIST_H

// Implemented in conformance with Herlihy's 'The art of multiprocessor programming'
// and with a peek at /pooh64's code

#include <limits.h>
#include <stdlib.h>
#include <stdint.h>

#include "helpers.h"
#include "stack.h"

#define MAX_LEVEL 4 // inclusive

#define IS_MARKED(ptr) ((int)  ((uint64_t)(ptr) &   ((uint64_t)0x1)))
#define    MARKED(ptr) ((void*)((uint64_t)(ptr) |   ((uint64_t)0x1)))
#define  UNMARKED(ptr) ((void*)((uint64_t)(ptr) &  ~((uint64_t)0x1)))
#define   POINTER(ptr) UNMARKED(ptr)

struct skiplist_node {
	int key;
	int val;
	int top_level;
	volatile struct skiplist_node* next[MAX_LEVEL + 1];
};
typedef struct skiplist_node sln_s;

struct skiplist {
	sln_s* head;
	sln_s* tail;
	struct stack freelist;
};
typedef struct skiplist sl_s;


void sl_node_init(sln_s* node, int val);

void sl_init(sl_s* skiplist);

void sl_deinit(sl_s* skiplist);

int sl_find(sl_s* sl, int val, sln_s** preds, sln_s** succs);

int sl_contains(sl_s* sl, int val);

int sl_add(sl_s* sl, int val);

int sl_remove(sl_s* sl, int val);

void sl_debug_show_all_keys(sl_s* sl);
int sl_debug_get_number_of_keys(sl_s* sl);

#endif
