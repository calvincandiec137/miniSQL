CC=gcc
CFLAGS=-I/home/faizmk/miniSQL/include -g -Wall
LDFLAGS=
VPATH=tests

VM_OBJ=vm.o

test_vm: test_vm.c $(VM_OBJ)
	$(CC) $(CFLAGS) -o test_vm tests/test_vm.c $(VM_OBJ) $(LDFLAGS)

run_test_vm: test_vm
	./test_vm

clean:
	rm -f test_vm