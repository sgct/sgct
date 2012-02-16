#if !(_MSC_VER >= 1400) //if not visual studio 2005 or later
    #define _WIN32_WINNT 0x501
#endif

#include "../include/sgct/SGCTNetwork.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/MessageHandler.h"
#include <stdlib.h>
#include <stdio.h>
#include <ws2tcpip.h>
#include <GL/glfw.h>

GLFWmutex gMutex;
GLFWmutex gDecoderMutex;

void GLFWCALL communicationHandler(void *arg);
void GLFWCALL listenForClients(void *arg);
void GLFWCALL pollClients(void *arg);

core_sgct::SGCTNetwork::SGCTNetwork()
{
	mainThreadID = -1;
	pollClientStatusThreadID = -1;
	mSocket = INVALID_SOCKET;
	mDecoderCallbackFn = NULL;
	mNumberOfNodesInConfig = 0;
	mAllNodesConnected = false;
	mServerType = SyncServer;
	mBufferSize = 512;
	mRequestedSize = 512;

	WSADATA wsaData;
	WORD version;
	int error;

	version = MAKEWORD( 2, 2 );

	error = WSAStartup( version, &wsaData );

	if ( error != 0 ||
		LOBYTE( wsaData.wVersion ) != 2 ||
		HIBYTE( wsaData.wVersion ) != 2 )
	{
		/* incorrect WinSock version */
		WSACleanup();
		throw "Winsock 2.2 startup failed!";
	}

	//get name & local ips
	char tmpStr[128];
    if (gethostname(tmpStr, sizeof(tmpStr)) == SOCKET_ERROR)
	{
        WSACleanup();
		throw "Failed to get host name!";
    }
	hostName.assign(tmpStr);

	struct hostent *phe = gethostbyname(tmpStr);
    if (phe == 0)
	{
        WSACleanup();
		throw "Bad host lockup!";
    }

    for (int i = 0; phe->h_addr_list[i] != 0; ++i)
	{
        struct in_addr addr;
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		localAddresses.push_back( inet_ntoa(addr) );
    }
}

void core_sgct::SGCTNetwork::init(const std::string port, const std::string ip, bool _isServer, unsigned int numberOfNodesInConfig, int serverType)
{
	mNumberOfNodesInConfig = numberOfNodesInConfig;
	mServer = _isServer;
	mServerType = serverType;
	gMutex = glfwCreateMutex();
	gDecoderMutex = glfwCreateMutex();
	mRunning = false;
	mBufferSize = sgct::SharedData::Instance()->getBufferSize();

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	int iResult;

	if( mServer )
		iResult = getaddrinfo(NULL, port.c_str(), &hints, &result);
	else
		iResult = getaddrinfo(ip.c_str(), port.c_str(), &hints, &result);
	if (iResult != 0)
	{
		WSACleanup();
		throw "Failed to parse hints for connection.";
	}

	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr=result;

	// Create a SOCKET for the server to listen for client connections
	mSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
	if (mSocket == INVALID_SOCKET)
	{
		freeaddrinfo(result);
		WSACleanup();
		throw "Failed to init socket!";
	}

	int flag = 1;
    iResult = setsockopt(mSocket,            /* socket affected */
		IPPROTO_TCP,     /* set option at TCP level */
		TCP_NODELAY,     /* name of option */
		(char *) &flag,  /* the cast is historical cruft */
		sizeof(int));    /* length of option value */

	if (iResult < 0)
	{
		freeaddrinfo(result);
		WSACleanup();
		throw "Failed to set TCP_NODELAY!";
	}

	if( mServer )
	{
		// Setup the TCP listening socket
		iResult = bind( mSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			freeaddrinfo(result);
			closesocket(mSocket);
			WSACleanup();
			throw "bind socket failed.";
		}

		freeaddrinfo(result);

		mRunning = true; //the following threads will run if this variable is true

		mainThreadID = glfwCreateThread( listenForClients, this );
		if( mainThreadID < 0)
		{
			WSACleanup();
			closesocket(mSocket);
			throw "Failed to start listen thread!";
		}

		pollClientStatusThreadID = glfwCreateThread( pollClients, this );
		if( pollClientStatusThreadID < 0)
		{
			WSACleanup();
			closesocket(mSocket);
			throw "Failed to start poll thread!";
		}
	}
	else //client
	{
		// Connect to server.
		while( true )
		{
			sgct::MessageHandler::Instance()->print("Attempting to connect to server...\n");
			iResult = connect( mSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult != SOCKET_ERROR)
				break;

			glfwSleep(0.25); //wait for next attempt
		}

		mRunning = true;
		freeaddrinfo(result);

		core_sgct::TCPData * dataPtr = new core_sgct::TCPData;
		dataPtr->mNetwork = this;

		mainThreadID = glfwCreateThread( communicationHandler, dataPtr );
		if( mainThreadID < 0)
		{
			WSACleanup();
			closesocket(mSocket);
			throw "Failed to connecto to server (thread error)!";
		}
	}
}

