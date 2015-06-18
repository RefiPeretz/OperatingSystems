/**
* implementation of the client class
*/

#include "clftp.h"


#ifndef SYSCALL_ERROR
#define SYSCALL_ERROR(syscall) "Error: function: " << syscall << "errno: " << errno << "\n" 
#endif

// structures for handling internet addresses , build-in
struct sockaddr_in serv_addr; 
// stracter for the host. build-in
struct hostent *server = NULL;

/**
* client constractor
*/
Client::Client()
{

}
/**
* client distractor
*/
Client::~Client()
{
	if(_fileToTransfer != NULL)
	{
		free(_fileToTransfer);
	}

	if(_fileNameToSave != NULL)
	{
		free(_fileNameToSave);
	}


}
/**
* a function to get the file size
*/
int Client::getSize(ifstream &temp)
{
	// Returns the position of the current character in the input stream.
	long start = temp.tellg();
	// sets position of the next character to be extracted from
	// the input stream. read from 0 to the end of the stream
	temp.seekg(0, ios::end);
	long end = temp.tellg();
	temp.seekg(0, ios::beg);
	long size = end - start;
	return size;

}

/** 
* a function to send the file buffer 
*/
int Client::sendBaffer(char* buff, int buffSize, int socket)
{
	int numByte = 0;
	int wasSent = 1; // start with false
	while (numByte < buffSize)
	{
		// send to socket the data starting from buffer+ numByte in length of buffSize- numByte
		wasSent = send(socket, buff + numByte, buffSize - numByte, 0);
		if (wasSent < 0)
		{
			return FAIL;
		}
		numByte += wasSent;
	}
	return SUCCESS;
}

/**
* a function to send the file data to the server
*/
int Client::sendData(ifstream &temp, int size , int socket)
{
	char* buff;
	int needToSend = size;
	ssize_t result;

	while (needToSend > BUFF_SIZE)
	{
		if((buff = (char*) malloc(BUFF_SIZE)) == NULL)
		{
			return FAIL;
		}
		temp.read(buff, BUFF_SIZE); 
		result = sendBaffer(buff , BUFF_SIZE, socket); 
		if(result < 0)
		{
			return FAIL;
		}

		needToSend -= BUFF_SIZE;
		free(buff);
	}
	if (needToSend > 0) // in case something more to send
	{
		char* buffDelta;
		if((buffDelta = (char*) malloc(needToSend)) == NULL)
		{
			return FAIL;
		}
		//bzero(buffDelta, needToSend);
		temp.read(buffDelta, needToSend); // read what was left
		result = sendBaffer(buffDelta , needToSend, socket); // send to socket
		if(result < 0)
		{
			return FAIL;
		}
		free(buffDelta);
	}
	return SUCCESS;

}


bool isFileEx(const char *fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

bool isDir(const char* path)
{
	struct stat buf;
	stat(path, &buf);
	return S_ISDIR(buf.st_mode);
}


/**
* PARAMETERS : server- port, server- hostname, file-to-transfer, filename-in-server
*/
int main(int argc, char** argv)
{
	//less then 5 param
	if (argc < 5)
	{
		cout << "Usage: clftp server-port server-hostname file-to-transfer filename-in-server" << endl;
		exit(1);
	}
	Client* newClient = new Client(); 
	newClient->_serverPort = atoi(argv[1]); 
	if (newClient->_serverPort < MIN_SERVER_PORT || newClient->_serverPort > MAX_SERVER_PORT)
	{
		cout << "Usage: clftp server-port server-hostname file-to-transfer filename-in-server" << endl;
		exit(1);
	}

	// get host name of the server
	server = gethostbyname(argv[2]); 

	if(server == NULL)
	{
		cout << "Usage: clftp server-port server-hostname file-to-transfer filename-in-server" << endl;
		exit(1);

	}
	// get the local file name that we want to transfer

	if(!isFileEx(argv[3]) || isDir(argv[3]))
	{
		cout << "Usage: clftp server-port server-hostname file-to-transfer filename-in-server" << endl;
		delete(newClient);
		exit(1);
	}

	string* newfile1 = new string(argv[3]);
	if((newClient->_fileToTransfer = (char*)malloc(newfile1->size() + 1)) == NULL)
	{
		cerr << SYSCALL_ERROR("malloc");
		exit(1);
	}
	// copy name to the new space
	memcpy(newClient->_fileToTransfer, newfile1->c_str(), newfile1->size() + 1);

	// get the name of the file we need to save
	delete(newfile1);
	string* newfile2 = new string(argv[4]);
	newClient->_fileNameSize = newfile2->length(); 
	
	
	if((newClient->_fileNameToSave = (char*)malloc(newfile2->size() + 1)) == NULL)
	{
		cerr << SYSCALL_ERROR("malloc");
		exit(1);
	}
	// copy name to the new space
	memcpy(newClient->_fileNameToSave, newfile2->c_str(), newfile2->size() + 1);
	delete(newfile2);
	// create socket to pass informetion 
	newClient->_socket = socket(AF_INET, SOCK_STREAM, 0);
	// set all the serv_addr to 0
	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET; 
	// copy -h_length- byte sequence from server to serv_addr for the socket
	bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
	// converts from host byte order to network byte order.
	serv_addr.sin_port = htons(newClient->_serverPort);
	// open file to read from it
	ifstream temp (newClient->_fileToTransfer, ios::in | ios::binary);

	if (!temp.good())
	{
		cerr << SYSCALL_ERROR("file problem");
		exit(1);
	}

	connect(newClient->_socket, ((struct sockaddr*)&serv_addr), sizeof(serv_addr)); 
	// TODO - if cannot conect to server?
	newClient->_fileSize = newClient->getSize(temp); 
	// send informetion to server
	if(newClient->sendBaffer ((char*) &newClient->_fileSize, sizeof (unsigned int), \
	   newClient->_socket) == FAIL)
	{
		cerr << SYSCALL_ERROR("send");
		exit(1);
	}
	//Can i send this file
	char* buffAnswer;
	if((buffAnswer = (char*) malloc(ANSWER_SIZE)) == NULL)
	{
		cerr << SYSCALL_ERROR("malloc");
		exit(1);
	}

	if(recv(newClient->_socket, buffAnswer, ANSWER_SIZE, 0) == FAIL)
	{
		cerr << SYSCALL_ERROR("recv");
		exit(1);
	}
	else if(buffAnswer[0] == NEGETIVE)
	{
		cout << "Transmission failed: too big file" << endl;
		temp.close();
		free(buffAnswer);
		close(newClient->_socket);
		delete(newClient);
		exit(1); 
	}

	
	if(newClient->sendBaffer((char*) &newClient->_fileNameSize , sizeof(unsigned int), \
	   newClient->_socket) == FAIL)
	{
		cerr << SYSCALL_ERROR("send");
		exit(1);
	}
	//int wasSent = send(newClient->_socket, newClient->_fileNameToSave, 1024, 0);
	if(newClient->sendBaffer(newClient->_fileNameToSave, newClient->_fileNameSize, \
	   newClient->_socket) == FAIL)
	{
		cerr << SYSCALL_ERROR("send");
		exit(1);
	}
	if(newClient->sendData(temp, newClient->_fileSize, newClient->_socket) == FAIL)
	{
		cerr << SYSCALL_ERROR("send");
		exit(1);
	}
	//close	
	temp.close();
	free(buffAnswer);
	close(newClient->_socket);
	delete(newClient);




	return SUCCESS;

}

