#include <wordexp.h>
#include "Parsing.h"

void parse_commands(char cmd_entry[MAX_COMMAND_ENTRY], Commands commands) {
    char* cmd_args[MAX_ARGUMENTS];
    int i = 0;

    clear_commands(commands);

    /* Split command line entry on pipe char
     * - cmd_entry = "foo | bar"
     * - cmd_args = ["foo", "bar"]
     */
    split(cmd_entry, cmd_args, "|", MAX_ARGUMENTS);

    /* Loop through each command and parse it */
    while (cmd_args[i] != NULL) {
        char* args[MAX_ARGUMENTS];
        bool escaped[MAX_ARGUMENTS];
        int j = 0;
        int k = 0;
        int offset = 0;
        int arg_size = 0;

        /* Initiate escaped array to false. */
        while (k < MAX_ARGUMENTS) escaped[k++] = FALSE;

        /* Parse argument
         * - cmd_args[i] = "ls -a"
         * - tmp = ["ls", "-a"]
         */
        parse_args(args, cmd_args[i], escaped);
        arg_size = get_arg_size(args);
        if (arg_size == 0)
            continue;

        /* Copy all arguments to the commands structure */
        strncpy(commands[i][0], args[0], MAX_COMMAND_ENTRY);
        j = 1;
        while(j < arg_size && j + offset < MAX_ARGUMENTS) {
            wordexp_t wordbuf;

            /* Expand arguments using wordexp */
            if (!escaped[j] && wordexp(args[j], &wordbuf, 0) == 0) {
                int k = 0;
                while(k < wordbuf.we_wordc && j + offset + k < MAX_ARGUMENTS){
                    handle_escapes(wordbuf.we_wordv[k]);
                    strncpy(commands[i][j + offset + k], wordbuf.we_wordv[k], MAX_COMMAND_ENTRY);
                    k++;
                }
                offset += k - 1;
                wordfree(&wordbuf);
            } else {
                /* Copy argument as is if it can't be expanded. */
                handle_escapes(args[j]);
                strncpy(commands[i][j + offset], args[j], MAX_COMMAND_ENTRY);
            }
            j++;
        }
        i++;
    }
}

void handle_escapes(char* str){
    int i = 0;
    int len = strlen(str);
    while(str[i] != '\0'){
        if(str[i] == '\\'){
            switch(str[i + 1]){
                case 'a': remove_char(str, i, len--); str[i] = '\a'; break;
                case 'b': remove_char(str, i, len--); str[i] = '\b'; break;
                case 'f': remove_char(str, i, len--); str[i] = '\f'; break;
                case 'n': remove_char(str, i, len--); str[i] = '\n'; break;
                case 'r': remove_char(str, i, len--); str[i] = '\r'; break;
                case 't': remove_char(str, i, len--); str[i] = '\t'; break;
                case 'v': remove_char(str, i, len--); str[i] = '\v'; break;
            }
        }
        i++;
    }
}

void parse_args(char** args, char* cmd_entry, bool escaped[MAX_ARGUMENTS]) {
    /* Command entry string */
    char* p = cmd_entry;
    int i = 0;
    int arg_count = 0;

    /* Loop until end of entry string */
    while (p[i] != '\0') {

        /* Increment pointer to beginning of argument */
        while (p[i] == ' ' || p[i] == '\n') {
            p[i] = '\0'; /* End of argument */
            i++;
        }

        /* Handle quotes */
        if (p[i] == '"' || p[i] == '\'') {
            char quote_char = p[i];
            i++;
            args[arg_count] = p + i;
            escaped[arg_count] = TRUE;
            while (p[i] != quote_char && p[i] != '\0') {
                i++;
            }
            p[i] = '\0';
            i++;
        } else {
            /* Set args pointer to beginning of argument */
            args[arg_count] = p + i;
        }
        /* Increment pointer to end of argument */
        while (p[i] != ' ' && p[i] != '\0' && p[i] != '\n') {
            i++;
        }
        arg_count++;
    }
    args[arg_count] = NULL;
}

void split(char* string, char** string_array, char* delimiters, size_t size) {
    char* token;
    int i = 0;
    token = strtok(string, delimiters);
    while (token != NULL && i < size - 1) {
        *(string_array + i) = token;
        token = strtok(NULL, delimiters);
        i++;
    }
    *(string_array + i) = NULL;
}

void clear_commands(Commands commands) {
    int i = 0;

    while (i < MAX_ARGUMENTS) {
        int j = 0;

        while (j < MAX_ARGUMENTS) {
            int k = 0;

            while (k < MAX_COMMAND_ENTRY) {
                commands[i][j][k] = '\0';
                ++k;
            }
            ++j;
        }
        ++i;
    }
}

int get_arg_size(char* args[MAX_ARGUMENTS]) {
    int i = 0;
    int arg_count = 0;

    while (args[i++] != NULL) arg_count++;

    return arg_count;
}

void remove_char(char str[MAX_COMMAND_ENTRY], int index, size_t len) {
    memmove(&str[index], &str[index + 1], len - index);
}

void print_commands(Commands commands) {
    int i = 0;
    while (**commands[i] != '\0') {
        int j = 0;
        while (*commands[i][j] != '\0') {
            printf("(%i, %i): %s\n", i, j, commands[i][j]);
            j++;
        }
        i++;
    }
}
