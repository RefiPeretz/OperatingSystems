/**
* header the class for the client
*/

#ifndef _CLFTP_H
#define _CLFTP_H
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <fstream>
#include <sys/stat.h>
#include <errno.h>


using namespace std;

#define BUFF_SIZE 1024
#define SUCCESS 0
#define MIN_SERVER_PORT 1
#define MAX_SERVER_PORT 65535
#define FAIL -1
#define ANSWER_SIZE 2
#define POSITIVE '1'
#define NEGETIVE '0'
/**
* class client.
*/
class Client
{
public: 
	/**
	* constractor
	*/
	Client();
	/**
	* destractor
	*/
	~Client();
	int _serverPort;
	int _socket;
	int _fileNameSize;
	int _fileSize;
	char* _fileToTransfer = NULL;
	char* _fileNameToSave = NULL;
	/**
	* help function to send the file data
	*/
	int sendData(ifstream &temp, int size , int socket);
	/**
	* function to send the file data
	*/
	int sendBaffer(char* buff, int buffSize, int socket);
	/**
	* function to get the file size
	*/
	int getSize(ifstream &temp);
private:

};

#endif