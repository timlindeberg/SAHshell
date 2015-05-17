#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>

typedef int bool;
#define TRUE  1
#define FALSE 0

#define BUF_SIZE 80

int i = 0;
int chars_in_buf = 0;

void remove_char(char str[BUF_SIZE], int index, size_t len) {
    memmove(&str[index], &str[index + 1], len - index);
}

void add_char(char str[BUF_SIZE], int index, size_t len) {
    memmove(&str[index + 1], &str[index], len - index);
}

bool get_char(char* c){
    fd_set set;
    struct timeval tv;

    tv.tv_sec = 1000000;
    tv.tv_usec = 0;

    FD_ZERO(&set);
    FD_SET(fileno(stdin), &set);
    int res = select(fileno(stdin) + 1, &set, NULL, NULL, &tv);
    fflush(stdout);
    if (res > 0) {
        read(fileno(stdin), c, 1);
        return TRUE;
    }
    else if (res < 0) {
        perror("select error");
        return FALSE;
    }
    else {
        printf("Select timeout\n");
        return FALSE;
    }
}

void clear_line(){
    printf("\033[2K\r");
}

void move_cursor(int steps){
    if(steps == 0) return;
    if(steps > 0){
        printf("\033[%iC", steps);
    }else{
        printf("\033[%iD", -steps);
    }
}

void clear_key(int a[2]){
    a[0] = FALSE;
    a[1] = FALSE;
}

void key_up(char buf[80]){
    printf("key up!\n");
}

void key_down(char buf[80]){
    printf("key down!\n");
}

void key_right(char buf[80]){
    if(i < chars_in_buf){
        move_cursor(1);
        i++;
    }
}

void key_left(char buf[80]){
    if(i > 0){
        move_cursor(-1);
        i--;
    }
}

bool check_keys(char* buf, char c){
    static int key[2] = {FALSE, FALSE};

    if(c == 27){
        key[0] = TRUE;
        return TRUE;
    }else if(key[0] && c == 91){
        key[1] = TRUE;
        return TRUE;
    }else if(key[0] && key[1]){
        switch(c){
        case 65: key_up(buf);       return TRUE;
        case 66: key_down(buf);     return TRUE;
        case 67: key_right(buf);    return TRUE;
        case 68: key_left(buf);     return TRUE;
        }
    }
    clear_key(key);
    return FALSE;
}

int main() {
    struct termios oldSettings, newSettings;
    char buf[80];

    tcgetattr(fileno(stdin), &oldSettings);
    newSettings = oldSettings;
    newSettings.c_lflag &= (~ICANON & ~ECHO);
    tcsetattr(fileno(stdin), TCSANOW, &newSettings);
    while (1) {
        char c;

        //int chars_to_move = chars_in_buf - i;
        //clear_line();
        //printf("%s", buf); /* print buffer at begining of line */
        //move_cursor(-chars_to_move);
        //fflush(stdout);

        if(!get_char(&c)){
            break;
        }

        printf("\n\nchar: %c", c);
        fflush(stdout);

        continue;
        /* Check for arrow key input */
        if(check_keys(buf, c)){
            continue;
        }

        if(c == '\n'){
            break;
        }else if(c == 127){ /* DEL char */
            if(i > 0){
                remove_char(buf, i - 1, chars_in_buf);
                i--;
                chars_in_buf--;
                buf[chars_in_buf] = '\0';
            }
        }else{
            if(chars_in_buf < BUF_SIZE){
                add_char(buf, i, chars_in_buf);
                chars_in_buf++;
                buf[i] = c;   
                i++;
            }
        }

    }
    
    printf("\n%s\n", buf);
    tcsetattr(fileno(stdin), TCSANOW, &oldSettings);
    return 0;
}