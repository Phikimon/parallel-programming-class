HEADERS=helpers.h skiplist.h

SANITIZE_FLAGS=-fsanitize=address
#SANITIZE_FLAGS=-fsanitize=thread

skiplist_OBJ=skiplist_test.o skiplist.o

all: skiplist_test

%.o: %.c $(HEADERS)
	gcc -c -g $(SANITIZE_FLAGS) -pthread -Wall -std=c99 $< -o $@

skiplist_test: skiplist_test.o skiplist.o $(HEADERS)
	gcc -g -Wall $(SANITIZE_FLAGS) -pthread -std=c99 $(skiplist_OBJ) -o $@

clean:
	rm -f skiplist_test *.o 2> /dev/null
