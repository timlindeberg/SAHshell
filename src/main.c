#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>

#define MAX_COMMAND_ENTRY 1024
#define MAX_PATH_LENGTH 1024
#define MAX_ARGUMENTS 32
#define MAX_OUTPUT 65536

#define TRUE  1
#define FALSE 0

#define READ  0
#define WRITE  1

void print_args(char **cmd_args);
void print_prompt();
void print_exec_time(char* process, double time_spent);
void set_current_dir();
void split(const char *cmd_entry, char **cmd_args, char *delim);

int starts_with_homedir(char* s);
int str_cmp(const void* a, const void* b);
int file_exists(char const *path);

char* create_dir_string(char* str, int index);
char* get_process(char* process, char* cmd);

void do_commands(char **cmd_args);
void do_pipeline_commands(char **cmd_args);
void execute(char* process_path, char** command);

/* Commands */
void sah_cd(char **cmd_args);
void sah_exit();
void sah_start_process(char **cmd_args);

char* HOME_DIR;
char PREVIOUS_DIR[MAX_PATH_LENGTH];
char CURRENT_DIR[MAX_PATH_LENGTH];

int RUNNING;

int main(int argc, char **argv, char **envp){
    char cmd_entry[MAX_COMMAND_ENTRY];

    HOME_DIR = getenv("HOME");
    if (HOME_DIR != NULL) {
        printf("HOME_DIR: %s\n", HOME_DIR);
    } else {
        printf("%s\n", "Could not set HOME_DIR.");
    }

    set_current_dir();

    strcpy(PREVIOUS_DIR, CURRENT_DIR);
    RUNNING = TRUE;
    while (RUNNING) {
        char *cmd_args[MAX_ARGUMENTS];
        set_current_dir();
        print_prompt();

        if (fgets(cmd_entry, MAX_COMMAND_ENTRY, stdin) != NULL) {
            char cp[MAX_PATH_LENGTH];
            strtok(cmd_entry, "\n"); /* Remove trailing newline */
            if(strstr(cmd_entry, "|") == NULL){
                /* No pipe in command */
                strcpy(cp, cmd_entry);
                split(cp, cmd_args, " ");
                if(cmd_args[0] != NULL){
                    do_commands(cmd_args);
                }
            }else{
                strcpy(cp, cmd_entry);
                split(cp, cmd_args, "|");
                do_pipeline_commands(cmd_args);
            }
        } else {
            printf("%s\n", "Could not fgets.");
            return 0;
        }
    }

    return 0;
}

void set_current_dir() {
    if (getcwd(CURRENT_DIR, MAX_PATH_LENGTH) == NULL) {
        printf("%s\n", "Could not get working directory");
        exit(0);
    }
}

void split(const char *string, char **string_array, char *delim) {
    char *token;
    token = strtok(string, delim);
    while (token != NULL) {
        *string_array = token;
        string_array++;
        token = strtok(NULL, delim);
    }
    *string_array = NULL;
}

void do_commands(char **cmd_args) {
    if (strcmp("exit", cmd_args[0]) == 0 || strcmp("quit", cmd_args[0]) == 0 ) {
        printf("%s\n", "Exit");
        sah_exit();
    } else if (strcmp("cd", cmd_args[0]) == 0) {
        sah_cd(++cmd_args);
    } else if (strcmp("cd..", cmd_args[0]) == 0) {
        char* s = "..";
        sah_cd(&s);
    } else if (strcmp("checkEnv", cmd_args[0]) == 0) {
        /* TODO */
    } else {
        sah_start_process(cmd_args);
    }
}

void do_pipeline_commands(char **commands) {
    int     i = 0;
    int     count = 0;

    while(commands[count] != NULL) count++;

    if(fork() == 0){
        while (commands[i] != NULL) {
            char*   command[MAX_ARGUMENTS];
            char    cp[MAX_PATH_LENGTH];
            char    process_path[MAX_PATH_LENGTH];
            int     pid;
            char*   process;
            int     stdout_fd[2];

            if (pipe(stdout_fd) == -1) {
                printf("Could not create pipe!\n");
                return;
            }


            strcpy(cp, commands[i]);
            split(cp, command, " ");
            process = command[0];

            get_process(process_path, process);

            if (process_path[0] == '\0') {
                printf("Unknown command: %s\n", process);
                return;
            }

            if(i < count - 1){
                pid = fork();
                if (pid == 0) { /* Child process */
                    /* Redirect fds */
                    dup2(stdout_fd[WRITE], STDOUT_FILENO);
                    close(stdout_fd[WRITE]);
                    execute(process_path, command);
                } else if (pid == -1) {  /* Error */
                    printf("Could not fork process!\n");
                    return;
                }

                dup2(stdout_fd[READ], STDIN_FILENO);
                close(stdout_fd[WRITE]);
            }else{
                /* Use this fork for last process. */
                execute(process_path, command);
            }
            i++;
        }
    }

    wait(NULL);
}

