CC = gcc
CFLAGS = -I. 
DEPS = minix.h minixcommands.h
OBJ = minix.o minixcommands.o

%.o: %c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) 
myexec: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) 
.PHONY: clean
clean:
	rm -f *.o myexec