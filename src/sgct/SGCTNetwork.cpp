/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#if !(_MSC_VER >= 1400) //if not visual studio 2005 or later
    #define _WIN32_WINNT 0x501
#endif

#ifdef __WIN32__
	#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
	#endif
	#include <windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else //Use BSD sockets
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <errno.h>
	#define SOCKET_ERROR (-1)
	#define INVALID_SOCKET (SOCKET)(~0)
	#define NO_ERROR 0L
#endif

#include <GL/glfw.h>
#include "../include/sgct/SGCTNetwork.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/Engine.h"
#include <stdlib.h>
#include <stdio.h>

void GLFWCALL communicationHandler(void *arg);
void GLFWCALL connectionHandler(void *arg);

#define MAX_NUMBER_OF_ATTEMPS 10

sgct_core::SGCTNetwork::SGCTNetwork()
{
	mCommThreadId		= -1;
	mMainThreadId		= -1;
	mSocket				= INVALID_SOCKET;
	mListenSocket		= INVALID_SOCKET;
#if (_MSC_VER >= 1700) //visual studio 2012 or later
	mDecoderCallbackFn	= nullptr;
	mUpdateCallbackFn	= nullptr;
	mConnectedCallbackFn = nullptr;
#else
	mDecoderCallbackFn	= NULL;
	mUpdateCallbackFn	= NULL;
	mConnectedCallbackFn = NULL;
#endif
	mServerType			= SyncServer;
	mBufferSize			= 1024;
	mRequestedSize		= mBufferSize;
	mSendFrame			= 0;
	mRecvFrame[0]		= 0;
	mRecvFrame[1]		= 0;
	mConnected			= false;
	mTerminate          = false;
	mId					= -1;
	mConnectionMutex	= NULL;
	mDoneCond			= NULL;
}

void sgct_core::SGCTNetwork::init(const std::string port, const std::string ip, bool _isServer, int id, int serverType)
{
	mServer = _isServer;
	mServerType = serverType;
	mBufferSize = sgct::SharedData::Instance()->getBufferSize();
	mId = id;

	mConnectionMutex = sgct::Engine::createMutex();
	mDoneCond = sgct::Engine::createCondition();
	mStartConnectionCond = sgct::Engine::createCondition();
    if(mConnectionMutex == NULL ||
		mDoneCond == NULL ||
		mStartConnectionCond == NULL )
        throw "Failed to create connection mutex & conditions.";

	struct addrinfo *result = NULL, *ptr = NULL, hints;
#ifdef __WIN32__ //WinSock
	ZeroMemory(&hints, sizeof (hints));
#else
	memset(&hints, 0, sizeof (hints));
#endif
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
		//WSACleanup(); hanteras i manager
		throw "Failed to parse hints for connection.";
	}

	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr=result;

	if( mServer )
	{
		// Create a SOCKET for the server to listen for client connections
		mListenSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (mListenSocket == INVALID_SOCKET)
		{
			freeaddrinfo(result);
			throw "Failed to listen init socket!";
		}

		setOptions( &mListenSocket );

		// Setup the TCP listening socket
		iResult = bind( mListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			freeaddrinfo(result);
#ifdef __WIN32__
			closesocket(mListenSocket);
#else
			close(mListenSocket);
#endif
			throw "Bind socket failed!";
		}

		if( listen( mListenSocket, SOMAXCONN ) == SOCKET_ERROR )
		{
            freeaddrinfo(result);
#ifdef __WIN32__
			closesocket(mListenSocket);
#else
			close(mListenSocket);
#endif
			throw "Listen failed!";
		}
	}
	else
	{
		//Client socket

		// Connect to server.
		while( true )
		{
			sgct::MessageHandler::Instance()->print("Attempting to connect to server...\n");

			mSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
			if (mSocket == INVALID_SOCKET)
            {
                freeaddrinfo(result);
                throw "Failed to init client socket!";
            }

            setOptions( &mSocket );

			iResult = connect( mSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult != SOCKET_ERROR)
				break;

			glfwSleep(1.0); //wait for next attempt
		}
	}

	freeaddrinfo(result);
	mMainThreadId = glfwCreateThread( connectionHandler, this );
	if( mMainThreadId < 0)
	{
#ifdef __WIN32__
		mServer ? closesocket(mListenSocket) : closesocket(mSocket);
#else
		mServer ? close(mListenSocket) : close(mSocket);
#endif
		throw "Failed to start main network thread!";
	}
	/*else
        sgct::MessageHandler::Instance()->print("Main thread created, id=%d\n", mMainThreadId);*/
}

