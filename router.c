/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "includes/router.h"

int main(int argc, char **argv)
{
    if (argc != 1)
    {
        print_router_usage();
        exit(-1);
    }

    // handle SIGINT as quit
    struct sigaction act;
    memset(&act, '\0', sizeof(act));
    act.sa_handler = close_router;
    if (sigaction(SIGINT, &act, NULL) < 0)
    {
        perror("sigaction");
        exit(1);
    }

    // start the router!
    run_router();
    cleanup();
    pthread_exit(NULL);
    return 0;
}

void close_router()
{
    endSession = 1;
}

void cleanup()
{
    // send disconnect message to bigboy
    pthread_mutex_lock(&mutex);
    broadcast_data((int *)talkieSockets, ROUTER_DISCONNECT_MESSAGE, strlen(ROUTER_DISCONNECT_MESSAGE) + 1, TYPE_OTHER_MSG);
    pthread_mutex_unlock(&mutex);

    // close router socket
    close(routerSocket);

    UpTo(i, MAX_TALKIES)
    {
        // shutdown and close talkie sockets
        if (talkieSockets[i] != -1)
        {
            if (shutdown(talkieSockets[i], SHUT_RDWR))
                perror("shutdown()");
            close(talkieSockets[i]);
        }
    }

    print_router_goodbye();
}

void run_router()
{
    // init socket with IPv4 & TCP
    routerSocket = socket(AF_INET, SOCK_STREAM, 0);
    
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
    int getaddrinfo_status = getaddrinfo(NULL, ROUTER_PORT, &hints, &result);
    if (getaddrinfo_status)
    {
        fprintf(stderr, "getaddrinfo(): %s\n", gai_strerror(getaddrinfo_status));
        exit(1);
    }

    // bind socket with addr
    if (bind(routerSocket, result->ai_addr, result->ai_addrlen))
    {
        perror("bind()");
        exit(1);
    }

    // listen for talkie connection: backlog set to MAX_TALKIES
    if (listen(routerSocket, MAX_TALKIES))
    {
        perror("listen()");
        exit(1);
    }

    // init talkieSockets
    UpTo(i, MAX_TALKIES)
        talkieSockets[i] = -1;

    // print welcome msg  
    print_router_greeting();

    // start accepting talkies
    while (!endSession)
    {
        struct sockaddr_in talkieAddr;
        memset(&talkieAddr, 0, sizeof(struct sockaddr_in));
        socklen_t addrlen = sizeof(struct sockaddr_in);
        int talkieSocket = accept(routerSocket, (struct sockaddr *)&talkieAddr, &addrlen);

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
        int64_t talkieIndex = 0;
        for (; talkieIndex < MAX_TALKIES; talkieIndex++)
            if (talkieSockets[talkieIndex] == -1)
                break;

        // welcome the talkie!
        talkieSockets[talkieIndex] = talkieSocket;
        talkieCount++;

        pthread_mutex_unlock(&mutex);

        // spawn a thread to process the new talkie
        pthread_t pid;
        pthread_create(&pid, NULL, process_talkie, (void *)talkieIndex);
    }
    freeaddrinfo(result);
    return;
}

void *process_talkie(void *p)
{
    pthread_detach(pthread_self());

    // parse the pointer into index
    int talkieIndex = (int)p;
    int talkieSocket = talkieSockets[talkieIndex];

    // set up tools
    char *buffer = NULL;
    talkie_info currInfo;
    int type_dump;

    pthread_mutex_lock(&mutex);

    // this is the bigboy if no active talkie
    if (youAreBigboy)
    {
        // send youAreBigboy = 1 to talkie
        if (send_data(talkieSocket, (char *)&youAreBigboy, sizeof(int), TYPE_OTHER_MSG) <= 0)
            goto END_PROCESS_TALKIE;

        // fetch nodeinfo from him
        if (fetch_data(talkieSocket, &buffer, &type_dump) <= 0)
        {
            pthread_mutex_unlock(&mutex);
            goto END_PROCESS_TALKIE;
        }
        memcpy((void *)&bigboyInfo, buffer, sizeof(talkie_info));
        memcpy((void *)&currInfo, buffer, sizeof(talkie_info));
        free(buffer);
        buffer = NULL;

        // other threads should not be bigboy
        youAreBigboy = 0;

        print_talkie_join(currInfo.name, currInfo.ip, 1);

        pthread_mutex_unlock(&mutex);
    }

    // else bigboy already exists
    else
    {
        pthread_mutex_unlock(&mutex);

        // send youAreBigboy = 0 to talkie
        if (send_data(talkieSocket, (char *)&youAreBigboy, sizeof(int), TYPE_OTHER_MSG) <= 0)
            goto END_PROCESS_TALKIE;

        // send nodeinfo of bigboy to talkie
        if (send_data(talkieSocket, (char *)&bigboyInfo, sizeof(talkie_info), TYPE_INFO) <= 0)
            goto END_PROCESS_TALKIE;

        // fetch nodeinfo from him
        if (fetch_data(talkieSocket, &buffer, &type_dump) <= 0)
            goto END_PROCESS_TALKIE;
        memcpy((void *)&currInfo, buffer, sizeof(talkie_info));
        free(buffer);
        buffer = NULL;

        print_talkie_join(currInfo.name, currInfo.ip, 0);

    }

    // keep listening for disconnection
    while (fetch_data(talkieSocket, &buffer, &type_dump) > 0)
    {
        // make sure msg is the right one
        if (!strcmp(buffer, TALKIE_DISCONNECT_MESSAGE))
            goto END_PROCESS_TALKIE;

        free(buffer);
        buffer = NULL;
    }

END_PROCESS_TALKIE:

    if (buffer)
    {
        free(buffer);
        buffer = NULL;
    }

    // talkie disconnected
    close(talkieSockets[talkieIndex]);

    pthread_mutex_lock(&mutex);
    talkieSockets[talkieIndex] = -1;
    talkieCount--;
    if (!talkieCount)
        youAreBigboy = 1;
    pthread_mutex_unlock(&mutex);

    print_talkie_left(currInfo.name);

    return NULL;
}