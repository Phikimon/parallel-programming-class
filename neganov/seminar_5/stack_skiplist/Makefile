SANITIZE_FLAGS=-fsanitize=address
#SANITIZE_FLAGS=-fsanitize=thread

STACK_OBJ=stack_test.o stack.o
SKIPLIST_OBJ=stack.o skiplist_test.o skiplist.o

HEADERS=helpers.h stack.h skiplist.h

all: stack_test skiplist_test

%.o: %.c $(HEADERS)
	gcc -c -g -I$(INCLUDE_DIR) $(SANITIZE_FLAGS) -pthread -Wall -std=c99 $< -o $@

stack_test: $(STACK_OBJ) $(HEADERS)
	gcc -g -Wall -I$(INCLUDE_DIR) $(SANITIZE_FLAGS) -pthread -std=c99 $(STACK_OBJ) -o $@

skiplist_test: $(SKIPLIST_OBJ) $(HEADERS)
	gcc -g -Wall -I$(INCLUDE_DIR) $(SANITIZE_FLAGS) -pthread -std=c99 $(SKIPLIST_OBJ) -o $@

clean:
	rm -f tags skiplist_test stack_test *.o 2> /dev/null
