FLAGS = -pedantic -Wall -ansi -O4 -g

FILES = src/main.c

all: bin/Main.out

bin/Main.out: $(FILES) $(HEADERS)
	gcc $(FILES) -o bin/Main.out $(FLAGS)