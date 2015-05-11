#ifndef SAHSHELL_GLOBALS_H
#define SAHSHELL_GLOBALS_H

/* Helpful defines */
#define READ  0
#define WRITE  1

typedef int bool;
#define TRUE  1
#define FALSE 0

/* Error handling. */

#define check(condition, msg) if(condition){ fprintf(stderr, "%s[l:%d]: ", __FILE__, __LINE__); perror(msg); sah_exit(); }

/* System error messages */
static char* SIGNAL_ERR      = "Failed to register signal handler";
static char* HOME_ENV_ERR    = "Could not get HOME env";
static char* PATH_ENV_ERR    = "Could not get PATH env";
static char* USER_ENV_ERR    = "Could not get USER env";
static char* WORKING_DIR_ERR = "Could not get working directory";
static char* FORK_ERR        = "Could not fork process";
static char* TIME_ERR        = "Could not get current time";
static char* PIPE_ERR        = "Could not create pipe";
static char* DUP_ERR         = "Could not duplicate file descriptor";
static char* CLOSE_ERR       = "Could not close pipe file descriptor";
static char* WAIT_ERR        = "Error when waiting for child process";

#endif /* SAHSHELL_GLOBALS_H */
