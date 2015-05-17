#include "ReadInput.h"

void add_char(char* str, int index, size_t len) {
    memmove(&str[index + 1], &str[index], len - index);
}

bool get_char(char* c) {
    fd_set set;
    struct timeval tv;
    int res;

    tv.tv_sec = 1000000;
    tv.tv_usec = 0;

    FD_ZERO(&set);
    FD_SET(fileno(stdin), &set);
    res = select(fileno(stdin) + 1, &set, NULL, NULL, &tv);
    fflush(stdout);
    if (res > 0) {
        read(fileno(stdin), c, 1);
        return TRUE;
    }
    else if (res < 0) {
        perror("Could not get char from select.");
        return FALSE;
    }
    else {
        printf("Select timeout\n");
        return FALSE;
    }
}

void clear_line() {
    printf("\033[2K\r");
}

void move_cursor(int steps) {
    if (steps == 0) return;
    /* C steps left and D steps right */
    printf("\033[%i%c", steps, steps > 0 ? 'C' : 'D');
}

int get_index() {
    int index = old_cmd_index - current_cmd_index;
    return (index < 0 ? num_old_cmds + index : index);
}

void key_up(char buf[80], size_t size, bool ctrl) {
    if (current_cmd_index == 0) {
        strncpy(buf_copy, buf, size);
    }
    current_cmd_index++;
    if (current_cmd_index >= num_old_cmds) {
        current_cmd_index = num_old_cmds;
    }

    /* Wrap around */
    strncpy(buf, old_commands[get_index()], size);

    chars_in_buf = strlen(buf);
    buf_pos = chars_in_buf;
}

void key_down(char buf[80], size_t size, bool ctrl) {
    current_cmd_index--;
    if (current_cmd_index <= 0) {
        current_cmd_index = 0;
    }

    strncpy(buf, current_cmd_index == 0 ? buf_copy : old_commands[get_index()], size);

    chars_in_buf = strlen(buf);
    buf_pos = chars_in_buf;
}

void key_right(char buf[80], size_t size, bool ctrl) {
    if (buf_pos < chars_in_buf) {
        int j = 1;
        if (ctrl) {
            if (buf[buf_pos] == ' ') {
                for (j = buf_pos; j < chars_in_buf && buf[j] == ' '; j++); /* Move to next space */
            } else {
                for (j = buf_pos; j < chars_in_buf && buf[j] != ' '; j++);
            }
            j = j - buf_pos;
        }
        move_cursor(j);
        buf_pos += j;
    }
}

void key_left(char buf[80], size_t size, bool ctrl) {
    if (buf_pos > 0) {
        int j = 1;
        if (ctrl) {
            if (buf[buf_pos] == ' ') {
                for (j = buf_pos; j > 0 && buf[j] == ' '; j--); /* Move to next space */
            } else {
                for (j = buf_pos; j > 0 && buf[j] != ' '; j--);
            }
            j = buf_pos - j;
        }
        move_cursor(-j);
        buf_pos -= j;
    }
}

KeyFunction get_key_function(char c) {
    switch (c) {
        case KEY_UP:    return key_up;
        case KEY_DOWN:  return key_down;
        case KEY_RIGHT: return key_right;
        case KEY_LEFT:  return key_left;
        default:        return NULL;
    }
}

bool char_part_of_keypress(int keys, char c) {
    switch (keys) {
        case 0:  return c == 27;
        case 1:  return c == 91;
        case 2:  return c == 49;
        case 3:  return c == 59;
        case 4:  return c == 53;
        default: return FALSE;
    }
}

bool check_keys(char* buf, size_t size, char c) {
    int keys = 0;
    while (char_part_of_keypress(keys, c)) {
        keys++;
        if (!get_char(&c)) {
            break;
        }
        if (keys == 2 || keys == 5) {
            bool ctrl = keys == 5;
            KeyFunction f = get_key_function(c);
            if (f != NULL) {
                f(buf, size,  ctrl);
                return TRUE;
            }
        }
    }
    return FALSE;
}

