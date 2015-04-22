#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_COMMAND_ENTRY 1024
#define MAX_PATH_LENGTH 1024
#define MAX_ARGUMENTS 32

#define TRUE  1
#define FALSE 0


void split(const char *cmd_entry, char *cmd_args[], char *delim);
void do_commands(char *cmd_args[]);
void print_args(char *cmd_args[]);
void print_prompt();
void set_current_dir();
char* create_dir_string(char* str, int index);
int starts_with_homedir(char* s);
int str_cmp(const void* a, const void* b);
int size(void** arr);
int file_exists(char const *path);

/* Commands */
void sah_cd(char *cmd_args[]);
void sah_exit();
void sah_start_process(char *cmd_args[]);
char* get_process(char* process, char* cmd);

char* HOME_DIR;
char PREVIOUS_DIR[MAX_PATH_LENGTH];
char CURRENT_DIR[MAX_PATH_LENGTH];

int running;

int main(int argc, char *argv[], char* envp[]){
    char cmd_entry[MAX_COMMAND_ENTRY];
    size_t envSize;

    HOME_DIR = getenv("HOME");
    if (HOME_DIR != NULL) {
        printf("HOME_DIR: %s\n", HOME_DIR);
    } else {
        printf("%s\n", "Could not set HOME_DIR.");
    }

    envSize = size((void**)envp);
    qsort(envp, envSize, sizeof(char*), str_cmp);

    /*
    for(int i = 0; i < envSize; i++)
        printf("%s\n", envp[i]);
    */

    set_current_dir();

    strcpy(PREVIOUS_DIR, CURRENT_DIR);
    running = TRUE;
    while (running) {
        char *cmd_args[MAX_ARGUMENTS];
        char delim[4] = " \n\t";
        set_current_dir();
        print_prompt();

        if (fgets(cmd_entry, MAX_COMMAND_ENTRY, stdin) != NULL) {
            split(cmd_entry, cmd_args, delim);
        } else {
            printf("%s\n", "Could not fgets.");
            return 0;
        }

        /* print_args(cmd_args); */

        if (cmd_args[0] != NULL) {
            do_commands(cmd_args);
        }
    }

    return 0;
}

int str_cmp(const void* a, const void* b){
    const char** strA = (const char**) a;
    const char** strB = (const char**) b;
    return strcmp(*strA, *strB);
}

int size(void** arr) {
    int i = 0;
    void** a = arr;
    while(*a != 0) {
        a++;
        i++;
    }
    return i;
}

void set_current_dir() {
    if (getcwd(CURRENT_DIR, MAX_PATH_LENGTH) == NULL) {
        printf("%s\n", "Could not get working directory");
        exit(0);
    }
}

void split(const char *cmd_entry, char *cmd_args[], char *delim) {
    char cpy[MAX_PATH_LENGTH]; /* TODO constant size */
    char *token;
    strcpy(cpy, cmd_entry);
    token = strtok(cpy, delim);
    while (token != NULL) {
        *cmd_args = token;
        cmd_args++;
        token = strtok(NULL, delim);
    }
    *cmd_args = NULL;
}

void do_commands(char *cmd_args[]) {
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

/*
------------------------------------------------
-- Shell commands
------------------------------------------------
*/

void sah_exit(){
    running = FALSE;
}

void sah_cd(char *cmd_args[]) {
    char* dir;
    if (cmd_args[0] == NULL){
        dir = HOME_DIR;
    } else if(cmd_args[0][0] == '~'){
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

void sah_start_process(char* cmd_args[]){
    char    process[MAX_PATH_LENGTH];
    char*   cmd;
    int     pid;
    int     link[2];

    cmd = cmd_args[0];
    get_process(process, cmd);
    if(process[0] == '\0'){
        printf("Unknown command: %s\n", cmd);
        return;
    }

    if(pipe(link) == -1){
        printf("Could not create pipe!\n");
        return;
    }

    pid = fork();
    if(pid == -1) {
        /* Error */
        printf("Could not fork process!\n");
    } else if(pid == 0) {
        /* Child process */
        dup2(link[1], STDOUT_FILENO);
        close(link[0]);
        close(link[1]);
        if(execv(process, ++cmd_args) == -1){
            printf("Could not execute program %s\n", process);
        }
    } else {
        char output[512];
        int nbytes = read(link[0], output, sizeof(output));
        close(link[1]);
        printf("Output: (%.*s)\n", nbytes, output);
        wait(NULL);
    }

}

char* get_process(char* process, char* cmd){
    const char* path_env;
    char*       paths[MAX_ARGUMENTS];

    path_env = getenv("PATH");
    if(path_env != NULL){
        int i = 0;
        split(path_env, paths, ":");
        while(paths[i] != NULL) {
            strcpy(process, paths[i]);
            strcat(process, "/");
            strcat(process, cmd);
            if(file_exists(process)){
                return process;
            }
            i++;
        }
    }
    *process = '\0';
    return process;
}

int file_exists(const char* path){
    return access(path, F_OK) == 0 ? TRUE : FALSE;
}

void print_prompt() {
    int index = starts_with_homedir(CURRENT_DIR);
    char tmp[MAX_PATH_LENGTH];
    char* dir = index == -1 ? CURRENT_DIR : create_dir_string(tmp, index);
    printf("%s $ ", dir);
}

char* create_dir_string(char* str, int index){
    strcpy(str, "~");
    strcat(str, CURRENT_DIR + index);
    return str;
}

int starts_with_homedir(char* s) {
    int i = 0;
    while(s[i] == HOME_DIR[i]){
        i++;
        if(HOME_DIR[i] == '\0'){
            return i;
        }
    }
    return -1;
}

void print_args(char *cmd_args[]) {
    int i = 0;
    printf("%s\n", "Args:");
    printf("%s\n", "---");
    while (cmd_args[i] != NULL) {
        printf("%s\n", cmd_args[i]);
        i++;
    }
    printf("%s\n", "---");
}
