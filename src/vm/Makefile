CC=gcc
CFLAGS=-I/home/faizmk/miniSQL/include -g -Wall
VPATH=src/vm

vm.o: vm.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o