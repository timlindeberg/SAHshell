#ifndef SAHSHELL_PARSING_H
#define SAHSHELL_PARSING_H

#include <stdlib.h>
#include <stdio.h>
#include <wordexp.h>
#include <string.h>

#include "Globals.h"

#define MAX_COMMAND_ENTRY 80
#define MAX_PATH_LENGTH 4096
#define MAX_ARGUMENTS 128

/* Type definitions for the structure of commands. */
typedef char Commands[MAX_ARGUMENTS][MAX_ARGUMENTS][MAX_COMMAND_ENTRY];
typedef char (* Command)[MAX_COMMAND_ENTRY];

/**
 * Parses a command line entry and stores the given Commands parameter.
 *
 * @param cmd_entry The command line entry.
 * @param commands The parsed commands.
 * @return void
 */
void parse_commands(char cmd_entry[MAX_COMMAND_ENTRY], Commands commands);

/**
 * Helper function to parse_commands.
 *
 * Given entry 'grep "foo bar" &' will be parsed to
 * ["grep", "\"foo bar\"", "&"]
 *
 * @param args An array of the parsed arguments.
 * @param cmd_entry The command line entry to be parsed.
 * @param escaped An array which will indicate which arguments were escaped on exit
 * @return void
 */
void parse_args(char** args, char* cmd_entry, bool escaped[MAX_ARGUMENTS]);

/**
 * Splits the given string on the delimiters specified.
 *
 * @param string The string to be split and modified.
 * @param string_array An array of char pointers that will contain the string parts.
 * @param delimiters The delimiter characters.
 * @return void
 */
void split(char* cmd_entry, char** cmd_args, char* delim, size_t size);

/**
 * Sets all entries in commands to null.
 * @param commands The commands structure to clear.
 * @return void
 */
void clear_commands(Commands commands);

/**
 * Calculates the size of the given args array.
 * @param args The array of arguments
 * @return The size of args
 */
int get_arg_size(char* args[MAX_ARGUMENTS]);

void handle_escapes(char* str);

void remove_char(char str[MAX_COMMAND_ENTRY], int index, size_t len);

/**
 * Debug function used to print the parsed commands.
 * @param commands The commands to print
 * @return void
 */
void print_commands(Commands commands);

#endif /* SAHSHELL_PARSING_H */