void GLFWCALL connectionHandler(void *arg)
{
	sgct_core::SGCTNetwork * nPtr = (sgct_core::SGCTNetwork *)arg;

	if( nPtr->isServer() )
	{
		while( !nPtr->isTerminated() )
		{
			if( !nPtr->isConnected() )
			{
                //first time the thread is -1 so the wait will not run
				if( nPtr->mCommThreadId != -1 )
				{
				    //wait for the connection to disconnect
					glfwWaitThread( nPtr->mCommThreadId, GLFW_WAIT );
                    //reset thread handle/id
				    nPtr->mCommThreadId = -1;
				}

				//start a new connection enabling the client to reconnect
				nPtr->mCommThreadId = glfwCreateThread( communicationHandler, nPtr );
				if( nPtr->mCommThreadId < 0)
				{
					return;
				}
				/*else
                    sgct::MessageHandler::Instance()->print("Comm thread created, id=%d\n", nPtr->mCommThreadId);*/
			}
			//wait for signal until next iteration in loop
            if( !nPtr->isTerminated() )
            {
                sgct::Engine::lockMutex(nPtr->mConnectionMutex);
                    sgct::Engine::waitCond( nPtr->mStartConnectionCond,
                        nPtr->mConnectionMutex,
                        GLFW_INFINITY );
                sgct::Engine::unlockMutex(nPtr->mConnectionMutex);
            }
		}
	}
	else //if client
	{
		nPtr->mCommThreadId = glfwCreateThread( communicationHandler, nPtr );

		if( nPtr->mCommThreadId < 0)
		{
			return;
		}
		/*else
            sgct::MessageHandler::Instance()->print("Comm thread created, id=%d\n", nPtr->mCommThreadId);*/
	}

	sgct::MessageHandler::Instance()->print("Closing connection handler for connection %d... \n", nPtr->getId());
}

void sgct_core::SGCTNetwork::setOptions(SOCKET * socketPtr)
{
	if(socketPtr != NULL)
	{
		int flag = 1;

		//set no delay, disable nagle's algorithm
		int iResult = setsockopt(*socketPtr, /* socket affected */
		IPPROTO_TCP,     /* set option at TCP level */
		TCP_NODELAY,     /* name of option */
		(char *) &flag,  /* the cast is historical cruft */
		sizeof(int));    /* length of option value */

        if( iResult != NO_ERROR )
            sgct::MessageHandler::Instance()->print("Failed to set no delay with error: %d\nThis will reduce cluster performance!", errno);

		//set timeout
		int timeout = 0; //infinite
        iResult = setsockopt(
                *socketPtr,
                SOL_SOCKET,
                SO_SNDTIMEO,
                (char *)&timeout,
                sizeof(timeout));

		//iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_DONTLINGER, (char*)&flag, sizeof(int));
        iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));
        iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int));
	}
}

void sgct_core::SGCTNetwork::closeSocket(SOCKET lSocket)
{
    if( lSocket != INVALID_SOCKET )
	{
	    /*
		Windows shutdown options
            * SD_RECIEVE
            * SD_SEND
            * SD_BOTH

        Linux & Mac shutdown options
            * SHUT_RD (Disables further receive operations)
            * SHUT_WR (Disables further send operations)
            * SHUT_RDWR (Disables further send and receive operations)
		*/

        sgct::Engine::lockMutex(mConnectionMutex);
#ifdef __WIN32__
        shutdown(lSocket, SD_BOTH);
		closesocket( lSocket );
#else
		shutdown(lSocket, SHUT_RDWR);
		close( lSocket );
#endif

		lSocket = INVALID_SOCKET;
		sgct::Engine::unlockMutex(mConnectionMutex);
	}
}

