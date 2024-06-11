CC = gcc
SRC = filesystem.c
BIN = filesystem
CFALGS = -Wall -Wextra -g
ARG = test.txt

build:
	$(CC) $(CFALGS) $(SRC) -o $(BIN)

rebuild:
	make clean
	make build

run: build
	rm -f myfs.txt
	./$(BIN) $(ARG)

clean:
	rm -f $(BIN) myfs.txt