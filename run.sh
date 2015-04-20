#!/bin/sh
set -e
make bin/Main.out;
valgrind -q bin/Main.out;
