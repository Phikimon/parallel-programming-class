HEADERS=helpers.h stack.h

SANITIZE_FLAGS=-fsanitize=address
#SANITIZE_FLAGS=-fsanitize=thread

STACK_OBJ=stack_test.o stack.o

all: stack_test

%.o: %.c $(HEADERS)
	gcc -c -g $(SANITIZE_FLAGS) -pthread -Wall -std=c99 $< -o $@

stack_test: stack_test.o stack.o $(HEADERS)
	gcc -g -Wall $(SANITIZE_FLAGS) -pthread -std=c99 $(STACK_OBJ) -o $@

clean:
	rm -f stack_test *.o 2> /dev/null
