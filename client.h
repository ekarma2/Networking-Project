#ifndef DAYTIME_CLIENT_H
#define DAYTIME_CLIENT_H

// Server information for the NIST Daytime service
//#define SERVER_ADDRESS   "time-a.nist.gov"
//#define SERVER_PORT      "13"


//LOCAL MACHINE TESTING INFORMATION
#define SERVER_ADDRESS "127.0.0.1"
#define SERVER_PORT "23657"

// MAX_MESSAGE_LENGTH is 80
#define MAX_MESSAGE_LENGTH 80
//Final char is * in message
#define ON_TIME_MARKER     '*'

#endif // DAYTIME_CLIENT_H 