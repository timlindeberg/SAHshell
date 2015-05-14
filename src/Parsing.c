#include "Parsing.h"

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
        bool escaped[MAX_ARGUMENTS];
        int j = 0;
        int k = 0;
        int offset = 0;

        /* Set escaped array to false. */
        while(k < MAX_ARGUMENTS) escaped[k++] = FALSE;

        /* Parse argument
         * - cmd_args[i] = "ls -a"
         * - tmp = ["ls", "-a"]
         */
        parse_args(args, cmd_args[i], escaped);

        /* Copy all arguments to the commands structure */
        while(args[j] != NULL) {
            glob_t globbuf;

            /* Glob arguments */
            if(j > 0 && !escaped[j] && glob(args[j], 0, NULL, &globbuf) == 0){
                int k = 0;
                while(k < globbuf.gl_pathc){
                    strncpy(commands[i][j + offset + k], globbuf.gl_pathv[k], MAX_COMMAND_ENTRY);
                    k++;
                }
                offset += k - 1;
                globfree(&globbuf);
            }else{
                /* Copy argument as is if it can't be globbed. */
                strncpy(commands[i][j + offset], args[j], MAX_COMMAND_ENTRY);
            }

            j++;
        }
        i++;
    }
}

void parse_args(char** args, char* cmd_entry, bool escaped[MAX_ARGUMENTS]) {
    /* Command entry string */
    char *p = cmd_entry;
    int i = 0;
    int argCount = 0;
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
            *(args + argCount) = p + i;
            escaped[argCount] = TRUE;
            while (p[i] != quote_char && p[i] != '\0') {
                i++;
            }
            p[i] = '\0';
            i++;
        } else {
            /* Set args pointer to beginning of argument */
            *(args + argCount) = p + i;
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
        argCount++;
    }
    *(args + argCount) = NULL;
}

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

void remove_char(char str[MAX_COMMAND_ENTRY], int index, size_t len) {
    memmove(&str[index], &str[index + 1], len - index);
}

void print_commands(Commands commands){
    int i = 0;
    while(**commands[i] != '\0'){
        int j = 0;
        while(*commands[i][j] != '\0'){
            printf("(%i, %i): %s\n", i, j, commands[i][j]);
            j++;
        }
        i++;
    }
}
