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
    int client_socket = *((int*)arg);

    /*
     * The socket descriptor has now been copied into this
     * thread's local variable, so main may reuse its variable.
     */
    if (sem_post(&socket_copied) == -1)
    {
        perror("Error posting semaphore");
        close(client_socket);
        return NULL;
    }

    time_t current_time;
    struct tm utc_time;
    char time_message[80];

    // get current time
    current_time = time(NULL);

    // error handling for no time recieved
    if (current_time == (time_t)-1)
    {
        perror("Error getting current time");
        close(client_socket);
        return NULL;
    }

    // convert to UTC in a thread-safe way
    // gmtime_r instead of gmtime, gmtime is not thread-safe
    if (gmtime_r(&current_time, &utc_time) == NULL)
    {
        perror("Error converting time");
        close(client_socket);
        return NULL;
    }

    // make the time message, ends in *
    strftime(
        time_message,
        sizeof(time_message),
        "%Y-%m-%d %H:%M:%S UTC *",
        &utc_time
    );

    // send the time message to the client
    if (write(client_socket, time_message, strlen(time_message)) == -1)
    {
        perror("Error writing to client");
    }

    printf("Sent: %s\n", time_message);

    // close connection after sending the time
    if (close(client_socket) == -1)
    {
        perror("Error closing client socket");
    }

    return NULL;
}