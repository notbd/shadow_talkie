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
 * Prints router greeting message.
 */
void print_router_greeting();

/**
 * Prints talkie joining message on router.
 */
void print_talkie_join(char *name, char *ip, int isBigboy);

/**
 * Prints talkie leaving message on router.
 */
void print_talkie_left(char *name);

/**
 * Prints goodbye message when router exits.
 */
void print_router_goodbye();

/**
 * Prints talkie usage.
 */
void print_talkie_usage();

/**
 * Prints talkie greeting message on chat window.
 */
void print_talkie_greeting_to_window();