void add_command(char* buf, size_t size){
    strncpy(old_commands[old_cmd_index], buf, size);
    if (++num_old_cmds >= CMD_HISTORY_SIZE) {
        num_old_cmds = CMD_HISTORY_SIZE;
    }
    old_cmd_index = (old_cmd_index + 1) % CMD_HISTORY_SIZE;
}

bool starts_with(const char *pre, const char *str)
{
    size_t lenpre = strlen(pre),
            lenstr = strlen(str);
    return lenstr < lenpre ? FALSE : strncmp(pre, str, lenpre) == 0;
}

size_t strlstchar(const char *str, const char ch)
{
    char *chptr = strrchr(str, ch);
    if(chptr == NULL){
        return 0;
    }
    return chptr - str + 1;
}

void expand(char* buf, size_t size){
    char* exp = strchr(buf, ' ');
    DIR* dir;
    char* dir_str = malloc(800);
    struct dirent *ent;
    int len, len2;
    int lol;

    if(exp == NULL)
        return;
    exp++; /* Ignore space */
    len = strlen(exp);

    lol = strlstchar(exp, '/');
    printf("lol: %i\n", lol);
    dir_str[0] = '.';
    dir_str[1] = '/';
    strncpy(dir_str + 2, exp, lol);
    len2 = strlen(dir_str);
    printf("dir_str, %i: %s\n", len2, dir_str);

    fflush(stdout);
    if ((dir = opendir (len2 > 0 ? exp : ".")) != NULL) {
        int i = 0;
        /* print all the files and directories within directory */
        while ((ent = readdir (dir)) != NULL) {
            char* file_name = ent->d_name;
            printf("%s\n", file_name);

            if(starts_with(".", file_name)){
                continue;
            }
            if(len <= 0 || starts_with(exp, file_name)){
                if(i == tab_index){
                    char* tmp = strchr(file_name, ' ');
                    if(tmp != NULL){
                        int len = strlen(file_name);
                        char* cpy = malloc(len + 1);
                        int index = file_name - tmp;

                        if(index < 0) index = -index;
                        strcpy(cpy, file_name);
                        add_char(cpy, index, len);
                        cpy[index] = '\\';
                        strncpy(exp, cpy, ent->d_namlen + 1);
                        free(cpy);
                    }else{
                        strncpy(exp, file_name, ent->d_namlen);
                    }
                    tab_index++;
                    break;
                }
                i++;
            }
        }
        closedir (dir);
    } else {
        /* could not open directory */
        perror ("");
    }
    chars_in_buf = strlen(buf);
    buf_pos = chars_in_buf;
    printf("lol: %s\n", exp);
}

char* get_input(char* prompt, char* buf, size_t size) {
    struct termios old_settings, new_settings;
    memset(buf, '\0', size);

    tcgetattr(fileno(stdin), &old_settings);
    new_settings = old_settings;
    new_settings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &new_settings);

    while (TRUE) {
        char c;

        clear_line();
        printf("%s%s", prompt, buf); /* print buffer at begining of line */
        move_cursor(-(chars_in_buf - buf_pos));
        fflush(stdout);

        if (!get_char(&c)) {
            break;
        }
        /* Check for arrow key input.*/
        if (check_keys(buf, size, c)) {
            tab_index = 0;
            continue;
        }

        if (c == '\n') { /* Stop reading input on newline */
            break;
        } else if(c == 4) { /* EOF */
            sah_exit(0);
        }else if(c == '\t'){
            /*expand(buf, size); */
        } else if (c == DEL) { /* DEL char */
            if (buf_pos > 0) {
                remove_char(buf, buf_pos - 1, chars_in_buf);
                buf_pos--;
                chars_in_buf--;
                buf[chars_in_buf] = '\0';
                tab_index = 0;
            }
        } else {
            if (chars_in_buf < size) {
                add_char(buf, buf_pos, chars_in_buf);
                chars_in_buf++;
                buf[buf_pos] = c;
                buf_pos++;
                tab_index = 0;
            }
        }
    }

    /* Reset global variables */
    current_cmd_index = 0;
    buf_pos = 0;
    chars_in_buf = 0;
    tcsetattr(fileno(stdin), TCSANOW, &old_settings);
    printf("\n");
    if(strlen(buf) > 0){
        add_command(buf, size);
        return buf;
    }else{
        return NULL;
    }
}