void sgct_core::SGCTNetwork::setBufferSize(unsigned int newSize)
{
	mRequestedSize = newSize;
}

void sgct_core::SGCTNetwork::iterateFrameCounter()
{
	if( mSendFrame < MAX_NET_SYNC_FRAME_NUMBER )
		mSendFrame++;
	else
		mSendFrame = 0;
}

void sgct_core::SGCTNetwork::pushClientMessage()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::pushClientMessage\n");

	//The servers' render function is locked until a message starting with the ack-byte is received.
	int currentFrame = getSendFrame();
	unsigned char *p = (unsigned char *)&currentFrame;

    if(sgct::MessageHandler::Instance()->getDataSize() > mHeaderSize)
    {
        //Don't remove this pointer, somehow the send function doesn't
		//work during the first call without setting the pointer first!!!
		char * messageToSend = sgct::MessageHandler::Instance()->getMessage();
		messageToSend[0] = SGCTNetwork::SyncByte;
		messageToSend[1] = p[0];
		messageToSend[2] = p[1];
		messageToSend[3] = p[2];
		messageToSend[4] = p[3];

		unsigned int currentMessageSize =
			sgct::MessageHandler::Instance()->getDataSize() > mBufferSize ?
			mBufferSize :
			sgct::MessageHandler::Instance()->getDataSize();

        unsigned int dataSize = currentMessageSize - mHeaderSize;
		unsigned char *currentMessageSizePtr = (unsigned char *)&dataSize;
		messageToSend[5] = currentMessageSizePtr[0];
		messageToSend[6] = currentMessageSizePtr[1];
		messageToSend[7] = currentMessageSizePtr[2];
		messageToSend[8] = currentMessageSizePtr[3];

		//crop if needed
		if( sendData((void*)messageToSend, currentMessageSize) == SOCKET_ERROR )
            sgct::MessageHandler::Instance()->print("Failed to send sync header + client log message!\n");

		sgct::MessageHandler::Instance()->clearBuffer(); //clear the buffer
    }
	else
	{
		char tmpca[mHeaderSize];
		tmpca[0] = SGCTNetwork::SyncByte;
		tmpca[1] = p[0];
		tmpca[2] = p[1];
		tmpca[3] = p[2];
		tmpca[4] = p[3];

		unsigned int localSyncHeaderSize = 0;
		unsigned char *currentMessageSizePtr = (unsigned char *)&localSyncHeaderSize;
		tmpca[5] = currentMessageSizePtr[0];
		tmpca[6] = currentMessageSizePtr[1];
		tmpca[7] = currentMessageSizePtr[2];
		tmpca[8] = currentMessageSizePtr[3];

		if( sendData((void *)tmpca, mHeaderSize) == SOCKET_ERROR )
            sgct::MessageHandler::Instance()->print("Failed to send sync header!\n");
	}
}

int sgct_core::SGCTNetwork::getSendFrame()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::getSendFrame\n");
	int tmpi;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpi = mSendFrame;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpi;
}

bool sgct_core::SGCTNetwork::compareFrames()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::compareFrames\n");
	bool tmpb;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpb = (mRecvFrame[0] == mRecvFrame[1]);
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpb;
}

void sgct_core::SGCTNetwork::swapFrames()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::swapFrames\n");
	sgct::Engine::lockMutex(mConnectionMutex);
		mRecvFrame[1] = mRecvFrame[0];
	sgct::Engine::unlockMutex(mConnectionMutex);
}

void sgct_core::SGCTNetwork::setDecodeFunction(sgct_cppxeleven::function<void (const char*, int, int)> callback)
{
	mDecoderCallbackFn = callback;
}

void sgct_core::SGCTNetwork::setUpdateFunction(sgct_cppxeleven::function<void (int)> callback)
{
	mUpdateCallbackFn = callback;
}

void sgct_core::SGCTNetwork::setConnectedFunction(sgct_cppxeleven::function<void (void)> callback)
{
	mConnectedCallbackFn = callback;
}

void sgct_core::SGCTNetwork::setConnectedStatus(bool state)
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::setConnectedStatus = %s at syncframe %d\n", state ? "true" : "false", getSendFrame());
	sgct::Engine::lockMutex(mConnectionMutex);
		mConnected = state;
	sgct::Engine::unlockMutex(mConnectionMutex);
}

