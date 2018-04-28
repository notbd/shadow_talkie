/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "lib.h"

static volatile int endSession = 0;
static volatile int routerSocket;
static volatile int talkieSockets[MAX_TALKIES];
static volatile int youAreBigboy = 1;
static volatile int bigboyChanged = 0;
static volatile int talkieCount = 0;
static volatile talkie_info bigboyInfo;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void init_containers();

/**
 * Signal handler for SIGINT.
 * Used to set flag to end router.
 */
void close_router();

/**
 * Cleanup function called in main after `run_router` exits.
 * Router ending clean up (such as shutting down talkie_infos) is handled here.
 */
void cleanup();

/**
 * Sets up a router connection.
 * Will not accept more than MAX_TALKIES connections.
 * If more than MAX_TALKIES talkie_infos attempts to connects, will shut down
 * the new talkie_info and continues accepting.
 * Once a talkie_info acknowledged, a thread is created and 'process_talkie_info' will handle
 * that talkie_info.
 *
 * If any networking call fails, the appropriate error is printed and the
 * function calls exit(1):
 *    - fprtinf to stderr for getaddrinfo
 *    - perror() for any other call
 */
void run_router();

/**
 * Broadcasts the message to all connected talkie_infos.
 *
 * message  - the message to send to all talkie_infos.
 * size     - length in bytes of message to send.
 */
void write_to_talkies(const char *message, size_t size);

/**
 * Handles the reading to and writing from talkie_infos.
 *
 * p  - (void*)intptr_t index where talkie_infoSockets[(intptr_t)p] is the file descriptor
 * for this talkie_info.
 *
 * Return value not used.
 */
void *process_talkie(void *p);