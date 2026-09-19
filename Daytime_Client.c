#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>

#include "daytime_client.h"

/*Resolves server_address/server_port and opens a TCP connection to it. 
 */
int connect_to_server(const char *server_address, const char *server_port)
{
    struct addrinfo hints;
    struct addrinfo *server_info;
    struct addrinfo *current_addr;
    int socket_fd;
    int lookup_status;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;    /* accept IPv4 or IPv6, whichever resolves */
    hints.ai_socktype = SOCK_STREAM;  /* Daytime over TCP */

    lookup_status = getaddrinfo(server_address, server_port, &hints, &server_info);
    if (lookup_status != 0) 
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(lookup_status));
        return -1;
    }

    socket_fd = -1;
    for (current_addr = server_info; current_addr != NULL; current_addr = current_addr->ai_next) 
    {
        socket_fd = socket(current_addr->ai_family,
                            current_addr->ai_socktype,
                            current_addr->ai_protocol);
        if (socket_fd == -1) 
        {
            continue;
        }

        if (connect(socket_fd, current_addr->ai_addr, current_addr->ai_addrlen) == 0) 
        {
            break; /* connected successfully, stop trying candidates */
        }

        close(socket_fd);
        socket_fd = -1;
    }

    freeaddrinfo(server_info); 

    return socket_fd;
}


 /* Reads the time message from socket_fd one byte at a time, stop
 when the byte just received is the on-time marker (OTM). */
int receive_daytime_message(int socket_fd, char *message_buffer)
{
    int total_bytes_received;
    ssize_t bytes_read;

    total_bytes_received = 0;

    while (total_bytes_received < MAX_MESSAGE_LENGTH) {
        bytes_read = read(socket_fd, message_buffer + total_bytes_received, 1);

        if (bytes_read < 0) {
            perror("read");
            return -1;
        }

        if (bytes_read == 0) {
            /* server closed the connection before we saw an OTM */
            break;
        }

        total_bytes_received += (int) bytes_read;

        /* stop as soon as the most recently received byte is the OTM */
        if (message_buffer[total_bytes_received - 1] == ON_TIME_MARKER) {
            break;
        }
    }

    return total_bytes_received;
}

int main(void)
{
    int socket_fd;
    int message_length;
    char message_buffer[MAX_MESSAGE_LENGTH];

    socket_fd = connect_to_server(SERVER_ADDRESS, SERVER_PORT);
    if (socket_fd == -1) {
        fprintf(stderr, "Could not connect to %s:%s\n", SERVER_ADDRESS, SERVER_PORT);
        return EXIT_FAILURE;
    }

    message_length = receive_daytime_message(socket_fd, message_buffer);
    close(socket_fd);

    if (message_length <= 0) {
        fprintf(stderr, "No data received from server.\n");
        return EXIT_FAILURE;
    }

    /* print exactly the bytes received */
    printf("%.*s\n", message_length, message_buffer);

    return EXIT_SUCCESS;
}