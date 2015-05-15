#ifndef SAHSHELL_GLOBALS_H
#define SAHSHELL_GLOBALS_H

/* Helpful defines */
#define READ  0
#define WRITE  1

typedef int bool;
#define TRUE  1
#define FALSE 0

/* Error handling. */

#define check(condition, msg) if(condition){ fprintf(stderr, "%s[l:%d]: ", __FILE__, __LINE__); perror(msg); sah_exit(1); }

/* System error messages */
extern const char* SIGNAL_ERR;
extern const char* HOME_ENV_ERR;
extern const char* PATH_ENV_ERR;
extern const char* USER_ENV_ERR;
extern const char* WORKING_DIR_ERR;
extern const char* FORK_ERR;
extern const char* TIME_ERR;
extern const char* PIPE_ERR;
extern const char* DUP_ERR;
extern const char* CLOSE_ERR;
extern const char* WAIT_ERR;

#endif /* SAHSHELL_GLOBALS_H */
