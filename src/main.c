#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_COMMAND_ENTRY 1024
#define MAX_PATH_LENGTH 1024
#define MAX_ARGUMENTS 32

#define TRUE  1
#define FALSE 0

void parse_commands(char *cmd_entry, char *cmd_args[]);
void do_commands(char *cmd_args[]);
void print_args(char *cmd_args[]);
void print_prompt();
void set_current_dir();

void sah_cd(char *cmd_args[]);

char* HOME_DIR;
char PREVIOUS_DIR[MAX_PATH_LENGTH];
char CURRENT_DIR[MAX_PATH_LENGTH];

int main(int argc, char *argv[]){
    char cmd_entry[MAX_COMMAND_ENTRY];

    HOME_DIR = getenv("HOME");
    if (HOME_DIR != NULL) {
        printf("HOME_DIR: %s\n", HOME_DIR);
    } else {
        printf("%s\n", "Could not set HOME_DIR.");
    }

    set_current_dir();

    strcpy(PREVIOUS_DIR, CURRENT_DIR);

    while (TRUE) {
        char *cmd_args[MAX_ARGUMENTS];

        set_current_dir();
        print_prompt();

        if (fgets(cmd_entry, MAX_COMMAND_ENTRY, stdin) != NULL) {
            parse_commands(cmd_entry, cmd_args);
        } else {
            printf("%s\n", "Could not fgets.");
            return 0;
        }

        print_args(cmd_args);

        if (cmd_args[0] != NULL) {
            do_commands(cmd_args);
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

void parse_commands(char *cmd_entry, char *cmd_args[]) {
    char s[4] = " \n\t";
    char *token = strtok(cmd_entry, s);
    while (token != NULL) {
        *cmd_args = token;
        cmd_args++;
        token = strtok(NULL, s);
    }
    *cmd_args = NULL;
}

void do_commands(char *cmd_args[]) {
    if (strcmp("exit", cmd_args[0]) == 0) {
        printf("%s\n", "Exit");
        exit(0);
    } else if (strcmp("cd", cmd_args[0]) == 0) {
        sah_cd(++cmd_args);
    } else if (strcmp("checkEnv", cmd_args[0]) == 0) {

    } else {

    }
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

void print_prompt() {
    printf("%s $ ", CURRENT_DIR);
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
