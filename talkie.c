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
    UpTo(i, 3)
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

void cleanup()
{
    if (routerSocket != -1)
    {
        if (shutdown(routerSocket, SHUT_RDWR)) // @@ testing shutdown
            perror("shutdown(routerSocket)");
        close(routerSocket);
    }
    if (serverSocket != -1)
    {
        if (shutdown(serverSocket, SHUT_RDWR)) // @@ testing shutdown
            perror("shutdown(serverSocket)");
        close(serverSocket);
    }
    if (bigboySocket != -1)
    {
        if (shutdown(bigboySocket, SHUT_RDWR)) // @@ testing shutdown
            perror("shutdown(bigboySocket)");
        close(bigboySocket);
    }

    UpTo(i, MAX_TALKIES)
    {
        if (talkieSockets[i] != -1)
        {
            if (shutdown(talkieSockets[i], SHUT_RDWR))
                perror("shutdown(talkieSockets[i])");
            close(talkieSockets[i]);
        }
    }
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
        write_message_to_screen("[Talkie] Invalid name! Try again.\n");
        read_message_from_screen(&buf);
    }
    
    // setup myInfo
    strcpy((char *)myInfo.name, buf);
    free(buf);
    myInfo.presence = 1;
    write_message_to_screen("[Talkie] Welcome %s! Enjoy your p2p chat!\n\n\n", (char *)myInfo.name);
    char *ipstring = get_local_ip();
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

    // !! spawn a thread to keep an eye on router for disconnection

    // I'm the bigboy!
    if (amIBigboy) 
    {
        write_message_to_screen("[TEST] I'm bigboy.\n");

        // send myInfo to router as bigboy
        if (send_data(routerSocket, (char *)&myInfo, sizeof(talkie_info), 1) <= 0)
            return;

        // run as server and client
        pthread_create(&threads[0], NULL, run_mini_server, NULL);
        run_mini_client();
        pthread_join(threads[0], NULL);
        // !! send discon msg here to router
    }

    // I'm just a normal talkie!
    else
    {
        write_message_to_screen("[TEST] I'm not bigboy.\n");

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
        if (send_data(routerSocket, (char *)&myInfo, sizeof(talkie_info), 1) <= 0)
            return;

        // run mini client
        run_mini_client();

        // send discon msg here to bigboy
        send_data(bigboySocket, TALKIE_DISCONNECT_MESSAGE, strlen(TALKIE_DISCONNECT_MESSAGE) + 1, 3);
    }
}

void run_mini_client()
{
    // I am the bigboy!
    if (amIBigboy)
    {
        // block until server part is online
        while (serverSocket == -1)
        { /* do nothing */
        }
        bigboySocket = connect_to_server(LOCAL_HOST, SERVER_PORT);
    }

    // I am a normal talkie!
    else
    {
        bigboySocket = connect_to_server((char *)bigboyInfo.ip, SERVER_PORT);
    }

    pthread_create(&threads[1], NULL, write_to_bigboy, NULL);
    pthread_create(&threads[2], NULL, read_from_bigboy, NULL);

    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
}

void *run_mini_server(void *p)
{
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
    // !! correctly getaddrinfo
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

void *process_talkie(void *p)
{
    pthread_detach(pthread_self());

    intptr_t talkieSocketIndex = (intptr_t)p;
    // int talkieSocket = talkieSockets[talkieSocketIndex];
    int type;
    char *buffer = NULL;

    // read data from talkie
    while (!endSession)
    {
        if (fetch_data(talkieSockets[talkieSocketIndex], &buffer, &type) <= 0)
            goto END_PROCESS_TALKIE;

        // data should either be chat msg or disconnected msg

        // chat msg: broadcast to all
        if (!type)
        {
            pthread_mutex_lock(&mutex);
            broadcast_data((int *)&talkieSockets, buffer, strlen(buffer) + 1, 0);
            pthread_mutex_unlock(&mutex);
        }

        // disconnected msg
        else if (type == 3)
            if (!strcmp(buffer, TALKIE_DISCONNECT_MESSAGE))
                goto END_PROCESS_TALKIE;

        if (buffer)
        {
            free(buffer);
            buffer = NULL;
        }
    }

END_PROCESS_TALKIE:

    if (buffer)
    {
        free(buffer);
        buffer = NULL;
    }

    close(talkieSockets[talkieSocketIndex]);

    pthread_mutex_lock(&mutex);
    talkieSockets[talkieSocketIndex] = -1;
    talkieCount--;
    // !! broadcast talkie disconnected msg
    // broadcast_data((int *)&talkieSockets, TALKIE_DISCONNECT_MESSAGE, strlen(TALKIE_DISCONNECT_MESSAGE) + 1, 3);
    // broadcast the identifier of the disconnected talkie
    // int id = talkieInfo[talkieInfoIndex].routerIdentifier;
    // broadcast_data((int *)&talkieSockets, (char *)&id, sizeof(int),  3);
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int connect_to_server(const char *host, const char *port)
{

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

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
        retval = send_data(bigboySocket, msg, strlen(msg) + 1, 0);

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
            if (!type)
                write_message_to_screen("%s\n", buffer);

            // disconnecting msg 
            else if (type == 3)
                write_message_to_screen("%s\n", buffer); // !! better handling later
        }
        free(buffer);
        buffer = NULL;
    }

    pthread_cleanup_pop(0);
    return 0;
}

void close_talkie()
{
    endSession = 1;

    // cancel threads
    UpTo(i, 3)
        if (threads[i])
            pthread_cancel(threads[i]);

    close_chat();
}