/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "lib.h"

static volatile int endSession = 0;

static volatile int routerSocket = -1;
static volatile int serverSocket = -1;
static volatile int bigboySocket = -1;

static volatile int talkieSockets[MAX_TALKIES];
static volatile int talkieCount = 0;
static volatile int amIBigboy;

static volatile talkie_info myInfo;
static volatile talkie_info bigboyInfo;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * thread[0] for run_mini_server
 * thread[1] for listen_from_router
 * thread[2] for write_to_bigboy
 * thread[3] for read_from_bigboy
 * 
 */
static pthread_t threads[4];

/**
 * Struct for thread cancellation handler
 * 
 */
typedef struct _thread_cancel_args
{
    char **buffer;
    char **msg;
} thread_cancel_args;

/**
 * Signal handler for SIGINT.
 * Used to set flag to end talkie and cancel running threads.
 * 
 */
void close_talkie();

/**
 * Cleanup function called in main after `run_router` exits.
 * Router ending clean up (such as shutting down talkieSockets) is handled here.
 * 
 */
void cleanup();

/**
 * Sets up the main talkie program.
 * 
 */
void run_talkie();

/**
 * If this talkie got instruction from router to become the bigboy,
 * this function is called to set up the server for chat.
 * 
 */
void *run_mini_server(void *p);

/**
 * Sets up resources for talkie and calls "write_to_bigboy" 
 * and "read_from_bigboy" to handle chatting.
 * 
 */
void run_mini_client();

/**
 * Called by "run_mini_server" as a thread to handle data among talkies.
 * 
 */
void *process_talkie(void *p);

/**
 * Called by "run_mini_server" as a thread to keep monitoring connection 
 * with router.
 * Broadcast message to everyone once router disconnects.
 * 
 */
void *listen_from_router(void *p);

/**
 * Sets up a connection to a chatroom server and returns
 * the file descriptor associated with the connection.
 *
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
 * 
 */
void *write_to_bigboy(void *p);

/**
 * Reads bytes from the bigboy and prints them to the user.
 *
 * 
 */
void *read_from_bigboy(void *p);