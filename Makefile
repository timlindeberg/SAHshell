FLAGS = -pedantic -Wall -ansi -O3
FILES = src/main.c

all: bin/Main.out

bin/Main.out: $(FILES)
	gcc $(FILES) -o bin/Main.out $(FLAGS)
