/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#pragma once

#include "window.h"
#include "format.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <curses.h>
#include <errno.h>
#include <netdb.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define UpTo(i, n) for (size_t i = 0; (i) < (n); (i)++)

/**
 * The numeric host address string for router to establish routing connection
 * 
 * ## !! IMPORTANT !! make sure it is the correct router address before running program
 * ## otherwise will not be able to successfully establish connection
 * 
 */
#define ROUTER_HOST "10.195.87.81"
// #define ROUTER_HOST "192.168.86.44"

/**
 * The numeric local host address string for bigboy to loop back its connection
 * 
 */
#define LOCAL_HOST "127.0.0.1"

/**
 * The decimal port number for router to establish routing connection
 * 
 */
#define ROUTER_PORT "40241"

/**
 * The decimal port number for talkies to establish p2p connection
 * 
 */
#define SERVER_PORT "24140"

/**
 * The largest number of talkies than can join
 * 
 */
#define MAX_TALKIES 8

/**
 * The number of digits for a single piece of header information
 * 
 */
#define MESSAGE_SIZE_DIGITS 4

/**
 * The largest size the message can be that a talkie sends to the server
 * 
 */
#define MSG_SIZE 256

/**
 * The array size for username in talkie_info. Actual max username length is less by 1
 * 
 */
#define USERNAME_LENGTH 16

/**
 * Macro for specifying "chat message" data type in our protocol
 * 
 */
#define TYPE_CHAT_MSG 0

/**
 * Macro for specifying "talkie_info" data type in our protocol
 * 
 */
#define TYPE_INFO 1

/**
 * Macro for specifying "other message" data type in our protocol
 * 
 */
#define TYPE_OTHER_MSG 2

/**
 * Message sent from router to bigboy when it exits
 * 
 */
#define ROUTER_DISCONNECT_MESSAGE "ROUTER-HAS-DISCONNECTED"

/**
 * Message sent from non-bigboy to bigboy when it exits
 * 
 */
#define TALKIE_DISCONNECT_MESSAGE "TALKIE-HAS-DISCONNECTED"

/**
 * Message broadcast from bigboy to talkies when router exits
 * 
 */
#define ROUTER_DISCONNECT_NOTI "[TALKIE] Router has detached... but keep rocking the p2p party!"

    /**
 * Struct that provides information about a talkie
 * 
 */
    typedef struct talkie_info
{
    char ip[INET_ADDRSTRLEN];
    char name[USERNAME_LENGTH]; // user name should not be more than 16 Bytes.
    // add other data here if needed
} talkie_info;

/**
 * Broadcast a single piece of message to all active talkies.
 * NOTE: array not locked - needs to lock mutex before calling this function.
 *
 * socketsArray - a pointer to the array of target sockets
 * buffer   - contains the data to send
 * size     - the size of the data and SHOULD NOT BE ZERO
 * type     - 0 if data should be interpreted as chat message;
 *            1 if nodeinfo, 2 if other data;
 * 
 * Returns the number of successful reach or -1 on failure.
 */
ssize_t broadcast_data(const int *socketsArray, const char *buffer, size_t size, int type);

/**
 * Read a single piece of message from the socket.
 *
 * socket   - the socket which is being read from
 * buffer   - will be populated with the data received and it is user's
 *            responsibility to free the buffer after using it
 * type     - 0 if data should be interpreted as chat message;
 *            1 if nodeinfo, 2 if other data;
 * 
 * Returns the size of the message received, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t fetch_data(int socket, char **buffer, int *type);

/**
 * Send a single piece of message to the socket.
 *
 * socket   - the socket which is being sent to
 * buffer   - contains the data to send
 * size     - the size of the data and SHOULD NOT BE ZERO
 * type     - 0 if data should be interpreted as chat message;
 *            1 if nodeinfo, 2 if other data;
 *
 * Returns the size of the message sent, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t send_data(int socket, const char *buffer, size_t size, int type);

/**
 * Get a string containing the local IPv4 address of the default network interface.
 * Supported default interface names include "eth0", "en0" and "wlp2s0".
 *
 * Returns a copy of the ip address in the dotted-decimal format.
 * The caller is responsible to free the string after use.
 */
char *get_local_ipv4_ip();

/**
 * Builds a message in the form of
 * <name>: <message>\n
 *
 * Returns a char* to the created message on the heap
 */
char *create_message(char *name, char *message);

/**
 * Read the first four bytes from socket and transform it into ssize_t
 *
 * Returns the size of the incomming message,
 * 0 if socket is disconnected, -1 on failure
 */
ssize_t get_message_size(int socket);

/**
 * Writes the bytes of size to the socket
 *
 * Returns the number of bytes successfully written,
 * 0 if socket is disconnected, or -1 on failure
 */
ssize_t write_message_size(int socket, size_t size);

/**
 * Attempts to read all count bytes from socket into buffer.
 * Assumes buffer is large enough.
 *
 * Returns the number of bytes read, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t read_all_from_socket(int socket, char *buffer, size_t count);

/**
 * Attempts to write all count bytes from buffer to socket.
 * Assumes buffer contains at least count bytes.
 *
 * Returns the number of bytes written, 0 if socket is disconnected,
 * or -1 on failure.
 */
ssize_t write_all_to_socket(int socket, const char *buffer, size_t count);
