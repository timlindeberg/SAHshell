#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/errno.h>

#define MAX_COMMAND_ENTRY 80
#define MAX_PATH_LENGTH 4096
#define MAX_ARGUMENTS 32

typedef int bool;
#define TRUE  1
#define FALSE 0

#define READ  0
#define WRITE  1

typedef char Commands[MAX_ARGUMENTS][MAX_ARGUMENTS][MAX_COMMAND_ENTRY];
typedef char (*Command)[MAX_COMMAND_ENTRY];

int kill(int a, int b);

void print_prompt();
void print_exec_time(struct timeval before, struct timeval after);

void set_current_dir();
void split(char* cmd_entry, char** cmd_args, char* delim);

bool starts_with_homedir(char* s);
bool file_exists(char* path);
bool get_process_path(char* process, char* cmd);
void wait_for_children();

void remove_char(char str[MAX_COMMAND_ENTRY], int index, size_t len);

char* create_dir_string(char* str, int index);
char* get_pager(char** pager);

bool is_background_command(Command cmd);
void do_commands(Commands commands);
void execute(char* process_path, Command command);
void wait_for_children();

void parse_commands(char cmd_entry[MAX_COMMAND_ENTRY], Commands commands);
void _parse_commands(char** args, char* cmd_entry);

/* Commands */
void sah_check_env(Commands commands);
void sah_cd(char (*cmd)[MAX_COMMAND_ENTRY]);
void sah_exit();
void sah_start_processes(Commands commands);
void sah_start_background_process(Command command);

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

#ifdef SIGDET

/**
 * SIGCHLD signal handler.
 *
 * Preforms wait on the child to allow the system to release
 * the resources associated with it.
 * @return void Immediately if no child has exited.
 */
static void sigchld_handler(int signo) {
    int pid, status;
    assert(signo == SIGCHLD);

    /* Listen to when a child process exits again */
    signal(SIGCHLD, sigchld_handler);

    do{
        pid = waitpid(-1, &status, WNOHANG);
        check(pid == -1 && errno != ECHILD, WAIT_ERR);
        if(pid > 0){
            printf("\nProcess with id '%d' exited with status '%d' \n", pid, status);
        }
    }while(pid > 0);
}
#else

/**
 * Preforms wait on all child process to allow the system to release the
 * resources associated with them.
 *
 * @return void Iimmediately if no child has exited.
 */
void wait_for_children() {
    int pid = 0, status = 0;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        fprintf(stderr, "Process with id '%d' exited with status '%d' \n", pid, status);
    }
}
#endif


char* HOME_DIR;                     /* The home directory path */
char PREVIOUS_DIR[MAX_PATH_LENGTH]; /* The previous directory path */
char CURRENT_DIR[MAX_PATH_LENGTH];  /* The current directory path */


/**
 * The main procedure:
 *  - reads user input
 *  - executes commands
*/
int main(int argc, char** argv, char** envp) {

    /* Ignore interactive stop and termination signal */
    check(signal(SIGTSTP, SIG_IGN) == SIG_ERR, SIGNAL_ERR);
    check(signal(SIGTERM, SIG_IGN) == SIG_ERR, SIGNAL_ERR);

#ifdef SIGDET
    /* Listen to when a child process exits */
    check(signal(SIGCHLD, sigchld_handler) == SIG_ERR, SIGNAL_ERR);
#endif

    /* Get the home directory path */
    HOME_DIR = getenv("HOME");
    check(HOME_DIR == NULL, HOME_ENV_ERR);

    /* Get the current directory and assign it as previous directory */
    set_current_dir();
    strcpy(PREVIOUS_DIR, CURRENT_DIR);

    while (TRUE) {
        char cmd_entry[MAX_COMMAND_ENTRY]; /* The current command line entry */

        /* Assign current directory and print shell prompt */
        set_current_dir();
        print_prompt();

        /* Read stdin input */
        if (fgets(cmd_entry, MAX_COMMAND_ENTRY, stdin) != NULL) {
            /* Remove trailing newline from stdin input */
            size_t ln = strlen(cmd_entry) - 1;
            if (cmd_entry[ln] == '\n') cmd_entry[ln] = '\0';

            /* Parse and do commands if any input */
            if (strlen(cmd_entry) > 0) {
                Commands commands;
                parse_commands(cmd_entry, commands);
                do_commands(commands);
            }
        } else if (feof(stdin)) {
            fprintf(stderr, "Could not read from stdin, exiting");
            exit(0);
        }

#ifndef SIGDET
        /* Wait for and terminate children */
        wait_for_children();
#endif
    }
}

