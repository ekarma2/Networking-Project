#include "server.h"

sem_t socket_copied;


/* ************************************************************************* */
/* MAIN                                                                      */
/* ************************************************************************* */

int main(int argc, char** argv)
{
    int server_socket;                 // descriptor of server socket
    struct sockaddr_in server_address; // for naming the server's listening socket
    int yes = 1;


    // ----------------------------------------------------------
    // ignore SIGPIPE, sent when client disconnected
    // ----------------------------------------------------------
    signal(SIGPIPE, SIG_IGN);
    
    // ----------------------------------------------------------
    // create unnamed network socket for server to listen on
    // ----------------------------------------------------------
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("Error creating socket");
        exit(EXIT_FAILURE);
    }
    
    // lose the pesky "Address already in use" error message
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // ----------------------------------------------------------
    // bind the socket
    // ----------------------------------------------------------
    server_address.sin_family      = AF_INET;           // accept IP addresses
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); // accept clients on any interface
    server_address.sin_port        = htons(PORT);       // port to listen on
    
    // binding unnamed socket to a particular port
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) != 0) 
    {
        perror("Error binding socket");
        exit(EXIT_FAILURE);
    }
    
    // ----------------------------------------------------------
    // listen on the socket
    // ----------------------------------------------------------
    if (listen(server_socket, NUM_CONNECTIONS) != 0)
    {
        perror("Error listening on socket");
        exit(EXIT_FAILURE);
    }
    
    // ----------------------------------------------------------
    // server loop
    // ----------------------------------------------------------

    //init semaphore to 0
    if (sem_init(&socket_copied, 0, 0) == -1)
    {
        perror("Error initializing semaphore");
        exit(EXIT_FAILURE);
    }

    while (TRUE)
    {
        // Accept a new client connection 
        int client_socket = accept(server_socket, NULL, NULL);

        if (client_socket == -1)
        {
            perror("Error accepting connection");
            continue;
        }

        printf("\nServer with PID %d: accepted client\n", getpid());

        pthread_t thread;

        if (pthread_create(
                &thread,
                NULL,
                handle_client,
                &client_socket) != 0)
        {
            perror("Error creating thread");
            close(client_socket);
            continue;
        }

        /*
        * Wait until the new thread has copied client_socket
        * before allowing the accept loop to reuse the variable.
        */
        if (sem_wait(&socket_copied) == -1)
        {
            perror("Error waiting on semaphore");
            exit(EXIT_FAILURE);
        }

        if (pthread_detach(thread) != 0)
        {
            perror("Error detaching thread");
            exit(EXIT_FAILURE);
        }
    }
}


/* ************************************************************************* */
/* handle client                                                             */
/* ************************************************************************* */

void* handle_client(void* arg)
{
    //for the proxy just ask the NIST instead of storing the time here
    //AKA no time functions

    int client_socket = *((int*)arg);

    /*
     * socket descriptor is copied into the thread's local variable
     * so main may reuse its variable.
     */
    if (sem_post(&socket_copied) == -1)
    {
        perror("error posting semaphore");
        close(client_socket);
        return NULL;
    }

    char time_message[MAX_MESSAGE_LENGTH];
    int nist_socket;
    int message_length;

    printf("[proxy] client connected\n");
    printf("[proxy] connecting to %s:%s\n",
           NIST_SERVER_ADDRESS,
           NIST_SERVER_PORT);

    //the proxy now acts as a client to NIST.
    nist_socket = connect_to_server(
        NIST_SERVER_ADDRESS,
        NIST_SERVER_PORT
    );

    if (nist_socket == -1)
    {
        fprintf(stderr, "[proxy] could not connect to NIST\n");
        close(client_socket);
        return NULL;
    }

    printf("[proxy] connected to NIST\n");
    printf("[proxy] waiting for Daytime response\n");

    message_length =
        receive_daytime_message(nist_socket, time_message);

    close(nist_socket);

    if (message_length <= 0)
    {
        fprintf(stderr,
                "[proxy] no Daytime message received from NIST\n");

        close(client_socket);
        return NULL;
    }

    printf(
        "[proxy] received from NIST: %.*s\n",
        message_length,
        time_message
    );

    printf("[proxy] forwarding response to client\n");

    if (write(client_socket,
              time_message,
              message_length) == -1)
    {
        perror("[proxy] error forwarding message");
    }
    else
    {
        printf("[proxy] response forwarded successfully\n");
    }

    close(client_socket);

    printf("[proxy] client connection closed\n");

    return NULL;
}

//fuction to connect to nist
int connect_to_server(const char *server_address, const char *server_port)
{
    struct addrinfo hints;
    struct addrinfo *server_info;
    struct addrinfo *current_addr;

    int socket_fd;
    int lookup_status;

    //init hints to 0
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    lookup_status = getaddrinfo(
        server_address,
        server_port,
        &hints,
        &server_info
    );

    if (lookup_status != 0)
    {
        fprintf(stderr,
                "getaddrinfo error: %s\n",
                gai_strerror(lookup_status));

        return -1;
    }

    socket_fd = -1;

    //loop through all the results and connect to the first we can
    for (current_addr = server_info;
         current_addr != NULL;
         current_addr = current_addr->ai_next)
    {
        socket_fd = socket(
            current_addr->ai_family,
            current_addr->ai_socktype,
            current_addr->ai_protocol
        );

        if (socket_fd == -1)
        {
            continue;
        }

        if (connect(
                socket_fd,
                current_addr->ai_addr,
                current_addr->ai_addrlen) == 0)
        {
            break;
        }

        close(socket_fd);
        socket_fd = -1;
    }

    freeaddrinfo(server_info);

    return socket_fd;
}

//function to get the daytime message from NIST
int receive_daytime_message(int socket_fd, char *message_buffer)
{
    int total_bytes_received = 0;
    ssize_t bytes_read;

    while (total_bytes_received < MAX_MESSAGE_LENGTH)
    {
        bytes_read = read(
            socket_fd,
            message_buffer + total_bytes_received,
            1
        );

        if (bytes_read < 0)
        {
            perror("Error reading from NIST");
            return -1;
        }

        if (bytes_read == 0)
        {
            break;
        }

        total_bytes_received++;

        if (message_buffer[total_bytes_received - 1]
                == ON_TIME_MARKER)
        {
            break;
        }
    }

    return total_bytes_received;
}