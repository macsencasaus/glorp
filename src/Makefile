.PHONY: lexer parser arena repl glorp

CC=gcc
CFLAGS=-pedantic -c -g

build: lexer parser arena repl glorp

lexer lexer.c:
	$(CC) $(CFLAGS) lexer.c -o lexer.o

parser parser.c:
	$(CC) $(CFLAGS) parser.c -o parser.o

arena arena.c:
	$(CC) $(CFLAGS) arena.c -o arena.o

repl repl.c:
	$(CC) $(CFLAGS) repl.c -o repl.o

glorp lexer.o parser.o arena.o repl.o glorp.c:
	$(CC) -g -pedantic lexer.o parser.o arena.o repl.o glorp.c -o glorp
