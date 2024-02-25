CC = gcc
CFLAGS = -g -Wall
OBJS = main.o sh.o get_path.o  

all: mysh

mysh: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o mysh

main.o: main.c sh.h get_path.h
	$(CC) $(CFLAGS) -c main.c

sh.o: sh.c sh.h get_path.h
	$(CC) $(CFLAGS) -c sh.c

get_path.o: get_path.c get_path.h
	$(CC) $(CFLAGS) -c get_path.c

clean:
	rm -f *.o mysh

