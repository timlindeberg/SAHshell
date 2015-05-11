cmake_minimum_required(VERSION 3.1)
project(SAHshell)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ansi -Wall -pedantic -O3 -DSIGDET=1")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "~/Dropbox/Aktuella Kurser/OS/SAHShell/bin")
add_definitions(-DNO_COLORS=1)
set(SOURCE_FILES
    src/Parsing.c
    src/SAHCommands.c
    src/SAHShell.c
    Makefile
    README.md
    run.sh)

add_executable(SAHshell ${SOURCE_FILES} src/Parsing.h src/Parsing.c src/SAHShell.h src/SAHShell.c src/SAHCommands.h src/SAHCommands.c src/Globals.h)
