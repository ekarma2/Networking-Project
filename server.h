#ifndef DAYTIME_SERVER_H
#define DAYTIME_SERVER_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
//daytime server libraries
#include <signal.h>
#include <string.h>
#include <time.h>
#include <semaphore.h>
#include <netdb.h>

/* Function prototypes */
void* handle_client(void* arg);

int connect_to_server(
    const char *server_address,
    const char *server_port
);

int receive_daytime_message(
    int socket_fd,
    char *message_buffer
);

/* Preprocessor directives */
#define SERVER_ADDR "127.0.0.1" // loopback ip address, we dont actually use it anymore
#define PORT 23657              // port the server will listen on
#define FALSE 0
#define TRUE !FALSE
#define NUM_CONNECTIONS 5       // number of pending connections in the connection queue
#define NIST_SERVER_ADDRESS "time.nist.gov"
#define NIST_SERVER_PORT "13"
#define MAX_MESSAGE_LENGTH 80
#define ON_TIME_MARKER '*'

#endif