void core_sgct::SGCTNetwork::setBufferSize(unsigned int newSize)
{
	mRequestedSize = newSize;
}

void GLFWCALL listenForClients(void *arg)
{
	core_sgct::SGCTNetwork * nPtr = (core_sgct::SGCTNetwork *)arg;

	while ( nPtr->isRunning() )
	{
		glfwLockMutex( gMutex );
		if( listen( nPtr->mSocket, SOMAXCONN ) == SOCKET_ERROR )
		{
			sgct::MessageHandler::Instance()->print("SocketError!\n");
			break;
		}

		core_sgct::ConnectionData * cd = NULL;
		cd = new core_sgct::ConnectionData();

		// Wait for connection
		cd->client_socket = accept(nPtr->mSocket, NULL, NULL);

		if (cd->client_socket != INVALID_SOCKET)
		{
			nPtr->clients.push_back(cd);

			core_sgct::TCPData * dataPtr = new core_sgct::TCPData;
			dataPtr->mClientIndex = static_cast<int>(nPtr->clients.size()) - 1;
			dataPtr->mNetwork = nPtr;
			dataPtr->mNetwork->clients[ dataPtr->mClientIndex ]->connected = true;

			//check if all connected and don't count itself
			if(dataPtr->mNetwork->getNumberOfNodesInConfig()-1 == dataPtr->mNetwork->clients.size())
			{
				dataPtr->mNetwork->setAllNodesConnected(true);
			}

			//start reading thread
			nPtr->clients[ dataPtr->mClientIndex ]->threadID = glfwCreateThread( communicationHandler, dataPtr );

			if( nPtr->clients[ dataPtr->mClientIndex ]->threadID < 0)
			{
				closesocket(cd->client_socket);
				sgct::MessageHandler::Instance()->print("Failed to connecto to client (thread error)!\n");
			}
		}
		glfwUnlockMutex( gMutex );
	}//end while
}

void core_sgct::SGCTNetwork::setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback)
{
	mDecoderCallbackFn = callback;
}

