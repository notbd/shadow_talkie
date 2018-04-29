/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "includes/talkie.h"

int main(int argc, char **argv)
{
    if (argc != 1)
    {
        print_talkie_usage();
        exit(-1);
    }

    // init thread, talkieSockets and myInfo containers
    UpTo(i, 4)
        threads[i] = 0;
    UpTo(i, MAX_TALKIES)
        talkieSockets[i] = -1;
    memset((void *)&myInfo, 0, sizeof(talkie_info));

    // creat the chat window and destroy upon exit
    create_windows(NULL);
    atexit(destroy_windows);

    // handle SIGINT as quit
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_talkie;
    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        perror("sigaction()");
        exit(1);
    }

    // start the talkie!
    run_talkie();
    cleanup();
    pthread_exit(NULL);
}

void close_talkie()
{
    endSession = 1;

    // cancel threads
    UpTo(i, 4) if (threads[i])
        pthread_cancel(threads[i]);
}

void cleanup()
{
    // shutdown and close routerSocket
    if (routerSocket != -1)
    {
        if (shutdown(routerSocket, SHUT_RDWR))
            perror("shutdown(routerSocket)");
        close(routerSocket);
    }

    // close serverSocket / doesn't need to shutdown since passive
    if (serverSocket != -1)
        close(serverSocket);

    // shutdown and close bigboySocket
    if (bigboySocket != -1)
    {
        if (shutdown(bigboySocket, SHUT_RDWR))
            perror("shutdown(bigboySocket)");
        close(bigboySocket);
    }

    // shutdown and close talkieSockets
    UpTo(i, MAX_TALKIES)
    {
        if (talkieSockets[i] != -1)
        {
            if (shutdown(talkieSockets[i], SHUT_RDWR))
                perror("shutdown(talkieSockets[i])");
            close(talkieSockets[i]);
        }
    }

    // close chat window
    close_chat();
}

void run_talkie()
{
    print_talkie_greeting_to_window();

    // get username from stdin and store in myInfo 
    char *buf = calloc(1, 256);
    read_message_from_screen(&buf);
    while (strlen(buf) >= USERNAME_LENGTH || !strlen(buf))
    {
        // invalid username: too long or empty
        write_message_to_screen("[TALKIE] Invalid name! Try again.\n");
        read_message_from_screen(&buf);
    }

    if (endSession)
    {
        free(buf);
        return;
    }

    // setup myInfo
    strcpy((char *)myInfo.name, buf);
    free(buf);
    write_message_to_screen("[TALKIE] Welcome, %s! Enjoy your p2p chat!\n\n", (char *)myInfo.name);
    write_message_to_screen("[TALKIE] Chat Window:\n");
    write_message_to_screen("------------------------\n");
    char *ipstring = get_local_ipv4_ip();
    strcpy((char *)myInfo.ip, ipstring);
    free(ipstring);

    // connect to router
    routerSocket = connect_to_server(ROUTER_HOST, ROUTER_PORT);

    // set up tools
    int type_dump;
    char *buffer = NULL;

    // get amIBigboy
    fetch_data(routerSocket, &buffer, &type_dump);
    amIBigboy = *((int *)buffer);
    free(buffer);
    buffer = NULL;

    // I'm the bigboy!
    if (amIBigboy) 
    {
        // send myInfo to router as bigboy
        if (send_data(routerSocket, (char *)&myInfo, sizeof(talkie_info), TYPE_INFO) <= 0)
            return;

        // run as server and client
        pthread_create(&threads[0], NULL, run_mini_server, NULL);
        run_mini_client();
    }

    // I'm just a normal talkie!
    else
    {
        // fetch bigboy info from router
        if (fetch_data(routerSocket, &buffer, &type_dump) <= 0)
        {
            if (buffer)
            {
                free(buffer);
                buffer = NULL;
            }
            return;
        }
        memcpy((void *)&bigboyInfo, buffer, sizeof(talkie_info));
        free(buffer);
        buffer = NULL;

        // send myInfo to router
        if (send_data(routerSocket, (char *)&myInfo, sizeof(talkie_info), TYPE_INFO) <= 0)
            return;

        // run mini client
        run_mini_client();
    }
}

