#include "SAHShell.h"

int main(int argc, char** argv, char** envp) {
    /* Signal handling */
    check(signal(SIGTERM, sah_exit) == SIG_ERR, SIGNAL_ERR);
    check(signal(SIGINT, SIG_IGN) == SIG_ERR, SIGNAL_ERR);
    check(signal(SIGTSTP, SIG_IGN) == SIG_ERR, SIGNAL_ERR);

#ifdef SIGDET
    /* Listen to when a child process exits */
    check(signal(SIGCHLD, sigchld_handler) == SIG_ERR, SIGNAL_ERR);
#endif /* SIGDET */

    /* Get the home directory path */
    HOME_DIR = getenv("HOME");
    check(HOME_DIR == NULL, HOME_ENV_ERR);

    /* Get the current directory and assign it as previous directory */
    set_current_dir();
    strncpy(PREVIOUS_DIR, CURRENT_DIR, MAX_PATH_LENGTH);

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
#if SIGDET && __MACH__
                /* Workaround since wordexp(3) is bugged on mac and crashes when SIGCHLD is handled.
                 * To resolve it we temporarily set the signal handler of SIGCHLD to default.*/
                check(signal(SIGCHLD, SIG_DFL) == SIG_ERR, SIGNAL_ERR);
                parse_commands(cmd_entry, commands);
                check(signal(SIGCHLD, sigchld_handler) == SIG_ERR, SIGNAL_ERR);
#else
                parse_commands(cmd_entry, commands);
#endif
                execute_commands(commands);
            }
        } else if (feof(stdin)) {
            sah_exit(0);
        }

#ifndef SIGDET
        /* Wait for and terminate children */
        wait_for_children();
#endif /* SIGDET */
    }
}

void execute_commands(Commands commands) {
    Command cmd_one = commands[0];

    /* Check for background process  */
    if(is_background_command(cmd_one)){
        sah_start_background_process(cmd_one);
        return;
    }

    if (strcmp("exit", cmd_one[0]) == 0 || strcmp("quit", cmd_one[0]) == 0) {
        sah_exit(0);
    } else if (strcmp("cd", cmd_one[0]) == 0) {
        sah_cd(cmd_one);
    } else if (strcmp("checkEnv", cmd_one[0]) == 0) {
        sah_check_env(commands);
    } else {
        sah_start_processes(commands);
    }
}

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

void set_current_dir() {
    check(getcwd(CURRENT_DIR, MAX_PATH_LENGTH) == NULL, WORKING_DIR_ERR);
}

void print_prompt() {
    char tmp[MAX_PATH_LENGTH];
    char* dir = create_dir_string(tmp);
    char* name;

    name = getenv("USER");
    check(name == NULL, USER_ENV_ERR);

    /* Uses ANSI colors. */
    printf("\x1b[34m%s\x1b[0m: \x1b[1m%s\x1b[0m\x1b[32m $ \x1b[0m", name, dir);
}

char* create_dir_string(char* str) {
    int index = starts_with_homedir(CURRENT_DIR);
    if(index == -1){
        return CURRENT_DIR;
    }

    sprintf(str, "~%s", CURRENT_DIR + index);
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

#ifdef SIGDET

void sigchld_handler(int signo) {
    int pid, status;
    assert(signo == SIGCHLD);

    /* Listen to when a child process exits again */
    signal(SIGCHLD, sigchld_handler);

    do {
        pid = waitpid(-1, &status, WNOHANG);
        check(pid == -1 && errno != ECHILD, WAIT_ERR);
        if(pid > 0){
            printf("\nProcess with id '%d' exited with status '%d' \n", pid, status);
        }
    } while(pid > 0);
}

#else

void wait_for_children() {
    int pid = 0, status = 0;
    while((pid = waitpid(-1, &status, WNOHANG)) > 0){
        fprintf(stderr, "Process with id '%d' exited with status '%d' \n", pid, status);
    }
}

#endif /* SIGDET */