/*
function to decode messages on the client from the server
*/
void GLFWCALL communicationHandler(void *arg)
{
	core_sgct::TCPData * dataPtr = (core_sgct::TCPData *)arg;

	//say hi
	if( dataPtr->mNetwork->isServer() )
		send(dataPtr->mNetwork->clients[ dataPtr->mClientIndex ]->client_socket, "Connected..\r\n", 13, 0);
	else
		send(dataPtr->mNetwork->mSocket, "Connected.\r\n", 13, 0);

	//init buffer
	int recvbuflen = dataPtr->mNetwork->mBufferSize;
	char * recvbuf;
	recvbuf = (char *) malloc(dataPtr->mNetwork->mBufferSize);
	//memset(recvbuf,0,dataPtr->mNetwork->mBufferSize);

	std::string extBuffer;

	// Receive data until the server closes the connection
	int iResult;
	do
	{
		//resize buffer request
		if( dataPtr->mNetwork->mRequestedSize > dataPtr->mNetwork->mBufferSize )
		{
			glfwLockMutex( gDecoderMutex );
				sgct::MessageHandler::Instance()->print("Network: New package size is %d\n", dataPtr->mNetwork->mRequestedSize);
				dataPtr->mNetwork->mBufferSize = dataPtr->mNetwork->mRequestedSize;
				recvbuflen = dataPtr->mNetwork->mRequestedSize;
				free(recvbuf);
				recvbuf = (char *)malloc(dataPtr->mNetwork->mRequestedSize);
			glfwUnlockMutex( gDecoderMutex );
		}
		
		
		//check to use correct socket
		if( dataPtr->mNetwork->isServer() )
			iResult = recv( dataPtr->mNetwork->clients[ dataPtr->mClientIndex ]->client_socket, recvbuf, recvbuflen, 0);
		else
			iResult = recv( dataPtr->mNetwork->mSocket, recvbuf, recvbuflen, 0);

		if (iResult > 0)
		{
			//if( dataPtr->mNetwork->isServer() )
			//	fprintf(stderr, "\nData received: [%s]\n", recvbuf);

			if( dataPtr->mNetwork->getTypeOfServer() == core_sgct::SGCTNetwork::SyncServer )
			{
				//check type of message
				//Resize if needed
				if( recvbuf[0] == core_sgct::SGCTNetwork::SizeHeader && recvbuflen > 4)
				{
					union
					{
						unsigned int newSize;
						char c[4];
					} cui;
					//parse int
					cui.c[0] = recvbuf[1];
					cui.c[1] = recvbuf[2];
					cui.c[2] = recvbuf[3];
					cui.c[3] = recvbuf[4];

					glfwLockMutex( gDecoderMutex );
					sgct::MessageHandler::Instance()->print("Network: New package size is %d\n", cui.newSize);
					dataPtr->mNetwork->mBufferSize = cui.newSize;
					recvbuflen = cui.newSize;
					free(recvbuf);
					recvbuf = (char *)malloc(cui.newSize);
					//memset(recvbuf,0,cui.newSize);
					glfwUnlockMutex( gDecoderMutex );
				}
				else if( recvbuf[0] == core_sgct::SGCTNetwork::SyncHeader && dataPtr->mNetwork->mDecoderCallbackFn != NULL)
				{
					glfwLockMutex( gDecoderMutex );
						(dataPtr->mNetwork->mDecoderCallbackFn)(recvbuf+1, iResult-1, dataPtr->mClientIndex);
					glfwUnlockMutex( gDecoderMutex );
				}
				else if( recvbuf[0] == core_sgct::SGCTNetwork::ClusterConnected )
				{
					glfwLockMutex( gDecoderMutex );
						dataPtr->mNetwork->setAllNodesConnected(true);
					glfwUnlockMutex( gDecoderMutex );
				}
			}
			else if(dataPtr->mNetwork->getTypeOfServer() == core_sgct::SGCTNetwork::ExternalControl)
			{
				std::string tmpStr(recvbuf);
				extBuffer += tmpStr.substr(0, iResult);
				std::size_t found = extBuffer.find("\r\n");
				if( found != std::string::npos )
				{
					std::string extMessage = extBuffer.substr(0,found);
					extBuffer = extBuffer.substr(found+2);//jump over \r\n

					glfwLockMutex( gDecoderMutex );
						if( dataPtr->mNetwork->mDecoderCallbackFn != NULL )
						{
							(dataPtr->mNetwork->mDecoderCallbackFn)(extMessage.c_str(), extMessage.size(), dataPtr->mClientIndex);
						}
						dataPtr->mNetwork->sendStrToAllClients("OK\r\n");
					glfwUnlockMutex( gDecoderMutex );
				}
			}
		}
		else if (iResult == 0)
		{
			glfwLockMutex( gDecoderMutex );
				sgct::MessageHandler::Instance()->print("TCP Connection closed [client: %d]\n", dataPtr->mClientIndex);
				if(dataPtr->mNetwork->isServer())
					dataPtr->mNetwork->setClientConnectionStatus(dataPtr->mClientIndex,false);
				else
					dataPtr->mNetwork->setRunningStatus(false);
			glfwUnlockMutex( gDecoderMutex );
		}
		else
		{
			glfwLockMutex( gDecoderMutex );
				sgct::MessageHandler::Instance()->print("TCP recv failed: %d [client: %d]\n", WSAGetLastError(), dataPtr->mClientIndex);
				if(dataPtr->mNetwork->isServer())
					dataPtr->mNetwork->setClientConnectionStatus(dataPtr->mClientIndex,false);
				else
					dataPtr->mNetwork->setRunningStatus(false);
			glfwUnlockMutex( gDecoderMutex );
		}
	} while (iResult > 0 || 
		(dataPtr->mNetwork->isServer() ?
			dataPtr->mNetwork->isClientConnected(dataPtr->mClientIndex) :
			dataPtr->mNetwork->isRunning()));

	free(recvbuf);
}

