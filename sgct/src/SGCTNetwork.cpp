#include "sgct/SGCTNetwork.h"
#include <stdlib.h>
#include <stdio.h>
#include <ws2tcpip.h>
#include <GL/glfw.h>

GLFWmutex gMutex;

void GLFWCALL communicationHandler(void *arg);
void GLFWCALL listenForClients(void *arg);

core_sgct::SGCTNetwork::SGCTNetwork()
{
	threadID = -1;
	mSocket = INVALID_SOCKET;
	mDecoderCallbackFn = NULL;

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

void core_sgct::SGCTNetwork::init(const std::string port, const std::string ip, bool _isServer, sgct::SharedData * _shdPtr)
{
	mServer = _isServer;
	shdPtr = _shdPtr;
	gMutex = glfwCreateMutex();
	mRunning = false;

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
	
		threadID = glfwCreateThread( listenForClients, this );
		if( threadID < 0)
		{
			WSACleanup();
			closesocket(mSocket);
			throw "Failed to start listen thread!";
		}
	}
	else //client
	{
		// Connect to server.
		while( true )
		{
			fprintf(stderr, "Attempting to connect to server...\n");
			iResult = connect( mSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult != SOCKET_ERROR)
				break;

			glfwSleep(0.25); //wait for next attempt
		}

		mRunning = true;
		freeaddrinfo(result);

		core_sgct::TCPData * dataPtr = new core_sgct::TCPData;
		dataPtr->mNetwork = this;

		threadID = glfwCreateThread( communicationHandler, dataPtr );
		if( threadID < 0)
		{
			WSACleanup();
			closesocket(mSocket);
			throw "Failed to connecto to server (thread error)!";
		}
	}
}

void GLFWCALL listenForClients(void *arg)
{
	core_sgct::SGCTNetwork * nPtr = (core_sgct::SGCTNetwork *)arg;

	while ( true )
	{
		glfwLockMutex( gMutex );
		if( listen( nPtr->mSocket, SOMAXCONN ) == SOCKET_ERROR )
		{
			fprintf( stderr, "SocketError!\n");
			break;
		}

		core_sgct::ConnectionData cd;

		// Wait for connection
		cd.client_socket = accept(nPtr->mSocket, NULL, NULL);
		
		if (cd.client_socket != INVALID_SOCKET)
		{	
			nPtr->clients.push_back(cd);
			
			core_sgct::TCPData * dataPtr = new core_sgct::TCPData;
			dataPtr->mClientIndex = static_cast<int>(nPtr->clients.size()) - 1;
			dataPtr->mNetwork = nPtr;
			dataPtr->mNetwork->clients[ dataPtr->mClientIndex ].connected = true;
			
			//start reading thread
			nPtr->clients[ dataPtr->mClientIndex ].threadID = glfwCreateThread( communicationHandler, dataPtr );

			if( nPtr->clients[ dataPtr->mClientIndex ].threadID < 0)
			{
				closesocket(cd.client_socket);
				fprintf( stderr, "Failed to connecto to client (thread error)!\n");
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
	
	//init buffer
	int recvbuflen = dataPtr->mNetwork->shdPtr->getBufferSize();
	char * recvbuf;
	recvbuf = new char[dataPtr->mNetwork->shdPtr->getBufferSize()];

	// Receive data until the server closes the connection
	int iResult;
	do
	{
		if( dataPtr->mNetwork->isServer() )
			iResult = recv( dataPtr->mNetwork->clients[ dataPtr->mClientIndex ].client_socket, recvbuf, recvbuflen, 0);
		else
			iResult = recv( dataPtr->mNetwork->mSocket, recvbuf, recvbuflen, 0);
			
		if (iResult > 0)
		{
			if( dataPtr->mNetwork->mDecoderCallbackFn != NULL )
				(dataPtr->mNetwork->mDecoderCallbackFn)(recvbuf, recvbuflen, dataPtr->mClientIndex);
		}
		else if (iResult == 0)
		{
			fprintf(stderr, "TCP Connection closed [client: %d]\n", dataPtr->mClientIndex);
		}
		else
		{
			fprintf(stderr, "TCP recv failed: %d [client: %d]\n", WSAGetLastError(), dataPtr->mClientIndex);
		}
	} while (iResult > 0);

	dataPtr->mNetwork->setRunning(false);
	delete recvbuf;
}

void core_sgct::SGCTNetwork::sendStrToAllClients(const std::string str)
{
	for(unsigned int i=0; i<clients.size(); i++)
		if( clients[i].connected )
		{
			send(clients[i].client_socket, str.c_str(), str.size(), 0);
		}
}

void core_sgct::SGCTNetwork::sendDataToAllClients(void * data, int lenght)
{
	for(unsigned int i=0; i<clients.size(); i++)
		if( clients[i].connected )
		{
			send(clients[i].client_socket, (const char *)data, lenght, 0);
		}
}

void core_sgct::SGCTNetwork::sync()
{
	sendDataToAllClients( shdPtr->getDataBlock(), shdPtr->getDataSize() );

	//should wait for reply from clients...
}

void core_sgct::SGCTNetwork::close()
{
	mDecoderCallbackFn = NULL;

	for(unsigned int i=0; i<clients.size(); i++)
	{
		fprintf( stderr, "Closing client connection %d...", i);
		glfwDestroyThread( clients[i].threadID );
		if( clients[i].client_socket != INVALID_SOCKET )
		{
			fprintf( stderr, "Closing socket on node %d...", i);
			shutdown(clients[i].client_socket, SD_BOTH);
			closesocket( clients[i].client_socket );
		}
		fprintf( stderr, " Done!\n");
	}	
	
	fprintf( stderr, "Closing server connection...");
	if( threadID != -1 )
		glfwDestroyThread( threadID	);
	if( mSocket != INVALID_SOCKET )
	{
		shutdown(mSocket, SD_BOTH);
		closesocket( mSocket );
	}
	WSACleanup();
	fprintf( stderr, " Done!\n");
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