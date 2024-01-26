CC=gcc
CFLAGS=-Wall -Werror

all : writer

writer : writer.o
	$(CROSS_COMPILE)$(CC) $(CFLAGS) writer.o -o writer

writer.o : writer.c
	$(CROSS_COMPILE)$(CC) $(CFLAGS) -c writer.c -o writer.o

clean :
	rm -rf *.o writer
