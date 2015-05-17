#ifndef SAHSHELL_READINPUT_H
#define SAHSHELL_READINPUT_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <termios.h>
#include <dirent.h>

#include "Globals.h"
#include "SAHCommands.h"

typedef enum {
    KEY_UP = 65,
    KEY_DOWN,
    KEY_RIGHT,
    KEY_LEFT,
    DEL = 127
} Keys;

typedef void (* KeyFunction)(char buf[80], size_t size, bool ctrl);

#define BUF_SIZE 80
#define CMD_HISTORY_SIZE 5

static int tab_index = 0;
static int buf_pos = 0;
static int chars_in_buf = 0;
static int current_cmd_index = 0;
static int old_cmd_index = 0;
static int num_old_cmds = 0;
static char old_commands[CMD_HISTORY_SIZE][BUF_SIZE];
static char buf_copy[BUF_SIZE];

void remove_char(char str[BUF_SIZE], int index, size_t len);
void add_char(char* str, int index, size_t len);

bool get_char(char* c);
void clear_line();
void move_cursor(int steps);
int get_index();

void key_up(char buf[BUF_SIZE], size_t size, bool ctrl);
void key_down(char buf[BUF_SIZE], size_t size, bool ctrl);
void key_right(char buf[BUF_SIZE], size_t size, bool ctrl);
void key_left(char buf[BUF_SIZE], size_t size, bool ctrl);
KeyFunction get_key_function(char c);

bool char_part_of_keypress(int keys, char c);
bool check_keys(char* buf, size_t size, char c);
void add_command(char* buf, size_t size);
char* get_input(char* prompt, char* buf, size_t size);


#endif /* SAHSHELL_READINPUT_H */
