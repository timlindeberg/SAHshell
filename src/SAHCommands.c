#include "SAHCommands.h"
#include "SAHShell.h"

void sah_check_env(Commands commands) {
    char* pager = NULL;
    int i = 0;

    strcpy(commands[i++][0], "printenv");

    /* Add grep to command */
    if (*commands[0][1] != '\0') {
        int j = 1;
        while (*commands[0][j] != '\0') {
            strncpy(commands[1][j], commands[0][j], MAX_COMMAND_ENTRY);
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
    strncpy(commands[i++][0], pager, MAX_COMMAND_ENTRY);

    sah_start_processes(commands);
}

void sah_cd(Command command) {
    char* dir;
    char tmp[MAX_PATH_LENGTH];
    char* path = command[1];

    dir = path[0] == '\0'        ? HOME_DIR :
          path[0] == '~'         ? strcat(strncpy(tmp, HOME_DIR, MAX_COMMAND_ENTRY), ++path) :
          strcmp(path, "-") == 0 ? PREVIOUS_DIR :
          /* else */               path;

    if (chdir(dir) != 0) {
        printf("%s: %s\n", "Could not change to", dir);
    } else {
        strncpy(PREVIOUS_DIR, CURRENT_DIR, MAX_PATH_LENGTH);
    }
}

void sah_exit(int status) {
#ifndef SIGDET
    /* Wait for and terminate children */
    wait_for_children();
#endif /* SIGDET */
    exit(status);
}

void sah_start_processes(Commands commands) {
    int count = 0;
    int pid1 = 0;
    int i = 0;
    struct timeval before, after;

    while (**commands[count] != '\0') count++;

    while (i < count) {
        char process_path[MAX_PATH_LENGTH];
        Command command = commands[i];
        if (!get_process_path(process_path, command[0])) {
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

        /* Clear SIGINT signal */
        check(signal(SIGINT, SIG_DFL) == SIG_ERR, SIGNAL_ERR);

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
                check(close(stdout_fd[WRITE]) == -1, CLOSE_ERR);
            } else {
                /* Use this fork for last process. */
                execute(process_path, command);
            }
            i++;
        }
    }

    /* Wait for executed process  */
    check(waitpid(pid1, NULL, 0) == -1 && errno != ECHILD, WAIT_ERR);

    check(gettimeofday(&after, NULL) == -1, TIME_ERR);
    print_exec_time(before, after);
}

void sah_start_background_process(Command command) {
    char process_path[MAX_PATH_LENGTH];
    char* process;
    int pid;

    process = command[0];
    if (!get_process_path(process_path, process)) {
        printf("Could not find process %s\n", process);
        return;
    }

    pid = fork();
    check(pid == -1, FORK_ERR);

    if (pid == 0) {
        execute(process_path, command);
    }

    printf("PID: %d\n", pid);
}

void execute(char* process_path, Command command) {
    char* cmd_args[MAX_ARGUMENTS];
    int i = 1;
    cmd_args[0] = process_path;
    while (*command[i] != '\0') {
        cmd_args[i] = command[i];
        i++;
    }
    cmd_args[i] = NULL;
    if (execv(process_path, cmd_args) == -1) {
        printf("Could not execute program %s\n", process_path);
        sah_exit(0);
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

bool get_process_path(char* process_path, char* process) {
    char* path_env;
    char* paths[MAX_ARGUMENTS];
    char cp[MAX_PATH_LENGTH];
    int i = 0;

    if (file_is_executable(process)) {
        strncpy(process_path, process, MAX_PATH_LENGTH);
        return TRUE;
    }

    path_env = getenv("PATH");
    check(path_env == NULL, PATH_ENV_ERR);

    strncpy(cp, path_env, MAX_PATH_LENGTH);
    split(cp, paths, ":", MAX_ARGUMENTS);
    while (paths[i] != NULL) {
        sprintf(process_path, "%s/%s", paths[i], process);
        if (file_is_executable(process_path)) {
            return TRUE;
        }
        i++;
    }

    return FALSE;
}

bool file_is_executable(char* path) {
    struct stat sb;
    stat(path, &sb);
    if((sb.st_mode & S_IFMT) == S_IFDIR){
        return FALSE;
    }
    return access(path, X_OK) == 0 ? TRUE : FALSE;
}

void print_exec_time(struct timeval before, struct timeval after) {
    double time_elapsed = (after.tv_sec - before.tv_sec) * 1000.0;
    time_elapsed += (after.tv_usec - before.tv_usec) / 1000.0;
    printf("Execution time: %f s\n", time_elapsed / 1000.0);
}
