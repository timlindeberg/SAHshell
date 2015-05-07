#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <sys/errno.h>

#define MAX_COMMAND_ENTRY 80
#define MAX_PATH_LENGTH 4096
#define MAX_ARGUMENTS 32

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

int starts_with_homedir(char* s);
int file_exists(char* path);
int get_process_path(char* process, char* cmd);
void wait_for_children();

char* create_dir_string(char* str, int index);
char* get_pager(char** pager);

void do_commands(Commands commands);
void execute(char* process_path, Command command);
void wait_for_children();

void escape_string(char str[MAX_COMMAND_ENTRY]);
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
static void sigchld_handler(int signo) {
    int pid, status;
    assert(signo == SIGCHLD);
    do{
        pid = waitpid(-1, &status, WNOHANG);
        check(pid == -1 && errno != ECHILD, WAIT_ERR);
        if(pid > 0){
            fprintf(stderr, "\nProcess with id '%d' exited with status '%d' \n", pid, status);
        }
    }while(pid > 0);
    signal(SIGCHLD, sigchld_handler);
}
#endif

char* HOME_DIR;
char PREVIOUS_DIR[MAX_PATH_LENGTH];
char CURRENT_DIR[MAX_PATH_LENGTH];

int main(int argc, char** argv, char** envp) {

    check(signal(SIGTSTP, SIG_IGN) == SIG_ERR, SIGNAL_ERR);
    check(signal(SIGTERM, SIG_IGN) == SIG_ERR, SIGNAL_ERR);

#ifdef SIGDET
    check(signal(SIGCHLD, sigchld_handler) == SIG_ERR, SIGNAL_ERR);
#endif


    HOME_DIR = getenv("HOME");
    check(HOME_DIR == NULL, HOME_ENV_ERR);

    strcpy(PREVIOUS_DIR, CURRENT_DIR);
    while (TRUE) {
        char cmd_entry[MAX_COMMAND_ENTRY];

        set_current_dir();
        print_prompt();

        if (fgets(cmd_entry, MAX_COMMAND_ENTRY, stdin) != NULL) {
            /* Remove trailing newline */
            int ln = strlen(cmd_entry) - 1;
            if (cmd_entry[ln] == '\n') cmd_entry[ln] = '\0';

            /* */
            if (strlen(cmd_entry) > 0) {
                Commands commands;
                parse_commands(cmd_entry, commands);
                do_commands(commands);
            }
        } else if (feof(stdin)) {
            exit(0);
        }

#ifndef SIGDET
        wait_for_children();
#endif
    }
}

#ifndef SIGDET
void wait_for_children() {
    int pid = 0, status = 0;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        fprintf(stderr, "Process with id '%d' exited with status '%d' \n", pid, status);
    }
}
#endif

void set_current_dir() {
    check(getcwd(CURRENT_DIR, MAX_PATH_LENGTH) == NULL, WORKING_DIR_ERR);
}

void split(char* string, char** string_array, char* delim) {
    char* token;
    token = strtok(string, delim);
    while (token != NULL) {
        *string_array = token;
        string_array++;
        token = strtok(NULL, delim);
    }
    *string_array = NULL;
}

void parse_commands(char cmd_entry[MAX_COMMAND_ENTRY], Commands commands) {
    int i = 0;
    int j = 0;
    int k = 0;
    char* cmd_args[MAX_ARGUMENTS];

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

    split(cmd_entry, cmd_args, "|");

    i = 0;
    while(cmd_args[i] != NULL){
        ++i;
    }

    i = 0;
    while(cmd_args[i] != NULL){
        char* tmp[MAX_ARGUMENTS];
        _parse_commands(tmp, cmd_args[i]);
        j = 0;
        while(tmp[j] != NULL) {
            strcpy(commands[i][j], tmp[j]);
            escape_string(commands[i][j]);
            j++;
        }
        i++;
    }
}

void _parse_commands(char** args, char* cmd_entry) {
    /* Command entry string */
    char *p = cmd_entry;

    /* Loop until end of entry string */
    while(*p != '\0') {

        /* Increment pointer to beginning of argument */
        while(*p == ' ' || *p == '\n') {
            *p = '\0'; /* End of argument */
            p++;
        }

        /* Special cases */
        if (*p == '"') {
            p++;
            *args = p;
            while (*p != '"' && *p != '\0') {
                p++;
            }
            *p = '\0';
            p++;
        } else if (*p == '\'') {
            p++;
            *args = p;
            while (*p != '\'' && *p != '\0') {
                p++;
            }
            *p = '\0';
            p++;
        } else {
            /* Set arg pointer to beginning of argument */
            *args = p;
        }

        /* Increment pointer to end of argument */
        while (*p != ' ' && *p != '\0' && *p != '\n') {
            if (*p == '\\') {
                p++;
            }
            p++;
        }

        /* Increment argument */
        args++;

    }
    *args = NULL;
}

void escape_string(char str[MAX_COMMAND_ENTRY]) {
    char *src = str;
    char *dst = str;
    while(*src != '\0') {
        *dst = *src;
        if(*dst != '\\'){
            dst++;
        }else{
            if(*(dst + 1) == '\\'){
                src++;
                dst++;
            }
        }
        src++;
    }
    *dst = '\0';
}

void do_commands(Commands commands) {
    int i = 0;
    int length = 0;
    Command cmd_one = commands[0];

    /* Check for background process  */
    i = 0;
    while(*cmd_one[i] != '\0') {
        ++i;
    }
    --i;
    length = strlen(cmd_one[i]);

    /* Check if the last char is an & */
    if(cmd_one[i][length-1] == '&') {
        cmd_one[i][length-1] = '\0';
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

    /* add grep */
    if (*commands[0][1] != '\0') {
        int j = 1;
        while(*commands[0][j] != '\0') {
            strcpy(commands[1][j], commands[0][j]);
            *commands[0][j] = '\0';
            j++;
        }
        strcpy(commands[i++][0], "grep");
    }
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
    struct timeval before, after;
    check(gettimeofday(&before, NULL) == -1, TIME_ERR);

    while (**commands[count] != '\0') count++;

    if(!get_process_path(process_path, command[0])){
        printf("Could not found process %s\n", command[0]);
        return;
    }

    pid1 = fork();
    check(pid1 == -1, FORK_ERR);
    if (pid1 == 0) {
        int i = 0;
        while (**commands[i] != '\0') {
            Command command = commands[i];
            char process_path[MAX_PATH_LENGTH];
            int stdout_fd[2];

            check(pipe(stdout_fd) == -1, PIPE_ERR);

            if(!get_process_path(process_path, command[0])){
                printf("Could not found process %s\n", command[0]);
                return;
            }

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

    if(waitpid(pid1, NULL, 0) == -1 && errno != ECHILD){
        perror(WAIT_ERR);
    }

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

int get_process_path(char* process_path, char* process) {
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
        strcpy(process_path, paths[i]);
        strcat(process_path, "/");
        strcat(process_path, process);
        if (file_exists(process_path)) {
            return TRUE;
        }
        i++;
    }

    /* Could not find a path, assign the process as path */
    strcpy(process_path, process);

    return FALSE;
}

int file_exists(char* path) {
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
    strcpy(str, "~");
    strcat(str, CURRENT_DIR + index);
    return str;
}

int starts_with_homedir(char* s) {
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
