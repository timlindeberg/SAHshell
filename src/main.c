#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>

#define MAX_COMMAND_ENTRY 1024
#define MAX_PATH_LENGTH 1024
#define MAX_ARGUMENTS 32
#define MAX_OUTPUT 65536

#define TRUE  1
#define FALSE 0

#define READ  0
#define WRITE  1


void print_args(char** cmd_args);
void print_prompt();
void print_exec_time(struct timeval before, struct timeval after);

void set_current_dir();
void split(char* cmd_entry, char** cmd_args, char* delim);

int starts_with_homedir(char* s);
int file_exists(char* path);
int get_process(char* process, char* cmd);

char* create_dir_string(char* str, int index);
char* get_pager(char** pager);

void do_commands(char** cmd_args);
void do_pipeline_commands(char** cmd_args);
void execute(char* process_path, char** command);

/* Commands */
void sah_check_env(char** cmd_args, char** cmd);
void sah_cd(char** cmd_args);
void sah_exit();
void sah_start_processes(char** cmd_args);

char* HOME_DIR;
char PREVIOUS_DIR[MAX_PATH_LENGTH];
char CURRENT_DIR[MAX_PATH_LENGTH];

int RUNNING;

int main(int argc, char** argv, char** envp) {
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
        char* cmd_args[MAX_ARGUMENTS];
        set_current_dir();
        print_prompt();

        if (fgets(cmd_entry, MAX_COMMAND_ENTRY, stdin) != NULL) {
            char cp[MAX_PATH_LENGTH];
            strtok(cmd_entry, "\n"); /* Remove trailing newline */
            strcpy(cp, cmd_entry);
            split(cp, cmd_args, "|");
            do_commands(cmd_args);
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

void do_commands(char** cmd_args) {
    char    cp[MAX_PATH_LENGTH];
    char*   cmd[MAX_ARGUMENTS];
    int     count = 0;

    strcpy(cp, cmd_args[0]);
    split(cp, cmd, " ");

    while(cmd[count] != NULL) count++;

    if(strcmp("&", cmd[count - 1]) == 0){
        cmd[count - 1] = NULL;
        sah_start_background_process(cmd);
        return;
    }

    if (strcmp("exit", cmd[0]) == 0 || strcmp("quit", cmd[0]) == 0 ) {
        printf("%s\n", "Exit");
        sah_exit();
    } else if (strcmp("cd", cmd[0]) == 0) {
        sah_cd(cmd + 1);
    } else if (strcmp("cd..", cmd[0]) == 0) {
        char* s = "..";
        sah_cd(&s);
    } else if (strcmp("checkEnv", cmd[0]) == 0) {
        sah_check_env(cmd_args, cmd);
    } else {
        sah_start_processes(cmd_args);
    }
}

char* get_pager(char** pager) {
    *pager = getenv("PAGER");
    if (*pager == NULL) {
        char cp[MAX_OUTPUT];
        *pager = get_process(cp, "less") ? "less" :
                 get_process(cp, "more") ? "more" :
                 NULL;
    }
    return *pager;
}

void sah_check_env(char** cmd_args, char** cmd) {
    char* cmds[MAX_ARGUMENTS];
    char grep[MAX_PATH_LENGTH];
    char* pager = NULL;
    int i = 0;
    cmds[i++] = "printenv";
    if (cmd[1] != NULL) {
        strcpy(grep, "grep ");
        strcat(grep, cmd_args[0] + sizeof("checkEnv"));
        cmds[i++] = grep;
    }
    if (get_pager(&pager) == NULL) {
        printf("Could not find pagers more or less.");
        return;
    }
    cmds[i++] = "sort";
    cmds[i++] = pager;
    cmds[i] = NULL;
    sah_start_processes(cmds);
}

void sah_start_background_process(char** command){
    char    process_path[MAX_PATH_LENGTH];
    char*   process;
    int     pid;
    process = command[0];

    if (!get_process(process_path, process)) {
        printf("Unknown command: %s\n", process);
        return;
    }

    pid = fork();
    if (pid == 0) { /* Child process */
        execute(process_path, command);
    } else if (pid == -1) {  /* Error */
        printf("Could not fork process!\n");
        return;
    }
}

void sah_start_processes(char** commands) {
    int     count = 0;
    struct timeval before, after;
    double time_elapsed = 0.0;
    gettimeofday(&before, NULL);
    
    while (commands[count] != NULL) count++;

    if (fork() == 0) {
        int i = 0;
        while (commands[i] != NULL) {
            char* command[MAX_ARGUMENTS];
            char cp[MAX_PATH_LENGTH];
            char process_path[MAX_PATH_LENGTH];
            char* process;
            int stdout_fd[2];

            if (pipe(stdout_fd) == -1) {
                printf("Could not create pipe!\n");
                return;
            }

            strcpy(cp, commands[i]);
            split(cp, command, " ");
            process = command[0];

            if (!get_process(process_path, process)) {
                printf("Unknown command: %s\n", process);
                return;
            }

            if (i < count - 1) {
                int pid = fork();
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
            } else {
                /* Use this fork for last process. */
                execute(process_path, command);
            }
            i++;
        }
    }

    wait(NULL);
    gettimeofday(&after, NULL);

    print_exec_time(before, after);
}

void execute(char* process_path, char** command) {
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

void sah_cd(char** cmd_args) {
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

int get_process(char* process, char* cmd) {
    char* path_env;

    path_env = getenv("PATH");
    if (path_env != NULL) {
        char* paths[MAX_ARGUMENTS];
        char cp[MAX_PATH_LENGTH];
        int i = 0;

        strcpy(cp, path_env);
        split(cp, paths, ":");
        while (paths[i] != NULL) {
            strcpy(process, paths[i]);
            strcat(process, "/");
            strcat(process, cmd);
            if (file_exists(process)) {
                return TRUE;
            }
            i++;
        }
    }
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

void print_args(char** cmd_args) {
    int i = 0;
    printf("%s\n", "Args:");
    printf("%s\n", "---");
    while (cmd_args[i] != NULL) {
        printf("%s\n", cmd_args[i]);
        i++;
    }
    printf("%s\n", "---");
}

void print_exec_time(struct timeval before, struct timeval after) {
    double time_elapsed = (after.tv_sec - before.tv_sec) * 1000.0;
    time_elapsed += (after.tv_usec - before.tv_usec) / (1000.0 * 1000.0);
    printf("Execution time: %f s\n", time_elapsed);
}
