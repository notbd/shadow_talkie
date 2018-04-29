/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "lib.h"

static volatile int endSession = 0;

static volatile int routerSocket;
static volatile int talkieSockets[MAX_TALKIES];
static volatile int talkieCount = 0;

static volatile int youAreBigboy = 1;
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
 * Router ending clean up (such as shutting down talkieSockets) is handled here.
 */
void cleanup();

/**
 * Sets up a router connection.
 * Will not accept more than MAX_TALKIES connections.
 * If more than MAX_TALKIES talkies attempts to connect, will shut down
 * the new connection and continues accepting.
 * Once a connection acknowledged, a thread is created and 'process_talkie' will handle
 * that talkie.
 *
 * If any networking call fails, the appropriate error is printed and the
 * function calls exit(1):
 *    - fprtinf to stderr for getaddrinfo
 *    - perror() for any other call
 */
void run_router();

/**
 * Handles the reading to and writing from talkies.
 *
 * p  - index where talkieSockets[(intptr_t)p] is the file descriptor
 * for this talkie.
 *
 * Return value not used.
 */
void *process_talkie(void *p);