#ifndef SAHSHELL_SAHCOMMANDS_H
#define SAHSHELL_SAHCOMMANDS_H

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/errno.h>

#include "Parsing.h"
#include "Globals.h"

/* Global variables to keep track of the current working directory etc. */
char* HOME_DIR;                     /* The home directory path */
char PREVIOUS_DIR[MAX_PATH_LENGTH]; /* The previous directory path */
char CURRENT_DIR[MAX_PATH_LENGTH];  /* The current directory path */

/**
 * Performs the built in command checkEnv. Will essentially perform the
 * command "printenv | sort | <pager>" if no arguments are given and the command
 * "printenv | grep <arguments> | sort | <pager>" if arguments are given. <pager> will
 * use the PAGER enviorenment to decide which pager to use. If PAGER is not set
 * 'less' will be used and if 'less' does not exist 'more' will be used.
 *
 * @param The command to be executed. Should be of the form "checkEnv <arguments>".
 */
void sah_check_env(Commands commands);

/**
 * Performs the built in command cd. Will change the current working directory
 * as specified by the arguments.
 *
 * @param command The command to execute. Should be of the form "cd <arguments>".
 */
void sah_cd(Command command);

/**
 * Exits the shell.
 */
void sah_exit();

/**
 * Executes the processes specified by the given commands.
 *
 * @param commands A command specifying which processes to execute. Should be
 * of the form <process 1> <arguments 1> | ... | <process n> <arguments n>.
 */
void sah_start_processes(Commands commands);

/**
 * Starts a process in the background as specified by the given command.
 *
 * @param command. A command specifying which process to execute. Should be
 * of the form <process> <arguments> &.
 */
void sah_start_background_process(Command command);

/**
 * Executes the process that lies at process_path with the arguments
 * given by command.
 *
 * @param process_path The path to the process to execute.
 * @param command Command containing the arguments.
 */
void execute(char* process_path, Command command);

/**
 * Fetches the name of the pager to use. Will check the environemtn variable
 * PAGER to decide which pager to use. If PAGER is not set 'less' will be returned
 * and if 'less' does not exist 'more' will be returned.
 *
 * @param pager A pointer to a char pointer. At return it will point to a
 * string containing the pager name.
 * @return returns the pager parameter.
 */
char* get_pager(char** pager);

/**
 * Searches through the PATH environment for the given process.
 *
 * @param process_path A buffer to fill with the path to the process.
 * @param process The process to search for.
 * @return 1 a process is found, 0 otherwise.
 */
bool get_process_path(char* process_path, char* process);

/**
 * Checks wether the file at path can be executed.
 *
 * @return 1 if the file at the given path can be
 * executed, 0 otherwise.
 */
bool file_is_executable(char* path);

/**
 * Prints a message "Execution time: <time> s" where <time> is given by
 * the before and after timevals.
 *
 * @param before The start of the time to print.
 * @param after The end of the time to print.
 */
void print_exec_time(struct timeval before, struct timeval after);

/**
 * Weird definition to remove compiler warning.
 */
int kill(int a, int b);

#endif /* SAHSHELL_SAHCOMMANDS_H */
