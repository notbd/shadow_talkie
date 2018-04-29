/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "includes/format.h"

void print_router_usage()
{
    fprintf(stderr, "./router usage:\nno arguments");
}

void print_router_greeting()
{
    printf("[ROUTER] Greetings! Router has been set up.\n[ROUTER] Now waiting for talkies to join the party... \n");
    printf("============================================================== \n");
}

void print_talkie_join(char *name, char *ip, int isBigboy)
{
    printf("[ROUTER] %s %sjoined on %s.\n", name, isBigboy ? "(Bigboy) " : "", ip);
}

void print_talkie_left(char *name)
{
    printf("[ROUTER] %s has left.\n", name);
}

void print_router_goodbye()
{
    printf("\n[ROUTER] Catcha later!\n");
}

void print_talkie_usage()
{
    fprintf(stderr, "./talkie usage:\nno arguments");
}

void print_talkie_greeting_to_window()
{
    write_message_to_screen("[TALKIE] Welcome to shadow talkie!\n[TALKIE] Please enter your username (no more than %d characters): \n", USERNAME_LENGTH - 1);
}