void *run_mini_server(void *p)
{
    pthread_detach(pthread_self());

    // init socket with IPv4 & TCP
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    // set socket options: reuse addr and port
    int optval = 1;
    if (setsockopt(routerSocket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        perror("setsockopt(SO_REUSEADDR)");
    if (setsockopt(routerSocket, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) < 0)
        perror("setsockopt(SO_REUSEPORT)");

    // init addrinfo
    struct addrinfo hints, *result = NULL;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    hints.ai_flags = AI_PASSIVE;
    int getaddrinfo_status = getaddrinfo(NULL, SERVER_PORT, &hints, &result);
    if (getaddrinfo_status)
    {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(getaddrinfo_status));
        exit(1);
    }

    // bind socket with addr
    if (bind(serverSocket, result->ai_addr, result->ai_addrlen))
    {
        perror("bind()");
        exit(1);
    }

    // result not used anymore; free it
    freeaddrinfo(result);

    // listen for talkie connection: backlog set to MAX_TALKIES
    if (listen(serverSocket, MAX_TALKIES))
    {
        perror("listen()");
        exit(1);
    }

    // spawn a thread to listen from the router
    pthread_create(&threads[1], NULL, listen_from_router, NULL);

    // start accepting talkies
    while (!endSession)
    {
        struct sockaddr_in talkieaddr;
        memset(&talkieaddr, 0, sizeof(struct sockaddr_in));
        socklen_t addrlen = sizeof(struct sockaddr_in);
        int talkieSocket = accept(serverSocket, (struct sockaddr *)&talkieaddr, &addrlen);

        if (endSession)
            break;

        pthread_mutex_lock(&mutex);

        // decline connection if crowded
        if (talkieCount == MAX_TALKIES)
        {
            close(talkieSocket);
            pthread_mutex_unlock(&mutex);
            continue;
        }

        // find first available slot
        int64_t talkieSocketIndex = 0;
        for (; talkieSocketIndex < MAX_TALKIES; talkieSocketIndex++)
            if (talkieSockets[talkieSocketIndex] == -1)
                break;

        // welcome the talkie!
        talkieSockets[talkieSocketIndex] = talkieSocket;
        talkieCount++;
        pthread_mutex_unlock(&mutex);

        // spawn a thread to process the new talkie
        pthread_t pid;
        pthread_create(&pid, NULL, process_talkie, (void *)talkieSocketIndex);
    }
    
    return NULL;
}

void run_mini_client()
{
    // I am the bigboy!
    if (amIBigboy)
    {
        // !! (naive) make sure server part is online
        sleep(1);

        // connect to server thread
        bigboySocket = connect_to_server(LOCAL_HOST, SERVER_PORT);
    }

    // I am a normal talkie!
    else
    {
        // connect to bigboy
        bigboySocket = connect_to_server((char *)bigboyInfo.ip, SERVER_PORT);

        // send myInfo to bigboy
        if (send_data(bigboySocket, (char *)&myInfo, sizeof(talkie_info), TYPE_INFO) <= 0)
            return;
    }

    // create threads to handle data
    pthread_create(&threads[2], NULL, write_to_bigboy, NULL);
    pthread_create(&threads[3], NULL, read_from_bigboy, NULL);

    pthread_join(threads[2], NULL);
    pthread_join(threads[3], NULL);
}

void *process_talkie(void *p)
{
    pthread_detach(pthread_self());

    // set up resources and tools
    intptr_t talkieSocketIndex = (intptr_t)p;
    int talkieSocket = talkieSockets[talkieSocketIndex];
    talkie_info currInfo;
    int type;
    char *buffer = NULL;

    // read data from talkie
    while (!endSession)
    {
        if (fetch_data(talkieSocket, &buffer, &type) <= 0)
            goto END_PROCESS_TALKIE;

        // chat msg: broadcast to all
        if (type == TYPE_CHAT_MSG)
        {
            pthread_mutex_lock(&mutex);
            broadcast_data((int *)&talkieSockets, buffer, strlen(buffer) + 1, TYPE_CHAT_MSG);
            pthread_mutex_unlock(&mutex);
        }

        // info from non-bigboy talkie
        if (type == TYPE_INFO)
        {
            // save the info
            memcpy((void *)&currInfo, buffer, sizeof(talkie_info));

            // broadcast joining msg to everyone
            char *msg = calloc(1, 256);
            sprintf(msg, "[TALKIE] %s joined the chat!", currInfo.name);
            pthread_mutex_lock(&mutex);
            broadcast_data((int *)&talkieSockets, msg, strlen(msg) + 1, TYPE_OTHER_MSG);
            pthread_mutex_unlock(&mutex);
            free(msg);
        }  

        // disconnected msg
        if (type == TYPE_OTHER_MSG)
            if (!strcmp(buffer, TALKIE_DISCONNECT_MESSAGE))
                goto END_PROCESS_TALKIE;

        if (buffer)
        {
            free(buffer);
            buffer = NULL;
        }
    }

END_PROCESS_TALKIE:

    // clean up

    if (buffer)
    {
        free(buffer);
        buffer = NULL;
    }

    close(talkieSockets[talkieSocketIndex]);

    pthread_mutex_lock(&mutex);
    talkieSockets[talkieSocketIndex] = -1;
    talkieCount--;
    // broadcast talkie disconnected msg
    char *msg = calloc(1, 256);
    sprintf(msg, "[TALKIE] %s has left.", currInfo.name);
    broadcast_data((int *)&talkieSockets, msg, strlen(msg) + 1, TYPE_OTHER_MSG);
    free(msg);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void *listen_from_router(void *p)
{
    pthread_detach(pthread_self());

    // set up tools
    char *buffer = NULL;
    int type;

    // keep listening for disconnection
    while (fetch_data(routerSocket, &buffer, &type) > 0 && !endSession)
    {
        // make sure msg is the right one
        if (!strcmp(buffer, ROUTER_DISCONNECT_MESSAGE))
            goto END_LISTEN_FROM_ROUTER;

        free(buffer);
        buffer = NULL;
    }

END_LISTEN_FROM_ROUTER:

    if (buffer)
    {
        free(buffer);
        buffer = NULL;
    }

    if (!endSession)
    {
        // broadcast router disconnect msg to talkies
        pthread_mutex_lock(&mutex);
        broadcast_data((int *)&talkieSockets, ROUTER_DISCONNECT_NOTI, strlen(ROUTER_DISCONNECT_NOTI) + 1, TYPE_OTHER_MSG);
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

int connect_to_server(const char *host, const char *port)
{
    // init a new socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    // init addrinfo
    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;       /* IPv4 only */
    hints.ai_socktype = SOCK_STREAM; /* TCP */
    int getaddrinfo_status = getaddrinfo(host, port, &hints, &result);
    if (getaddrinfo_status)
    {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(getaddrinfo_status));
        exit(1);
    }

    // connect!
    if (connect(sock_fd, result->ai_addr, result->ai_addrlen) == -1)
    {
        perror("connect()");
        exit(1);
    }

    freeaddrinfo(result);
    return sock_fd;
}

void thread_cancellation_handler(void *arg)
{
    // make sure resources are clean when the thread got cancelled
    printf("Cancellation handler\n");
    thread_cancel_args *a = (thread_cancel_args *)arg;
    char **msg = a->msg;
    char **buffer = a->buffer;
    if (*buffer)
    {
        free(*buffer);
        *buffer = NULL;
    }
    if (msg && *msg)
    {
        free(*msg);
        *msg = NULL;
    }
}

void *write_to_bigboy(void *arg)
{
    // Silence the unused parameter warning.
    (void)arg;

    // set up tools
    char *buffer = NULL;
    char *msg = NULL;
    ssize_t retval = 1;

    // setup thread cancellation handlers
    thread_cancel_args cancel_args;
    cancel_args.buffer = &buffer;
    cancel_args.msg = &msg;
    pthread_cleanup_push(thread_cancellation_handler, &cancel_args);

    while (retval > 0 && !endSession)
    {
        // read user input
        read_message_from_screen(&buffer);
        if (buffer == NULL)
            break;

        // send the msg to bigboy as chat msg
        msg = create_message((char *)myInfo.name, buffer);
        retval = send_data(bigboySocket, msg, strlen(msg) + 1, TYPE_CHAT_MSG);

        // clean up buffer and msg
        free(buffer);
        buffer = NULL;
        free(msg);
        msg = NULL;
    }

    pthread_cleanup_pop(0);
    return 0;
}

void *read_from_bigboy(void *arg)
{
    // Silence the unused parameter warning.
    (void)arg;

    // set up tools
    char *buffer = NULL;
    int type;
    ssize_t retval = 1;

    // Setup thread cancellation handlers
    thread_cancel_args cancellation_args;
    cancellation_args.buffer = &buffer;
    cancellation_args.msg = NULL;
    pthread_cleanup_push(thread_cancellation_handler, &cancellation_args);

    // read data from bigboy
    while (retval > 0 && !endSession)
    {
        if ((retval = fetch_data(bigboySocket, &buffer, &type)) > 0)
        {
            // chat message
            if (type == TYPE_CHAT_MSG)
                write_message_to_screen("%s\n", buffer);

            // disconnecting msg 
            else if (type == TYPE_OTHER_MSG)
                write_message_to_screen("%s\n", buffer); // !! better handling later
        }
        free(buffer);
        buffer = NULL;
    }

    pthread_cleanup_pop(0);
    return 0;
}