void GLFWCALL pollClients(void *arg)
{
	core_sgct::SGCTNetwork * nwPtr = (core_sgct::SGCTNetwork *)arg;
	while(nwPtr->isRunning())
	{
		//fprintf(stderr, "Polling clients...\n");
		for(unsigned int i=0; i<nwPtr->clients.size(); i++)
			if( nwPtr->clients[i] != NULL && !nwPtr->clients[i]->connected)
				nwPtr->terminateClient(i);

		glfwSleep(1.0); //sleep one sec
	}
}

void core_sgct::SGCTNetwork::sendStrToAllClients(const std::string str)
{
	int iResult = 0;
	for(unsigned int i=0; i<clients.size(); i++)
		if( clients[i] != NULL && clients[i]->connected )
		{
			iResult = send(clients[i]->client_socket, str.c_str(), str.size(), 0);
			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::Instance()->print("Send failed to client %d with error: %d\n", i, WSAGetLastError());
		}
}

void core_sgct::SGCTNetwork::sendDataToAllClients(void * data, int lenght)
{
	int iResult = 0;
	for(unsigned int i=0; i<clients.size(); i++)
		if( clients[i] != NULL && clients[i]->connected )
		{
			iResult = send(clients[i]->client_socket, (const char *)data, lenght, 0);
			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::Instance()->print("Send failed to client %d with error: %d\n", i, WSAGetLastError());
		}
}

void core_sgct::SGCTNetwork::setAllNodesConnected(bool state)
{
	mAllNodesConnected = state;
	if( isServer() && mAllNodesConnected )
	{
		char tmpMessage = ClusterConnected;
		sendDataToAllClients(&tmpMessage, 1);
	}
}

void core_sgct::SGCTNetwork::setClientConnectionStatus(int clientIndex, bool status)
{
	if(clients[clientIndex] != NULL)
		clients[clientIndex]->connected = status;
}

void core_sgct::SGCTNetwork::terminateClient(int index)
{
	if( index >= 0 && index < static_cast<int>(clients.size()) && clients[index] != NULL )
	{
		sgct::MessageHandler::Instance()->print("Closing client connection %d...\n", index);
		clients[index]->connected = false;

		//Brutal kill but we cannot wait since the thead is waiting for messages (blocking)
		glfwDestroyThread( clients[index]->threadID );
		if( clients[index]->client_socket != INVALID_SOCKET )
		{
			sgct::MessageHandler::Instance()->print("Closing socket on node %d...\n", index);
			shutdown(clients[index]->client_socket, SD_BOTH);
			closesocket( clients[index]->client_socket );
		}
		sgct::MessageHandler::Instance()->print("Done!\n");

		delete clients[index];
		clients[index] = NULL;
	}
}

