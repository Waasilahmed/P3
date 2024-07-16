CC = gcc
CFLAGS = -std=c99 -g -Wall -fsanitize=address,undefined

mysh: mysh.o
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf *.o demo test