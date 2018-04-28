/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "lib.h"

#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static volatile int endSession = 0;
static volatile int routerSocket = -1;
static volatile int serverSocket = -1;
static volatile int bigboySocket = -1;
static volatile int amIBigboy;
static volatile int serverOnline = 0;

static volatile int talkieSockets[MAX_TALKIES];
// static volatile talkie_info talkieInfo[MAX_TALKIES];
static volatile talkie_info myInfo;
static volatile talkie_info bigboyInfo;

static volatile int talkieCount = 0;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t threads[4];

// thread[0] for run_mini_server
// thread[1] for listen_from_router
// thread[2] for write_to_bigboy
// thread[3] for read_from_bigboy

typedef struct _thread_cancel_args
{
    char **buffer;
    char **msg;
} thread_cancel_args;

void *listen_from_router(void *p);
void init_threads();
void close_talkie();
void cleanup();
void run_mini_client();
void init_containers();
void *run_mini_server(void *p);
void run_talkie();
void *process_talkie(void *p);
void write_to_talkies(const char *message, size_t size);


/**
 * Shuts down connection with 'serverSocket'.
 * Called by close_program upon SIGINT.
 */
void close_server_connection();

/**
 * Sets up a connection to a chatroom server and returns
 * the file descriptor associated with the connection.
 *
 * host - Server to connect to.
 * port - Port to connect to server on.
 *
 * Returns integer of valid file descriptor, or exit(1) on failure.
 */
int connect_to_server(const char *host, const char *port);

/**
 * Cleanup routine in case the thread gets cancelled.
 * Ensure buffers are freed if they point to valid memory.
 */
void thread_cancellation_handler(void *arg);

/**
 * Reads bytes from user and writes them to bigboy.
 *
 * arg - void* casting of char* that is the username of talkie.
 */
void *write_to_bigboy(void *p);

/**
 * Reads bytes from the bigboy and prints them to the user.
 *
 * arg - void* requriment for pthread_create function.
 */
void *read_from_bigboy(void *p);