void execute(char* process_path, char** command){
    command[0] = process_path;
    if (execv(process_path, command) == -1) {
        printf("Could not execute program %s\n", process_path);
        exit(1);
    }
}

/*
------------------------------------------------
-- Shell commands
------------------------------------------------
*/

void sah_exit() {
    RUNNING = FALSE;
}

void sah_cd(char **cmd_args) {
    char* dir;
    if (cmd_args[0] == NULL) {
        dir = HOME_DIR;
    } else if (cmd_args[0][0] == '~') {
        char tmp[MAX_PATH_LENGTH];
        strcpy(tmp, HOME_DIR);
        dir = strcat(tmp, ++cmd_args[0]);
    } else if (strcmp("-", cmd_args[0]) == 0) {
        dir = PREVIOUS_DIR;
    } else {
        dir = cmd_args[0];
    }

    if (chdir(dir) != 0) {
        printf("%s: %s\n", "Could not change to", dir);
    } else {
        strcpy(PREVIOUS_DIR, CURRENT_DIR);
    }
}

void sah_start_process(char **cmd_args) {
    char    process[MAX_PATH_LENGTH];
    char*   cmd = cmd_args[0];
    int     pid;

    get_process(process, cmd);
    if (process[0] == '\0') {
        printf("Unknown command: %s\n", cmd);
        return;
    }

    pid = fork();
    if (pid == -1) {
         /* Error */
        printf("Could not fork process!\n");
    } else if (pid == 0) {
        /* Let first arg points to process path */
        cmd_args[0] = process;

        if (execv(cmd_args[0], cmd_args) == -1) {
            printf("Could not execute program %s\n", process);
        }
    } else {
        /* Parent proces */
        double  time_spent;
        clock_t begin = clock();

        /* Wait for child */
        wait(NULL);

        time_spent = (double) (clock() - begin) / CLOCKS_PER_SEC;
        print_exec_time(cmd_args[0], time_spent);
    }
}

char* get_process(char* process, char* cmd) {
    const char* path_env;

    path_env = getenv("PATH");
    if (path_env != NULL){
        char*   paths[MAX_ARGUMENTS];
        char    cp[MAX_PATH_LENGTH];
        int     i = 0;

        strcpy(cp, path_env);
        split(cp, paths, ":");
        while (paths[i] != NULL) {
            strcpy(process, paths[i]);
            strcat(process, "/");
            strcat(process, cmd);
            if (file_exists(process)){
                return process;
            }
            i++;
        }
    }
    *process = '\0';
    return process;
}

int file_exists(const char* path) {
    return access(path, X_OK) == 0 ? TRUE : FALSE;
}

void print_prompt() {
    int     index = starts_with_homedir(CURRENT_DIR);
    char    tmp[MAX_PATH_LENGTH];
    char*   dir = index == -1 ? CURRENT_DIR : create_dir_string(tmp, index);

#ifndef NO_COLORS
    printf("\x1b[1m%s\x1b[0m \x1b[32m$ \x1b[0m", dir);
#else
    printf("%s $ ", dir);
#endif
}

char* create_dir_string(char* str, int index) {
    strcpy(str, "~");
    strcat(str, CURRENT_DIR + index);
    return str;
}

int starts_with_homedir(char* s) {
    int i = 0;
    while (s[i] == HOME_DIR[i]){
        i++;
        if (HOME_DIR[i] == '\0'){
            return i;
        }
    }
    return -1;
}

void print_args(char **cmd_args) {
    int i = 0;
    printf("%s\n", "Args:");
    printf("%s\n", "---");
    while (cmd_args[i] != NULL) {
        printf("%s\n", cmd_args[i]);
        i++;
    }
    printf("%s\n", "---");
}

void print_exec_time(char* process, double time_spent) {
    printf("'%s' total execution time: %f s\n", process, time_spent);
}
