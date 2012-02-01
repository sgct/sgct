#include "Network.h"

#include <stdlib.h>
#include <stdio.h>
#include <ws2tcpip.h>

GLFWmutex gMutex;

void GLFWCALL communicationHandler(void *arg);
void GLFWCALL listenForClients(void *arg);
void GLFWCALL decode(void *arg);

Network::Network()
{
	threadID = -1;
	mSocket = INVALID_SOCKET;

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

void Network::init(const std::string port, const std::string ip, bool _isServer, sgct::SharedData * _shdPtr)
{
	isServer = _isServer;
	shdPtr = _shdPtr;
	gMutex = glfwCreateMutex();
	running = false;

	struct addrinfo *result = NULL, *ptr = NULL, hints;

	ZeroMemory(&hints, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	// Resolve the local address and port to be used by the server
	int iResult;
	
	if( isServer )
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

	
	if( isServer )
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
	else
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

		running = true;
		freeaddrinfo(result);

		threadID = glfwCreateThread( communicationHandler, this );
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
	Network * nPtr = (Network *)arg;

	while ( true )
	{
		glfwLockMutex( gMutex );
		if( listen( nPtr->mSocket, SOMAXCONN ) == SOCKET_ERROR )
		{
			fprintf( stderr, "SocketError!\n");
			break;
		}
		
		ConnectionData cd;

		// Wait for connection
		cd.client_socket = accept(nPtr->mSocket, NULL, NULL);
		
		if (cd.client_socket != INVALID_SOCKET)
		{
			nPtr->clients.push_back(cd);
			unsigned int clientIndex = nPtr->clients.size() - 1;
			TCPData * dataPtr = new TCPData;
			dataPtr->clientIndex = clientIndex;
			dataPtr->mNetwork = nPtr;

			nPtr->clients[clientIndex].threadID = glfwCreateThread( decode, dataPtr );
			nPtr->clients[clientIndex].connected = true;
		}

		glfwUnlockMutex( gMutex );
	}
}

void GLFWCALL decode(void *arg)
{
	TCPData * dataPtr = (TCPData*)arg;
	
	fprintf( stderr, "Decoder for client %d is active.\n", dataPtr->clientIndex );
	Network * nPtr = dataPtr->mNetwork;
}

void GLFWCALL communicationHandler(void *arg)
{
	Network * nPtr = (Network *)arg;
	int recvbuflen = nPtr->shdPtr->getBufferSize();
	char * recvbuf;
	recvbuf = new char[nPtr->shdPtr->getBufferSize()];

	// Receive data until the server closes the connection
	int iResult;
	do
	{
		iResult = recv(nPtr->mSocket, recvbuf, recvbuflen, 0);
		if (iResult > 0)
		{
			glfwLockMutex( gMutex );
				nPtr->shdPtr->decode(recvbuf, recvbuflen);
			glfwUnlockMutex( gMutex );
		}
		else if (iResult == 0)
		{
			fprintf(stderr, "TCP Connection closed\n");
		}
		else
		{
			fprintf(stderr, "TCP recv failed: %d\n", WSAGetLastError());
		}
	} while (iResult > 0);

	glfwLockMutex( gMutex );
		nPtr->setRunning(false);
	glfwUnlockMutex( gMutex );
	delete recvbuf;
}

void Network::sendStrToAllClients(const std::string str)
{
	for(unsigned int i=0; i<clients.size(); i++)
		if( clients[i].connected )
		{
			send(clients[i].client_socket, str.c_str(), str.size(), 0);
		}
}

void Network::sendDataToAllClients(void * data, int lenght)
{
	for(unsigned int i=0; i<clients.size(); i++)
		if( clients[i].connected )
		{
			send(clients[i].client_socket, (const char *)data, lenght, 0);
		}
}

void Network::sync()
{
	sendDataToAllClients( shdPtr->getDataBlock(), shdPtr->getDataSize() );

	//should wait for reply from clients...
}

void Network::close()
{
	for(unsigned int i=0; i<clients.size(); i++)
	{
		fprintf( stderr, "Closing client connection %d...", i);
		glfwDestroyThread( clients[i].threadID );
		if( clients[i].client_socket != INVALID_SOCKET )
		{
			closesocket( clients[i].client_socket );
		}

		fprintf( stderr, " Done!\n");
	}	
	
	fprintf( stderr, "Closing server connection...");
	if( threadID != -1 )
		glfwDestroyThread( threadID	);
	if( mSocket != INVALID_SOCKET )
		closesocket( mSocket );
	WSACleanup();
	fprintf( stderr, " Done!\n");
}

bool Network::matchHostName(const std::string name)
{
	return strcmp(name.c_str(), hostName.c_str() ) == 0;
}

bool Network::matchAddress(const std::string ip)
{
	for( unsigned int i=0; i<localAddresses.size(); i++)
		if( strcmp(ip.c_str(), localAddresses[i].c_str()) == 0 )
			return true;
	//No match
	return false;
}