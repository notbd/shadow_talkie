/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "lib.h"

#include <ncurses.h>

static volatile int closeChat;

static WINDOW *output;
static WINDOW *input;
static WINDOW *boundary;

static FILE *output_file;

void close_chat();

void draw_border(WINDOW *screen);

void create_windows(char *filename);

void write_message_to_screen(const char *format, ...);

void read_message_from_screen(char **buffer);

void destroy_windows();