/**
 * Sets the CURRENT_DIR variable to the current directory path.
 */
void set_current_dir() {
    check(getcwd(CURRENT_DIR, MAX_PATH_LENGTH) == NULL, WORKING_DIR_ERR);
}


/**
 * Splits the given string on the delimiters specified.
 *
 * @param string The string to be split and modified.
 * @param string_array An array of char pointers that will contain the string parts.
 * @param delimiters The delimiter characters.
 * @return void
 */
void split(char* string, char** string_array, char* delimiters) {
    char* token;
    token = strtok(string, delimiters);
    while (token != NULL) {
        *string_array = token;
        string_array++;
        token = strtok(NULL, delimiters);
    }
    *string_array = NULL;
}

/**
 * Parses a command line entry and stores the given Commands parameter.
 *
 * @param cmd_entry The command line entry.
 * @param commands The parsed commands.
 * @return void
 */
void parse_commands(char cmd_entry[MAX_COMMAND_ENTRY], Commands commands) {
    int i = 0;
    int j = 0;
    int k = 0;
    char* cmd_args[MAX_ARGUMENTS];

    /* Initialize string matrix contain only null characters */
    while (i < MAX_ARGUMENTS) {
        j = 0;
        while (j < MAX_ARGUMENTS) {
            k = 0;
            while (k < MAX_ARGUMENTS) {
                commands[i][j][k] = '\0';
                ++k;
            }
            ++j;
        }
        ++i;
    }

    /* Split command line entry on pipe char
     * - cmd_entry = "foo | bar"
     * - cmd_args = ["foo", "bar"]
     */
    split(cmd_entry, cmd_args, "|");

    /* Loop through each argument and parse it */
    i = 0;
    while(cmd_args[i] != NULL){
        char* args[MAX_ARGUMENTS];
        /* Parse argument
         * - cmd_args[i] = "ls -a"
         * - tmp = ["ls", "-a"]
         */
        _parse_commands(args, cmd_args[i]);
        j = 0;
        while(args[j] != NULL) {
            /* Copy to commands */
            strcpy(commands[i][j], args[j]);
            j++;
        }
        i++;
    }
}

/**
 * Helper function to parse_commands.
 *
 * Given entry 'grep "foo bar" &' will be parsed to
 * ["grep", "\"foo bar\"", "&"]
 *
 * @param args An array of the parsed arguments.
 * @param cmd_entry The command line entry to be parsed.
 * @return void
 */
void _parse_commands(char** args, char* cmd_entry) {
    /* Command entry string */
    char *p = cmd_entry;
    int i = 0;
    size_t len = strlen(cmd_entry);

    /* Loop until end of entry string */
    while(p[i] != '\0') {

        /* Increment pointer to beginning of argument */
        while(p[i] == ' ' || p[i] == '\n') {
            p[i] = '\0'; /* End of argument */
            i++;
        }

        /* Handle quotes */
        if (p[i] == '"' || p[i] == '\'') {
            char quote_char = p[i];
            i++;
            *args = p + i;
            while (p[i] != quote_char && p[i] != '\0') {
                i++;
            }
            p[i] = '\0';
            i++;
        } else {
            /* Set args pointer to beginning of argument */
            *args = p + i;
        }

        /* Increment pointer to end of argument */
        while (p[i] != ' ' && p[i] != '\0' && p[i] != '\n') {
            if (p[i] == '\\') {
                remove_char(cmd_entry, i, len);
                len--;
                i++; /* Leave escaped char */
            }
            i++;
        }
        args++;
    }
    *args = NULL;
}

/**
 * Removes an character from a string.
 *
 * @param str The string.
 * @param index The index of str to be removed.
 * @param len Length of he string.
 * @return void
 */
