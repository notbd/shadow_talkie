/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#pragma once

#include "lib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Prints router usage.
 */
void print_router_usage();

/**
 * Prints talkie usage.
 */
void print_talkie_usage();

/**
 * Prints router greeting message.
 */
void print_router_greeting();

/**
 * Prints talkie greeting.
 */
void print_talkie_greeting_to_window();

/**
 * Prints goodbye message when router or talkie exits.
 */
void print_router_goodbye();

/**
 * Prints info of leaving talkie.
 */
void print_talkie_left(char *name);

void print_talkie_join(char *name, char *ip, int isBigboy);

/**
 * Prints message at talkies when router left.
 */
void print_router_left();
