SRC = src/
BIN = bin/
COMP = gcc

assignment3: LinkedList
	$(COMP) -o $(BIN)assignment4 $(BIN)LinkedList.o $(SRC)assignment4.c

LinkedList:
	$(COMP) -o $(BIN)LinkedList.o -c $(SRC)LinkedList.c

clean:
	rm -rf bin/*

