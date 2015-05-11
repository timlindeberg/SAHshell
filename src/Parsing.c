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
