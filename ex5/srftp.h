/**
* the header the class for the client
*/

#ifndef _SRFTP_H
#define _SRFTP_H
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <limits.h>
#include <pthread.h>
#include <errno.h>

#define SUCCESS 0
#define FAIL -1
#define EXIT_FAIL 1
#define MAX_PORT_NUM 65535
#define MIN_PORT_NUM 1
#define LIMIT_CONNECTIONS 5
#define MAX_HOSTNAME 128
#define BUFF_SIZE 1024
#define USEAGE_ERROR "Usage: srftp server-port max-file-size"
#define ANSWER_SIZE 2
#define POSITIVE '1'
#define NEGETIVE '0'


using namespace std;


/**
* class client.
*/
class Server
{
public: 
	/**
	* constructor
	*/
	Server(int port, int maxSize);
	/**
	* distructor
	*/
	~Server();
	/**
	* a function that check if the parameters are valide.
	*/
	int validateParameters(int argc, char const *argv[]);
	/**
	* a function that load the server for the first time when starting the program
	*/
	int uploadServer();
	int _srvSocket;
	int _srvPort;
	int _maxFileSize;
	char srvHostName[MAX_HOSTNAME + 1];
	struct hostent *srvHost;
	struct sockaddr_in srvSocketAddress;

};

#endif