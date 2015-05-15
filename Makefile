FLAGS = -pedantic -Wall -ansi -O3 -DSIGDET=1

SOURCE = src/Parsing.c src/SAHCommands.c src/SAHShell.c src/Globals.c
HEADERS = src/Parsing.h src/SAHCommands.h src/SAHShell.h src/Globals.h

all: bin/Main.out

bin/Main.out: $(SOURCE) $(HEADERS)
	gcc $(SOURCE) -o bin/Main.out $(FLAGS)
