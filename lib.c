/**
 * Shadow_Talkie
 * 
 * @ Illinois, 2018
 */

#include "includes/lib.h"

ssize_t broadcast_data(const int *socketsArray, const char *buffer, size_t size, int type)
{
    size_t counter = 0;
    int clientSocket;

    // Assume mutex for array is already locked
    UpTo(i, MAX_TALKIES)
    {
        if ((clientSocket = socketsArray[i]) != -1)
        {
            if (send_data(clientSocket, buffer, size, type) <= 0)
                return -1;
            counter++;
        }
    }
    // Remember to release mutex after return
    return counter;
}

ssize_t fetch_data(int socket, char **buffer, int *type)
{
    ssize_t status = 0;

    // read size
    ssize_t size = get_message_size(socket);

    // read data type
    if (size > 0)
        if ((status = get_message_size(socket)) > 0)
            *type = status - 1;

    // read data
    if (status > 0)
    {
        *buffer = calloc(1, size);
        status = read_all_from_socket(socket, *buffer, size);
    }

    return status;
}

ssize_t send_data(int socket, const char *buffer, size_t size, int type)
{
    // write size
    ssize_t status = write_message_size(socket, size);

    // write data type
    if (status > 0)
        status = write_message_size(socket, type + 1);

    // write data
    if (status > 0)
        status = write_all_to_socket(socket, buffer, size);

    return status;
}

char *get_local_ipv4_ip()
{
    struct ifaddrs *iflist, *interface;

    // get a linked list of ifaddrs
    if (getifaddrs(&iflist) < 0)
    {
        perror("getifaddrs()");
        return NULL;
    }

    // loop through the list and look for target
    for (interface = iflist; interface; interface = interface->ifa_next)
    {
        // ## filter for default gateway; dummy implementation using hardcoded strcmp on ifa_name
        // ## including some common aliases for interface of default gateway:
        if (strcmp(interface->ifa_name, "en0") && 
            strcmp(interface->ifa_name, "eth0") && 
            strcmp(interface->ifa_name, "wlp1s0") && 
            strcmp(interface->ifa_name, "wlp2s0") && 
            strcmp(interface->ifa_name, "wlp3s0"))
            continue;
            
        int af = interface->ifa_addr->sa_family;
        const void *addr;
        char buf[INET_ADDRSTRLEN];

        // filter for IPv4 addr
        switch (af)
        {
        case AF_INET:
            addr = &((struct sockaddr_in *)interface->ifa_addr)->sin_addr;
            break;
        default:
            addr = NULL;
        }

        // parse addr into string if found the valid default IPv4 IP
        if (addr)
        {
            if (!inet_ntop(af, addr, buf, sizeof(buf)))
            {
                perror("inet_ntop()");
                continue;
            }
            freeifaddrs(iflist);
            return strdup(buf);
        }
    }

    // return NULL if no specified addr was found
    freeifaddrs(iflist);
    return NULL;
}

char *create_message(char *name, char *message)
{
    int name_len = strlen(name);
    int msg_len = strlen(message);
    char *msg = calloc(1, msg_len + name_len + 4);
    sprintf(msg, "%s: %s", name, message);
    return msg;
}

ssize_t get_message_size(int socket)
{
    int32_t size;
    ssize_t read_bytes =
        read_all_from_socket(socket, (char *)&size, MESSAGE_SIZE_DIGITS);
    if (read_bytes == 0 || read_bytes == -1)
        return read_bytes;

    return (ssize_t)size;
}

ssize_t write_message_size(int socket, size_t size)
{
    uint32_t ssize = (uint32_t)size;
    return write_all_to_socket(socket, (char *)&ssize, MESSAGE_SIZE_DIGITS);
}

ssize_t read_all_from_socket(int socket, char *buffer, size_t count)
{
    size_t counter = 0;

    while (counter < count)
    {
        ssize_t reading = read(socket, buffer + counter, count - counter);

        if (!reading)
            return counter;

        else if (reading == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }

        counter += reading;
    }

    return counter;
}

ssize_t write_all_to_socket(int socket, const char *buffer, size_t count)
{
    size_t counter = 0;

    while (counter < count)
    {
        ssize_t writing = write(socket, buffer + counter, count - counter);

        if (!writing)
            return counter;

        else if (writing == -1)
        {
            if (errno == EINTR)
                continue;
            else
                return -1;
        }

        counter += writing;
    }

    return counter;
}

