#!/bin/sh
set -e
make bin/Main.out;
exec bin/Main.out;
