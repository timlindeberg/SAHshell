#include "Parsing.h"

void parse_commands(char cmd_entry[MAX_COMMAND_ENTRY], Commands commands) {
    char* cmd_args[MAX_ARGUMENTS];
    int i;

    clear_commands(commands);

    /* Split command line entry on pipe char
     * - cmd_entry = "foo | bar"
     * - cmd_args = ["foo", "bar"]
     */
    split(cmd_entry, cmd_args, "|", MAX_ARGUMENTS);

    /* Loop through each command and parse it */
    for(i = 0; cmd_args[i] != NULL; i++) {
        char* args[MAX_ARGUMENTS];
        bool escaped[MAX_ARGUMENTS];
        int j;
        int k;
        int offset = 0;
        int arg_size = 0;

        /* Initiate escaped array to false. */
        for(k = 0; k < MAX_ARGUMENTS; k++) escaped[k] = FALSE;

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
        for(j = 1; j < arg_size && j + offset < MAX_ARGUMENTS; j++) {
            wordexp_t wordbuf;

            /* Expand arguments using wordexp */
            if (!escaped[j] && wordexp(args[j], &wordbuf, 0) == 0) {
                for(k = 0; k < wordbuf.we_wordc && j + offset + k < MAX_ARGUMENTS; k++){
                    replace_escape_symbols(wordbuf.we_wordv[k]);
                    strncpy(commands[i][j + offset + k], wordbuf.we_wordv[k], MAX_COMMAND_ENTRY);
                }
                offset += k - 1;
                wordfree(&wordbuf);
            } else {
                /* Copy argument as is if it can't be expanded. */
                replace_escape_symbols(args[j]);
                strncpy(commands[i][j + offset], args[j], MAX_COMMAND_ENTRY);
            }
        }
    }
}

void replace_escape_symbols(char* str){
    int i;
    int len = strlen(str);

    for(i = 0; str[i] != '\0'; i++){
        char c;
        if(str[i] == '\\' && (c = get_escaped_char(str, i + 1)) != '\0'){
            /* Remove \ and replace the char with the escaped version
             * eg ['\', 'n'] becomes ['\n']. */
            remove_char(str, i, len--);
            str[i] = c;
        }
    }
}

char get_escaped_char(char* str, int i){
    switch(str[i]){
        case 'a': return '\a';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'n': return '\n';
        case 'r': return '\r';
        case 't': return '\t';
        case 'v': return '\v';
        default:  return '\0';
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
            if(p[i] == '\\' && p[i + 1] == ' '){
                i++; /* Skip over escaped whitespace */
            }
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
    for(i = 0; token != NULL && i < size - 1; i++) {
        *(string_array + i) = token;
        token = strtok(NULL, delimiters);
    }
    *(string_array + i) = NULL;
}

void clear_commands(Commands commands) {
    int i, j;
    for(i = 0; i < MAX_ARGUMENTS; i++) {
        for (j = 0; j < MAX_ARGUMENTS; j++) {
            commands[i][j][0] = '\0';
        }
    }
}

int get_arg_size(char* args[MAX_ARGUMENTS]) {
    int arg_count;

    for (arg_count = 0; args[arg_count] != NULL; arg_count++);

    return arg_count;
}

void remove_char(char str[MAX_COMMAND_ENTRY], int index, size_t len) {
    memmove(&str[index], &str[index + 1], len - index);
}

void print_commands(Commands commands) {
    int i, j;
    for(i = 0; **commands[i] != '\0'; i++) {
        for (j = 0; *commands[i][j] != '\0'; j++) {
            printf("(%i, %i): %s\n", i, j, commands[i][j]);
        }
    }
}
