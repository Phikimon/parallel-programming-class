#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdint.h>
#include <limits.h>

#include "list.h"

struct my_struct {
	struct list_head list;

	uint32_t owner;
	uint32_t index;
};

struct thread_arg {
	struct my_struct* head;
	int list_quota;
	int id;
};

void grow_list(void* arg);
void print_list(struct my_struct* head);
int verify_list(struct my_struct* head, long thread_num, int list_quota);

long read_long(long* res, const char* str);

int main(int argc, char* argv[])
{
	long list_size, thread_num;
	if (argc != 3) {
		fprintf(stderr, "usage: ./test <list_size> <thread_num>\n");
		return 1;
	}
	if (read_long(&list_size, argv[1]) != 0)
		return 1;
	if (read_long(&thread_num, argv[2]) != 0)
		return 1;
	if ((list_size < 0) || (thread_num < 0)) {
		fprintf(stderr, "Both arguments shall be > 0\n");
		return 1;
	}

	int list_quota = (int)list_size / thread_num;
	if (list_quota == 0) {
		fprintf(stderr, "Thread number(%ld) exceeds list size(%d)\n", thread_num, list_size);
		return 1;
	}

	struct my_struct head = {
		.owner = UINT_MAX,
		.index = UINT_MAX,
		.list = LIST_HEAD_INIT(head.list),
	};

	struct thread_arg targ = {
		.head = &head,
		.list_quota = list_quota,
		.id = 0,
	};

	grow_list(&targ);
	print_list(&head);

	return verify_list(&head, thread_num, list_quota);
}

// Thread's func. Adds 'list_quota' elements to the list
void grow_list(void* arg) {
	struct thread_arg* targ = (struct thread_arg*)arg;
	int num = targ->list_quota;
	struct my_struct* head = targ->head;
	int id = targ->id;

	struct my_struct* sublist = (struct my_struct*)calloc(num, sizeof(*sublist));
	assert(sublist);

	struct list_head* prev = &head->list;
	for (int i = 0; i < num; i++) {
		sublist[i].owner = id;
		sublist[i].index = i;
		list_add(&sublist[i].list, prev);
		prev = &sublist[i].list;
	}
}

int verify_list(struct my_struct* head, long thread_num, int list_quota)
{
	struct my_struct* pos;
	int* entry_counters_by_owner = (int*)calloc(thread_num, sizeof(int));
	list_for_each_entry(pos, &head->list, list) {
		if (entry_counters_by_owner[pos->owner] != pos->index) {
			fprintf(stderr, "Element {o = %5u; i = %5u} out of order\n",
			        pos->owner, pos->index);
			return 1;
		}
		entry_counters_by_owner[pos->owner]++;
	}
	for (long i = 0; i < thread_num; i++) {
		if (entry_counters_by_owner[i] != list_quota) {
			fprintf(stderr, "Owner %u written %d elements, %d expected\n",
					i, entry_counters_by_owner[i], list_quota);
			return 1;
		}
	}
}

// \brief prints list to stdout
void print_list(struct my_struct* head) {
	struct my_struct* pos;
	list_for_each_entry(pos, &head->list, list) {
		printf("{o = %5u; i = %5u}\n",
		        pos->owner, pos->index);
	}
}

// \brief strtol wrapper
long read_long(long* res, const char* str)
{
	long val;
	char* endptr;
	val = strtol(str, &endptr, 10);
	if ((*str == '\0') || (*endptr != '\0')) {
		fprintf(stderr, "\nFailed to convert string %s to long integer\n", str);
		return 1;
	}
	if ((val == LONG_MIN) || (val == LONG_MAX)) {
		fprintf(stderr, "\nOverflow/underflow occured(%s)\n", str);
		return 1;
	}
	*res = val;
	return 0;
}