void remove_char(char str[MAX_COMMAND_ENTRY], int index, size_t len) {
    memmove(&str[index], &str[index + 1], len - index);
}

/**
 * Executes commands.
 *
 * @param commands The commands to be executed.
 * @return void
 */
void do_commands(Commands commands) {
    Command cmd_one = commands[0];

    /* Check for background process  */
    if(is_background_command(cmd_one)){
        sah_start_background_process(cmd_one);
        return;
    }

    if (strcmp("exit", cmd_one[0]) == 0 || strcmp("quit", cmd_one[0]) == 0) {
        printf("%s\n", "Exit");
        sah_exit();
    } else if (strcmp("cd", cmd_one[0]) == 0) {
        sah_cd(cmd_one);
    } else if (strcmp("checkEnv", cmd_one[0]) == 0) {
        sah_check_env(commands);
    } else {
        sah_start_processes(commands);
    }
}

/**
 * Checks if the commands contains a background process.
 *
 * @param commands The commands.
 * @return bool TRUE if background command otherwise FALSE.
 */
bool is_background_command(Command cmd){
    int i = 0;
    size_t length = 0;
    while(*cmd[i] != '\0') {
        ++i;
    }
    --i;
    length = strlen(cmd[i]);

    /* Check if the last char is an & */
    if(cmd[i][length-1] == '&') {
        cmd[i][length-1] = '\0';
        return TRUE;
    }
    return FALSE;
}

char* get_pager(char** pager) {
    *pager = getenv("PAGER");
    if (*pager == NULL) {
        char cp[MAX_PATH_LENGTH];
        *pager = get_process_path(cp, "less") ? "less" :
                 get_process_path(cp, "more") ? "more" :
                 NULL;
    }
    return *pager;
}

void sah_check_env(Commands commands) {
    char* pager = NULL;
    int i = 0;

    strcpy(commands[i++][0], "printenv");

    /* Add grep to command */
    if (*commands[0][1] != '\0') {
        int j = 1;
        while(*commands[0][j] != '\0') {
            strcpy(commands[1][j], commands[0][j]);
            *commands[0][j] = '\0';
            j++;
        }
        strcpy(commands[i++][0], "grep");
    }

    /* Fetch pager from environment */
    if (get_pager(&pager) == NULL) {
        printf("Could not find pagers more or less.");
        return;
    }
    strcpy(commands[i++][0], "sort");
    strcpy(commands[i++][0], pager);

    sah_start_processes(commands);
}

void sah_start_background_process(Command command) {
    char process_path[MAX_PATH_LENGTH];
    char* process;
    int pid;

    process = command[0];
    if(!get_process_path(process_path, process)){
        printf("Could not found process %s\n", process);
        return;
    }

    pid = fork();
    check(pid == -1, FORK_ERR);

    if (pid == 0) {
        execute(process_path, command);
    }

    printf("PID: %d\n", pid);
}

void sah_start_processes(Commands commands) {
    int count = 0;
    int pid1 = 0;
    int i = 0;
    struct timeval before, after;

    while (**commands[count] != '\0') count++;

    while(i < count){
        char process_path[MAX_PATH_LENGTH];
        Command command = commands[i];
        if(!get_process_path(process_path, command[0])){
            printf("Could not found process %s\n", command[0]);
            return;
        }
        i++;
    }

    check(gettimeofday(&before, NULL) == -1, TIME_ERR);

    pid1 = fork();
    check(pid1 == -1, FORK_ERR);
    if (pid1 == 0) {
        int i = 0;
        while (i < count) {
            Command command = commands[i];
            char process_path[MAX_PATH_LENGTH];
            int stdout_fd[2];

            check(pipe(stdout_fd) == -1, PIPE_ERR);

            get_process_path(process_path, command[0]);

            if (i < count - 1) {
                int pid = fork();
                check(pid == -1, FORK_ERR);
                if (pid == 0) { /* Child process */
                    /* Redirect fds */
                    check(dup2(stdout_fd[WRITE], STDOUT_FILENO) == -1, DUP_ERR);
                    check(close(stdout_fd[WRITE]) == -1, CLOSE_ERR);
                    execute(process_path, command);
                }

                check(dup2(stdout_fd[READ], STDIN_FILENO) == -1, DUP_ERR);
                check(close(stdout_fd[WRITE]) == -1 , CLOSE_ERR);
            } else {
                /* Use this fork for last process. */
                execute(process_path, command);
            }
            i++;
        }
    }

    /* Ignore SIGINT */
    check(signal(SIGINT, SIG_IGN) == SIG_ERR, SIGNAL_ERR);

    /* Wait for executed process  */
    check(waitpid(pid1, NULL, 0) == -1 && errno != ECHILD, WAIT_ERR);

    /* Clear SIGINT signal */
    check(signal(SIGINT, SIG_DFL) == SIG_ERR, SIGNAL_ERR);

    check(gettimeofday(&after, NULL) == -1, TIME_ERR);
    print_exec_time(before, after);
}

