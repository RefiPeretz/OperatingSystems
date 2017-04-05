/**
* a file that contain the server implementation
*/
#include "srftp.h"



//#ifndef SYSCALL_ERROR
#define SYSCALL_ERROR(syscall) "Error: function: " << syscall << "errno: " << errno << "\n"
//#endif


static Server* myServer;

struct handlerPacket
{
	int socket;
};

/**
* constractor of the class Server
*/
Server::Server(int port, int maxSize)
{
	_srvPort = port;
	_maxFileSize = maxSize;
}

/**
* destractor of the class Server
*/
Server::~Server()
{

}

/**
* function that restart the server for the first time
*/
int Server::uploadServer()
{
 	
	gethostname(srvHostName, MAX_HOSTNAME);
	srvHost = gethostbyname(srvHostName);
	if (srvHost == NULL)
	{
		cerr << SYSCALL_ERROR("gethostbyname"); //TODO do we need server name???
		exit(EXIT_FAIL);
	}
	// Get own address
	srvSocketAddress.sin_family = srvHost->h_addrtype;
	srvSocketAddress.sin_port = htons(_srvPort);
	memcpy(&srvSocketAddress.sin_addr, srvHost->h_addr, srvHost->h_length);
	// Create welcome socket
	if ((_srvSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		cerr << SYSCALL_ERROR("socket FAIL");
		exit(EXIT_FAIL);
	}


	if (bind(_srvSocket, (struct sockaddr*) &srvSocketAddress,
		sizeof(struct sockaddr_in)) < 0)
	{
		cerr << SYSCALL_ERROR("bind FAIL");
		close(_srvSocket);
		exit(EXIT_FAIL);
	}
 	// waiting for a client
	if(listen(_srvSocket, LIMIT_CONNECTIONS) < 0)
	{
		cerr << SYSCALL_ERROR("bind FAIL");
		close(_srvSocket);
		exit(EXIT_FAIL);

	}
	return _srvSocket;
}





/**
* function that check if the parameters from the user is valid
*/

int validateParameters(int argc, char const *argv[])
{

	if (argc != 3 || atoi(argv[1]) < MIN_PORT_NUM || atoi(argv[1]) > MAX_PORT_NUM ||\
	    strtol(argv[2], NULL, 10) < 0)
	{
		return FAIL;
	}
	return SUCCESS;       
   
}

/**
* a function that check if a file exist
*/
bool isFileEx(const char *fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}



/**
* function that read the the data from the file that the client sent
*/
int readAndWriteDataClient(int fileSize, int clientSocket, char* fileName)
{
	fstream outputStream;
	char* readBuff = NULL; 
	int readByte = 0;
	int realRead = 0;
	int needReed = 0;
	//int endWhileOffset = fileSize / BUFF_SIZE;



	outputStream.open(fileName, ofstream::out | ofstream::binary);

	while(fileSize > 0)
	{
		readBuff = (char*) malloc(BUFF_SIZE);
		needReed = fileSize > BUFF_SIZE ? BUFF_SIZE : fileSize;
		if((realRead = recv(clientSocket, readBuff, needReed, 0)) == FAIL)
		{

			cerr << SYSCALL_ERROR("recv");
			free(readBuff);
			outputStream.close();
			return FAIL;
		}

		if (!outputStream.good())
		{

			free(readBuff);
			return FAIL;
		}

		outputStream.write(readBuff, realRead); 
		

		readByte += realRead;
		fileSize -= realRead;
		free(readBuff);
	}
	outputStream.close();

	return readByte;
}

/**
* function that handle the connection
*/
void* connectionHandler(void* data)
{

	unsigned int fileSize;
	unsigned int maxFileSize = myServer->_maxFileSize;

	handlerPacket* handlerData = (handlerPacket*)(data);

	int clientSocket = handlerData->socket;
	delete(handlerData);

	//Receving file size
	char* buffFileSize;
	if((buffFileSize = (char*) malloc(sizeof(unsigned int))) == NULL)
	{
		cerr << SYSCALL_ERROR("malloc");
		close(clientSocket); 
		return nullptr;
	}
	if(recv(clientSocket, buffFileSize, sizeof(unsigned int), 0) == FAIL) 
	{
		cerr << SYSCALL_ERROR("recv");
		free(buffFileSize);
		close(clientSocket); 
		return nullptr;
	}

	
	fileSize = (*((unsigned int*)(buffFileSize)));
	free(buffFileSize);
	char* buffAnswer;

	if((buffAnswer = (char*) malloc(ANSWER_SIZE)) == NULL)
	{
		cerr << SYSCALL_ERROR("malloc");
		close(clientSocket); 
		return nullptr;

	}

	buffAnswer[1] = '\0'; 
	if(fileSize > maxFileSize)
	{
	
		
		buffAnswer[0] = NEGETIVE;
		if(send(clientSocket, buffAnswer, ANSWER_SIZE, 0) == FAIL)
		{

			cerr << SYSCALL_ERROR("send");
			free(buffAnswer);
			close(clientSocket); 
			return nullptr;
		}
		free(buffAnswer);
		close(clientSocket); 
		return nullptr;
	}
	
	//Its okay to work with this kind of size

	buffAnswer[0] = POSITIVE; 
	
	if(send(clientSocket, buffAnswer, ANSWER_SIZE, 0) == FAIL) //TODO magic
	{
		cerr << SYSCALL_ERROR("send");
		free(buffAnswer);
		close(clientSocket);
		return nullptr;
	}

	free(buffAnswer);



	//Size of file path
	char* buffFilePathSize;
	if((buffFilePathSize = (char*) malloc(sizeof(unsigned int))) == NULL)
	{
		cerr << SYSCALL_ERROR("malloc");
		close(clientSocket); 
		pthread_exit(NULL);
	}

	if(recv(clientSocket, buffFilePathSize, sizeof(unsigned int), 0 ) == FAIL)
	{
		cerr << SYSCALL_ERROR("recv");
		free(buffFilePathSize);
		close(clientSocket); //Close connection TODO error
		return nullptr;
	}

	unsigned int filePathSize = (*((unsigned int*)(buffFilePathSize)));

	free(buffFilePathSize);

	char* fileName;
	char* buffPath;

	if((fileName = (char*) malloc(filePathSize + 1)) == NULL)
	{
		cerr << SYSCALL_ERROR("malloc");
		close(clientSocket); 
		return nullptr;

	}

	if((buffPath = (char*) malloc(filePathSize + 1)) == NULL)
	{
		cerr << SYSCALL_ERROR("malloc");
		free(fileName);
		close(clientSocket); 
		return nullptr;

	}



	if(recv(clientSocket, buffPath, filePathSize, 0 ) == FAIL)
	{
		cerr << SYSCALL_ERROR("recv");
		free(fileName);
		free(buffPath);
		close(clientSocket); 
		return nullptr;
	}
	memcpy(fileName , buffPath, filePathSize);

	free(buffPath);
	//Ready for data
	fileName[filePathSize] = '\0';
	
	int readAndWriteDataResult = readAndWriteDataClient(fileSize, clientSocket, fileName);

	free(fileName);
	if(readAndWriteDataResult == FAIL)
	{
		close(clientSocket); 
		pthread_exit(NULL);
	}
	
	close(clientSocket);
	return nullptr;

}

/**
* function that create a thread
* PARAM: createThread
*/
int createThread(int clientSocket)
{
	pthread_t socketThread;
	handlerPacket* data =  new handlerPacket();
	data->socket = clientSocket;
	if(pthread_create(&socketThread, NULL, connectionHandler, data))
	{
		cerr << SYSCALL_ERROR("pthread_create");
		close(clientSocket);
	}
	pthread_detach(socketThread);
	return 0 ;

}

/**
* main function
*/

int main(int argc , char const *argv[])
{
	if(validateParameters(argc , argv) !=  SUCCESS) 
	{
		cout << USEAGE_ERROR << endl;
		return FAIL;
	}


	myServer = new Server(atoi(argv[1]), atoi(argv[2]));

	myServer->uploadServer();

	int clientSocket;
	struct sockaddr clientSocketAddress;
	socklen_t sockAddrLen = sizeof(struct sockaddr_storage);

	while((clientSocket = accept(myServer->_srvSocket, &clientSocketAddress, &sockAddrLen)) != FAIL)
	{
		createThread(clientSocket);
	}

}





