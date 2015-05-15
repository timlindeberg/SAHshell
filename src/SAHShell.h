#ifndef SAHSHELL_SAHSHELL_H
#define SAHSHELL_SAHSHELL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include <assert.h>

#include "Parsing.h"
#include "SAHCommands.h"
#include "Globals.h"

/**
 * The main procedure:
 *  - reads user input
 *  - executes commands
 */
int main(int argc, char** argv, char** envp);

/**
 * Executes commands.
 *
 * @param commands The commands to be executed.
 * @return void
 */
void execute_commands(Commands commands);

/**
 * Checks if the commands contains a background process.
 *
 * @param commands The commands.
 * @return bool TRUE if background command otherwise FALSE.
 */
bool is_background_command(Command cmd);

/**
 * Sets the CURRENT_DIR variable to the current directory path.
 */
void set_current_dir();

/**
 * Prints a prompt containing the users name and current
 * directory.
 */
void print_prompt();

/**
 * Checks whether the the string starts with the home directory.
 * @return the index at which the string s starts to differ
 * from HOME_DIR.
 */
int starts_with_homedir(char* s);

/**
 * Creates a string describing the current directory where
 * the home directory is replaced by '~'.
 * @param str the string buffer to be filled
 * @return returns str
 */
char* create_dir_string(char* str);

/**
 * Checks wether the the string starts with the home directory.
 * @return the index at which the string s starts to differ
 * from HOME_DIR.
 */
int starts_with_homedir(char* s);


/**
 * Signal handling. If SIGDET is defined the shell we handle the
 * termination of child processes through signals (SIGCHLD).
 * Otherwise polling will be used.
 */
#ifdef SIGDET

/**
 * SIGCHLD signal handler.
 *
 * Preforms wait on the child to allow the system to release
 * the resources associated with it.
 * @return void Immediately if no child has exited.
 */
static void sigchld_handler(int signo);

#else

/**
 * Preforms wait on all child process to allow the system to release the
 * resources associated with them.
 *
 * @return void Immediately if no child has exited.
 */
void wait_for_children();

#endif /* SIGDET */


#endif /* SAHSHELL_SAHSHELL_H */