void execute(char* process_path, Command command) {
    char* cmd_args[MAX_ARGUMENTS];
    int i = 1;
    cmd_args[0] = process_path;
    while(*command[i] != '\0') {
        cmd_args[i] = command[i];
        i++;
    }
    cmd_args[i] = NULL;
    if (execv(process_path, cmd_args) == -1) {
        printf("Could not execute program %s\n", process_path);
        sah_exit();
    }
}

/*
------------------------------------------------
-- Shell commands
------------------------------------------------
*/

void sah_exit() {
    kill(-getpid(), SIGTERM);
}

void sah_cd(char (*cmd)[MAX_COMMAND_ENTRY]) {
    char* dir;
    char tmp[MAX_PATH_LENGTH];
    char* path = cmd[1];

    dir = path[0] == '\0'        ? HOME_DIR :
          path[0] == '~'         ? strcat(strcpy(tmp, HOME_DIR), ++path) :
          strcmp(path, "-") == 0 ? PREVIOUS_DIR :
          /* else */               path;

    if (chdir(dir) != 0) {
        printf("%s: %s\n", "Could not change to", dir);
    } else {
        strcpy(PREVIOUS_DIR, CURRENT_DIR);
    }
}

bool get_process_path(char* process_path, char* process) {
    char* path_env;
    char* paths[MAX_ARGUMENTS];
    char cp[MAX_PATH_LENGTH];
    int i = 0;

    if (file_exists(process)){
        strcpy(process_path, process);
        return TRUE;
    }

    path_env = getenv("PATH");
    check(path_env == NULL, PATH_ENV_ERR);

    strcpy(cp, path_env);
    split(cp, paths, ":");
    while (paths[i] != NULL) {
        sprintf(process_path, "%s/%s", paths[i], process);
        if (file_exists(process_path)) {
            return TRUE;
        }
        i++;
    }

    return FALSE;
}

bool file_exists(char* path) {
    return access(path, X_OK) == 0 ? TRUE : FALSE;
}

void print_prompt() {
    int index = starts_with_homedir(CURRENT_DIR);
    char tmp[MAX_PATH_LENGTH];
    char* dir = index == -1 ? CURRENT_DIR : create_dir_string(tmp, index);
    char* name;

    name = getenv("USER");
    check(name == NULL, USER_ENV_ERR);

#ifndef NO_COLORS
    printf("\x1b[34m%s\x1b[0m: \x1b[1m%s\x1b[0m \x1b[32m $ \x1b[0m", name, dir);
#else
    printf("%s: %s $ ", name, dir);
#endif
}

char* create_dir_string(char* str, int index) {
    sprintf(str, "~%s", CURRENT_DIR + index);
    return str;
}

bool starts_with_homedir(char* s) {
    int i = 0;
    while (s[i] == HOME_DIR[i]) {
        i++;
        if (HOME_DIR[i] == '\0') {
            return i;
        }
    }
    return -1;
}

void print_exec_time(struct timeval before, struct timeval after) {
    double time_elapsed = (after.tv_sec - before.tv_sec) * 1000.0;
    time_elapsed += (after.tv_usec - before.tv_usec) / 1000.0;
    printf("Execution time: %f s\n", time_elapsed / 1000.0);
}