void core_sgct::SGCTNetwork::sync()
{
	if( isServer() )
	{

        //check if buffer needs to be re-sized
        if( sgct::SharedData::Instance()->getDataSize() > mBufferSize )
        {
            mBufferSize = sgct::SharedData::Instance()->getDataSize();
            char * p = (char *)&mBufferSize;

            //create package
            char resizeMessage[5];
            resizeMessage[0] = SGCTNetwork::SizeHeader;
            resizeMessage[1] = p[0];
            resizeMessage[2] = p[1];
            resizeMessage[3] = p[2];
            resizeMessage[4] = p[3];

            sendDataToAllClients(resizeMessage,5);
        }
        sendDataToAllClients( sgct::SharedData::Instance()->getDataBlock(), sgct::SharedData::Instance()->getDataSize() );
        //should wait for reply from clients...
	}
	else// if(isRunning()) //if client
	{
        int iResult = 0;

		//check if message fits in buffer
        if(sgct::MessageHandler::Instance()->getDataSize() > 1 && //larger than first header byte + '\0'
           sgct::MessageHandler::Instance()->getDataSize() < mBufferSize)
        {
            /* Don't remove this pointer, somehow the send function doesn't
			work during the first call without setting the pointer first!!! */
			const char * messageToSend = sgct::MessageHandler::Instance()->getMessage();

			iResult = send(mSocket,
                 messageToSend,
                 sgct::MessageHandler::Instance()->getDataSize(),
                 0);

			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::Instance()->print("Send failed with error: %d\n", WSAGetLastError());

			sgct::MessageHandler::Instance()->clearBuffer(); //clear the buffer
        }
        else if(sgct::MessageHandler::Instance()->getDataSize() > 1 && //larger than first header byte + '\0'
                sgct::MessageHandler::Instance()->getDataSize() >= mBufferSize )
        {
			/* Don't remove this pointer, somehow the send function doesn't
			work during the first call without setting the pointer first!!! */
			const char * messageToSend = sgct::MessageHandler::Instance()->getTrimmedMessage(mBufferSize-1);

			iResult = send(mSocket,
                 messageToSend,
                 sgct::MessageHandler::Instance()->getTrimmedDataSize(),
                 0);

			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::Instance()->print("Send failed with error: %d\n", WSAGetLastError());
        }
	}
}

void core_sgct::SGCTNetwork::close()
{
	char gameOver[7];
	gameOver[0] = 24; //ASCII for cancel
	gameOver[1] = '\r';
	gameOver[2] = '\n';
	gameOver[3] = 27; //ASCII for Esc
	gameOver[4] = '\r';
	gameOver[5] = '\n';
	gameOver[6] = '\0';
	sendDataToAllClients(gameOver, 7);
	
	mDecoderCallbackFn = NULL;

	for(unsigned int i=0; i<clients.size(); i++)
	{
		terminateClient(i);
	}

	mRunning = false;
	if( pollClientStatusThreadID != -1 )
		glfwWaitThread(pollClientStatusThreadID, GLFW_WAIT);

	sgct::MessageHandler::Instance()->print("Closing server connection...");
	if( mainThreadID != -1 )
		glfwDestroyThread(mainThreadID); //blocking sockets -> cannot wait for thread so just kill it brutally
	if( mSocket != INVALID_SOCKET )
	{
		shutdown(mSocket, SD_BOTH);
		closesocket( mSocket );
	}
	WSACleanup();
	sgct::MessageHandler::Instance()->print(" Done!\n");
}

bool core_sgct::SGCTNetwork::matchHostName(const std::string name)
{
	return strcmp(name.c_str(), hostName.c_str() ) == 0;
}

bool core_sgct::SGCTNetwork::matchAddress(const std::string ip)
{
	for( unsigned int i=0; i<localAddresses.size(); i++)
		if( strcmp(ip.c_str(), localAddresses[i].c_str()) == 0 )
			return true;
	//No match
	return false;
}