bool sgct_core::SGCTNetwork::isConnected()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::isConnected\n");
	bool tmpb;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpb = mConnected;
	sgct::Engine::unlockMutex(mConnectionMutex);
    return tmpb;
}

int sgct_core::SGCTNetwork::getTypeOfServer()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::getTypeOfServer\n");
	int tmpi;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpi = mServerType;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpi;
}

int sgct_core::SGCTNetwork::getId()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::getId\n");
	int tmpi;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpi = mId;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpi;
}

bool sgct_core::SGCTNetwork::isServer()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::isServer\n");
	bool tmpb;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpb = mServer;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpb;
}

bool sgct_core::SGCTNetwork::isTerminated()
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::isTerminated\n");
    bool tmpb;
	sgct::Engine::lockMutex(mConnectionMutex);
		tmpb = mTerminate;
	sgct::Engine::unlockMutex(mConnectionMutex);
	return tmpb;
}

void sgct_core::SGCTNetwork::setRecvFrame(int i)
{
    sgct::MessageHandler::Instance()->printDebug("SGCTNetwork::setRecvFrame\n");
	sgct::Engine::lockMutex(mConnectionMutex);
	mRecvFrame[0] = i;
	sgct::Engine::unlockMutex(mConnectionMutex);
}

int sgct_core::SGCTNetwork::receiveData(SOCKET & lsocket, char * buffer, int length, int flags)
{
    int iResult = 0;
    int attempts = 1;

    while( iResult < length )
    {
        int tmpRes = recv( lsocket, buffer + iResult, length - iResult, flags);
#ifdef __SGCT_NETWORK_DEBUG__
        sgct::MessageHandler::Instance()->print("Received %d bytes of %d...\n", tmpRes, length);
        for(int i=0; i<tmpRes; i++)
            sgct::MessageHandler::Instance()->print("%u\t", buffer[i]);
        sgct::MessageHandler::Instance()->print("\n");
#endif

        if( tmpRes > 0 )
            iResult += tmpRes;
#ifdef __WIN32__
        else if( errno == WSAEINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#else
        else if( errno == EINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#endif
        {
            sgct::MessageHandler::Instance()->print("Receiving data after interrupted system error (attempt %d)...\n", attempts);
            //iResult = 0;
            attempts++;
        }
        else
        {
            //capture error
            iResult = tmpRes;
            break;
        }
    }

    return iResult;
}

int sgct_core::SGCTNetwork::parseInt(char * str)
{
    union
    {
        int i;
        char c[4];
    } ci;

    ci.c[0] = str[0];
    ci.c[1] = str[1];
    ci.c[2] = str[2];
    ci.c[3] = str[3];

    return ci.i;
}

unsigned int sgct_core::SGCTNetwork::parseUnsignedInt(char * str)
{
    union
    {
        unsigned int ui;
        char c[4];
    } cui;

    cui.c[0] = str[0];
    cui.c[1] = str[1];
    cui.c[2] = str[2];
    cui.c[3] = str[3];

    return cui.ui;
}

/*
function to decode messages
*/
void GLFWCALL communicationHandler(void *arg)
{
	sgct_core::SGCTNetwork * nPtr = (sgct_core::SGCTNetwork *)arg;

	//exit if terminating
	if( nPtr->isTerminated() )
        return;

	//listen for client if server
	if( nPtr->isServer() )
	{
		sgct::MessageHandler::Instance()->print("Waiting for client to connect to connection %d...\n", nPtr->getId());

        nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);

#ifdef __WIN32__
        int accErr = WSAGetLastError();
        while( !nPtr->isTerminated() && nPtr->mSocket == INVALID_SOCKET && accErr == WSAEINTR)
#else
        int accErr = errno;
        while( !nPtr->isTerminated() && nPtr->mSocket == INVALID_SOCKET && accErr == EINTR)
#endif
		{
		    sgct::MessageHandler::Instance()->print("Re-accept after interrupted system on connection %d...\n", nPtr->getId());

		    nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);
		}

		if (nPtr->mSocket == INVALID_SOCKET)
		{
            sgct::MessageHandler::Instance()->printDebug("Accept connection %d failed! Error: %d\n", nPtr->getId(), accErr);
#if (_MSC_VER >= 1700) //visual studio 2012 or later
			if(nPtr->mUpdateCallbackFn != nullptr)
#else
			if(nPtr->mUpdateCallbackFn != NULL)
#endif
                nPtr->mUpdateCallbackFn( nPtr->getId() );
			return;
		}
	}

	nPtr->setConnectedStatus(true);
	sgct::MessageHandler::Instance()->print("Connection %d established!\n", nPtr->getId());

#if (_MSC_VER >= 1700) //visual studio 2012 or later
	if(nPtr->mUpdateCallbackFn != nullptr)
#else
	if(nPtr->mUpdateCallbackFn != NULL)
#endif
		nPtr->mUpdateCallbackFn( nPtr->getId() );

	//init buffers
	char recvHeader[sgct_core::SGCTNetwork::mHeaderSize];
	memset(recvHeader, sgct_core::SGCTNetwork::FillByte, sgct_core::SGCTNetwork::mHeaderSize);
	char * recvBuf = NULL;

	//recvbuf = reinterpret_cast<char *>( malloc(nPtr->mBufferSize) );
	recvBuf = new (std::nothrow) char[nPtr->mBufferSize];
	std::string extBuffer; //for external comm

	// Receive data until the server closes the connection
	int iResult = 0;
	do
	{
		//resize buffer request
		if( nPtr->mRequestedSize > nPtr->mBufferSize )
		{
                sgct::MessageHandler::Instance()->printDebug("Re-sizing tcp buffer size from %d to %d... ", nPtr->mBufferSize, nPtr->mRequestedSize);

            sgct::MessageHandler::Instance()->print("Network: New package size is %d\n", nPtr->mRequestedSize);
			sgct::Engine::lockMutex(nPtr->mConnectionMutex);
				nPtr->mBufferSize = nPtr->mRequestedSize;

				//clean up
                delete [] recvBuf;
                recvBuf = NULL;

                //allocate
                bool allocError = false;
                recvBuf = new (std::nothrow) char[nPtr->mRequestedSize];
                if(recvBuf == NULL)
                    allocError = true;

			sgct::Engine::unlockMutex(nPtr->mConnectionMutex);

			if(allocError)
				sgct::MessageHandler::Instance()->print("Network error: Buffer failed to resize!\n");
			else
				sgct::MessageHandler::Instance()->print("Network: Buffer resized successfully!\n");

                sgct::MessageHandler::Instance()->printDebug("Done.\n");
		}

        sgct::MessageHandler::Instance()->printDebug("Receiving message header...\n");

        /*
            Get & parse the message header if not external control
        */
        int syncFrameNumber = -1;
        unsigned int dataSize = 0;
        unsigned char packageId = sgct_core::SGCTNetwork::FillByte;

        if( nPtr->getTypeOfServer() != sgct_core::SGCTNetwork::ExternalControl )
        {
            iResult = sgct_core::SGCTNetwork::receiveData(nPtr->mSocket,
                               recvHeader,
                               static_cast<int>(sgct_core::SGCTNetwork::mHeaderSize),
                               0);
        }

#ifdef __SGCT_NETWORK_DEBUG__
        sgct::MessageHandler::Instance()->print("Header: %u | %u %u %u %u | %u %u %u %u\n",
                                                recvHeader[0],
                                                recvHeader[1],
                                                recvHeader[2],
                                                recvHeader[3],
                                                recvHeader[4],
                                                recvHeader[5],
                                                recvHeader[6],
                                                recvHeader[7],
                                                recvHeader[8]);
#endif

        if( iResult == static_cast<int>(sgct_core::SGCTNetwork::mHeaderSize))
        {
            packageId = recvHeader[0];
            sgct::MessageHandler::Instance()->printDebug("Package id=%d...\n", packageId);
            if( packageId == sgct_core::SGCTNetwork::SyncByte )
            {
                //parse the sync frame number
                syncFrameNumber = sgct_core::SGCTNetwork::parseInt(&recvHeader[1]);
                //parse the data size
                dataSize = sgct_core::SGCTNetwork::parseUnsignedInt(&recvHeader[5]);

                //resize buffer if needed
                sgct::Engine::lockMutex(nPtr->mConnectionMutex);
                if( dataSize > nPtr->mBufferSize )
                {
                    //clean up
                    delete [] recvBuf;
                    recvBuf = NULL;

                    //allocate
                    recvBuf = new (std::nothrow) char[dataSize];
                    if(recvBuf != NULL)
                    {
                        nPtr->mBufferSize = dataSize;
                    }
                }

                sgct::Engine::unlockMutex(nPtr->mConnectionMutex);
            }
        }


        sgct::MessageHandler::Instance()->printDebug("Receiving data (buffer size: %d)...\n", dataSize);
        /*
            Get the data/message
        */
        if( dataSize > 0 )
        {
			iResult = sgct_core::SGCTNetwork::receiveData(nPtr->mSocket,
                                        recvBuf,
                                        dataSize,
                                        0);
        sgct::MessageHandler::Instance()->printDebug("Data type: %d, %d bytes of %u...\n", packageId, iResult, dataSize);
        }

		/*
			Get external message
		*/
		if( nPtr->getTypeOfServer() == sgct_core::SGCTNetwork::ExternalControl )
		{
			//do a normal read
			iResult = recv( nPtr->mSocket,
                                        recvBuf,
                                        nPtr->mBufferSize,
                                        0);

			//if read fails try for x attempts
			int attempts = 1;
#ifdef __WIN32__
			while( iResult <= 0 && errno == WSAEINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#else
			while( iResult <= 0 && errno == EINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#endif
			{
				iResult = recv( nPtr->mSocket,
                                        recvBuf,
                                        nPtr->mBufferSize,
                                        0);

				sgct::MessageHandler::Instance()->print("Receiving data after interrupted system error (attempt %d)...\n", attempts);
				attempts++;
			}
		}

		if (iResult > 0)
		{
            //game over message
			if( packageId == sgct_core::SGCTNetwork::DisconnectByte &&
                recvHeader[1] == 24 &&
                recvHeader[2] == '\r' &&
                recvHeader[3] == '\n' &&
                recvHeader[4] == 27 &&
                recvHeader[5] == '\r' &&
                recvHeader[6] == '\n' &&
                recvHeader[7] == '\0' )
            {
                nPtr->setConnectedStatus(false);

                /*
                    Terminate client only. The server only resets the connection,
                    allowing clients to connect.
                */
                if( !nPtr->isServer() )
                {
                    sgct::Engine::lockMutex(nPtr->mConnectionMutex);
                        nPtr->mTerminate = true;
                    sgct::Engine::unlockMutex(nPtr->mConnectionMutex);
                }

				sgct::MessageHandler::Instance()->print("Network: Client %d terminated connection.\n", nPtr->getId());

                break; //exit loop
            }
			else if( nPtr->getTypeOfServer() == sgct_core::SGCTNetwork::SyncServer )
			{
#if (_MSC_VER >= 1700) //visual studio 2012 or later
				if( packageId == sgct_core::SGCTNetwork::SyncByte &&
					nPtr->mDecoderCallbackFn != nullptr)
#else
				if( packageId == sgct_core::SGCTNetwork::SyncByte &&
					nPtr->mDecoderCallbackFn != NULL)
#endif
				{
					nPtr->setRecvFrame( syncFrameNumber );
					if( syncFrameNumber < 0 )
					{
						sgct::MessageHandler::Instance()->print("Network: Error sync in sync frame: %d for connection %d\n", syncFrameNumber, nPtr->getId());
					}

                    sgct::MessageHandler::Instance()->printDebug("Package info: Frame = %d, Size = %u\n", syncFrameNumber, dataSize);

                    //decode callback
                    if(dataSize > 0)
                        (nPtr->mDecoderCallbackFn)(recvBuf, dataSize, nPtr->getId());

					sgct::Engine::signalCond( sgct_core::NetworkManager::gCond );
                    sgct::MessageHandler::Instance()->printDebug("Done.\n");
				}
#if (_MSC_VER >= 1700) //visual studio 2012 or later
				else if( packageId == sgct_core::SGCTNetwork::ConnectedByte &&
					nPtr->mConnectedCallbackFn != nullptr)
#else
				else if( packageId == sgct_core::SGCTNetwork::ConnectedByte &&
					nPtr->mConnectedCallbackFn != NULL)
#endif
				{
                    sgct::MessageHandler::Instance()->printDebug("Signaling slave is connected... ");
					(nPtr->mConnectedCallbackFn)();
					sgct::Engine::signalCond( sgct_core::NetworkManager::gCond );
                    sgct::MessageHandler::Instance()->printDebug("Done.\n");
				}
			}
			else if(nPtr->getTypeOfServer() == sgct_core::SGCTNetwork::ExternalControl)
			{
                sgct::MessageHandler::Instance()->printDebug("Parsing external TCP data... ");
				std::string tmpStr(recvBuf);
				extBuffer += tmpStr.substr(0, iResult);

                bool breakConnection = false;

				//look for cancel
				std::size_t found = extBuffer.find(24); //cancel
				if( found != std::string::npos )
				{
				    breakConnection = true;
				}
				//look for escape
				found = extBuffer.find(27); //escape
				if( found != std::string::npos )
				{
				    breakConnection = true;
				}
				//look for logout
				found = extBuffer.find("logout");
				if( found != std::string::npos )
				{
				    breakConnection = true;
				}
				//look for close
				found = extBuffer.find("close");
				if( found != std::string::npos )
				{
				    breakConnection = true;
				}
				//look for exit
				found = extBuffer.find("exit");
				if( found != std::string::npos )
				{
				    breakConnection = true;
				}
				//look for quit
				found = extBuffer.find("quit");
				if( found != std::string::npos )
				{
				    breakConnection = true;
				}

				if(breakConnection)
				{
				    nPtr->setConnectedStatus(false);
				    break;
				}

                //separate messages by <CR><NL>
				found = extBuffer.find("\r\n");
				while( found != std::string::npos )
				{
					std::string extMessage = extBuffer.substr(0,found);
					//extracted message
					//fprintf(stderr, "Extracted: '%s'\n", extMessage.c_str());

					sgct::Engine::lockMutex(sgct_core::NetworkManager::gMutex);
						extBuffer = extBuffer.substr(found+2);//jump over \r\n
#if (_MSC_VER >= 1700) //visual studio 2012 or later
						if( nPtr->mDecoderCallbackFn != nullptr )
#else
						if( nPtr->mDecoderCallbackFn != NULL )
#endif
						{
							(nPtr->mDecoderCallbackFn)(extMessage.c_str(), extMessage.size(), nPtr->getId());
						}
					sgct::Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

					//reply
					nPtr->sendStr("OK\r\n");
                    found = extBuffer.find("\r\n");
				}
                sgct::MessageHandler::Instance()->printDebug("Done.\n");
			}
		}
		else if (iResult == 0)
		{
            sgct::MessageHandler::Instance()->printDebug("Setting connection status to false... ");
			nPtr->setConnectedStatus(false);
            sgct::MessageHandler::Instance()->printDebug("Done.\n");

#ifdef __WIN32__
			sgct::MessageHandler::Instance()->print("TCP Connection %d closed (error: %d)\n", nPtr->getId(), WSAGetLastError());
#else
            sgct::MessageHandler::Instance()->print("TCP Connection %d closed (error: %d)\n", nPtr->getId(), errno);
#endif
		}
		else
		{
            sgct::MessageHandler::Instance()->printDebug("Setting connection status to false... ");
			nPtr->setConnectedStatus(false);
            sgct::MessageHandler::Instance()->printDebug("Done.\n");

#ifdef __WIN32__
            sgct::MessageHandler::Instance()->print("TCP connection %d recv failed: %d\n", nPtr->getId(), WSAGetLastError());
#else
            sgct::MessageHandler::Instance()->print("TCP connection %d recv failed: %d\n", nPtr->getId(), errno);
#endif
		}

	} while (iResult > 0 || nPtr->isConnected());


	//cleanup
	sgct::Engine::lockMutex(nPtr->mConnectionMutex);
	if( recvBuf != NULL )
    {
        //free(recvbuf);
        delete [] recvBuf;
        recvBuf = NULL;
    }
    sgct::Engine::unlockMutex(nPtr->mConnectionMutex);

    //Close socket
    //contains mutex
	nPtr->closeSocket( nPtr->mSocket );

#if (_MSC_VER >= 1700) //visual studio 2012 or later
	if(nPtr->mUpdateCallbackFn != nullptr)
#else
	if(nPtr->mUpdateCallbackFn != NULL)
#endif
		nPtr->mUpdateCallbackFn( nPtr->getId() );

	sgct::MessageHandler::Instance()->print("Node %d disconnected!\n", nPtr->getId());
}

int sgct_core::SGCTNetwork::sendData(void * data, int length)
{
	//fprintf(stderr, "Send data size: %d\n", length);
#ifdef __SGCT_NETWORK_DEBUG__
	for(int i=0; i<length; i++)
        fprintf(stderr, "%u ", ((const char *)data)[i]);
    fprintf(stderr, "\n");
#endif
	return send(mSocket, (const char *)data, length, 0);
}

int sgct_core::SGCTNetwork::sendStr(std::string msg)
{
	//fprintf(stderr, "Send message: %s, size: %d\n", msg.c_str(), msg.size());
	return send(mSocket, msg.c_str(), msg.size(), 0);
}

void sgct_core::SGCTNetwork::closeNetwork(bool forced)
{
    if( mCommThreadId != -1 )
    {
        if( forced )
            glfwDestroyThread(mCommThreadId); //blocking sockets -> cannot wait for thread so just kill it brutally
        else
            glfwWaitThread(mCommThreadId, GLFW_WAIT);
    }

    if( mMainThreadId != -1 )
	{
        if( forced )
            glfwDestroyThread( mMainThreadId );
        else
            glfwWaitThread(mMainThreadId, GLFW_WAIT);
    }

	//wait for everything to really close!
	//Otherwise the mutex can be called after
	//it is destroyed.
	sgct::Engine::lockMutex(mConnectionMutex);
    sgct::Engine::waitCond( mDoneCond,
        mConnectionMutex,
        0.25 );
    sgct::Engine::unlockMutex(mConnectionMutex);

    if( mConnectionMutex != NULL )
	{
		sgct::Engine::destroyMutex(mConnectionMutex);
		mConnectionMutex = NULL;
	}

	if( mStartConnectionCond != NULL )
	{
		sgct::Engine::destroyCond(mStartConnectionCond);
		mStartConnectionCond = NULL;
	}

	if( mDoneCond != NULL )
	{
		sgct::Engine::destroyCond(mDoneCond);
		mDoneCond = NULL;
	}

	sgct::MessageHandler::Instance()->print("Connection %d successfully terminated.\n", mId);
}

void sgct_core::SGCTNetwork::initShutdown()
{
	if( isConnected() )
	{
        char gameOver[9];
		gameOver[0] = DisconnectByte;
        gameOver[1] = 24; //ASCII for cancel
        gameOver[2] = '\r';
        gameOver[3] = '\n';
        gameOver[4] = 27; //ASCII for Esc
        gameOver[5] = '\r';
        gameOver[6] = '\n';
        gameOver[7] = '\0';
		gameOver[8] = FillByte;
        sendData(gameOver, mHeaderSize);
	}

	sgct::MessageHandler::Instance()->print("Closing connection %d... \n", getId());
	sgct::Engine::lockMutex(mConnectionMutex);
        mTerminate = true;
#if (_MSC_VER >= 1700) //visual studio 2012 or later
        mDecoderCallbackFn = nullptr;
#else
		 mDecoderCallbackFn = NULL;
#endif
        mConnected = false;
	sgct::Engine::unlockMutex(mConnectionMutex);

	//wake up the connection handler thread (in order to finish)
	if( isServer() )
	{
		sgct::Engine::signalCond( mStartConnectionCond );
	}

    closeSocket( mSocket );
    closeSocket( mListenSocket );
}
