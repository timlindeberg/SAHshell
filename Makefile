FLAGS = -pedantic -Wall -ansi -g
FILES = src/main.c

all: bin/Main.out

bin/Main.out: $(FILES)
	gcc $(FILES) -o bin/Main.out $(FLAGS)
