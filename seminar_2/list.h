#ifndef __LIST_H
#define __LIST_H

struct list_head {
	volatile struct list_head *next;
};

#define LIST_HEAD_INIT(name) { &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next)
{
	do {
		// Before element is added to the list
		// its fields are our local so we feel
		// free not to use atomics here
		new->next = next;
		// This is the only place where we need CAS.
		// Since elements are not deleted from
		// list we can guarantee that element
		// isn't e.g. free()-d or put to another
		// place in the list while we try to
		// access its ->next field. In case of
		// failure we just change our ->next
		// field.
		__atomic_compare_exchange_n(&prev->next, &next, new, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED);
	} while (new->next != next);
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new,
                            struct list_head *head)
{
	__list_add(new, head, head->next);
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(struct list_head *head)
{
	return head->next == head;
}

/**
 * list_entry - get the struct for this entry
 * @ptr:    the &struct list_head pointer.
 * @type:   the type of the struct this is embedded in.
 * @member: the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member) \
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))

/**
 * list_for_each    -    iterate over a list
 * @pos:  the &struct list_head to use as a loop counter.
 * @head: the head for your list.
 */
#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); \
			pos = pos->next)

/**
 * list_for_each_entry    -    iterate over list of given type
 * @pos:    the type * to use as a loop counter.
 * @head:   the head for your list.
 * @member: the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)                \
	for (pos = list_entry((head)->next, __typeof__(*pos), member);    \
			&pos->member != (head);                     \
			pos = list_entry(pos->member.next, __typeof__(*pos), member))

#endif
