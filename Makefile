FLAGS = -pedantic-errors -Wall -ansi -O3 -DSIGDET=1

SOURCE = src/Parsing.c src/SAHCommands.c src/SAHShell.c src/Globals.c src/ReadInput.c
HEADERS = src/Parsing.h src/SAHCommands.h src/SAHShell.h src/Globals.h src/ReadInput.h

all: bin/Main.out

bin/Main.out: $(SOURCE) $(HEADERS)
	gcc $(SOURCE) -o bin/Main.out $(FLAGS)
