/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
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
	#define SGCT_ERRNO WSAGetLastError()
#else //Use BSD sockets
    #ifdef _XCODE
        #include <unistd.h>
    #endif
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netinet/tcp.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <errno.h>
	#define SOCKET_ERROR (-1)
	#define INVALID_SOCKET (SGCT_SOCKET)(~0)
	#define NO_ERROR 0L
	#define SGCT_ERRNO errno
#endif

#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/Engine.h"
#include <stdlib.h>
#include <stdio.h>

void communicationHandler(void *arg);
void connectionHandler(void *arg);

#define MAX_NUMBER_OF_ATTEMPS 10
#define SGCT_SOCKET_BUFFER_SIZE 4096

sgct_core::SGCTNetwork::SGCTNetwork()
{
	mCommThread		= NULL;
	mMainThread		= NULL;
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
	mConnectionType		= SyncConnection;
	mBufferSize			= 1024;
	mRequestedSize		= mBufferSize;
	mSendFrame[Current]	= 0;
	mSendFrame[Previous]= 0;
	mRecvFrame[Current]	= 0;
	mRecvFrame[Previous]= -1;
	mTimeStamp[Send]	= 0.0;
	mTimeStamp[Total]	= 0.0;

	mFirmSync			= false;
	mUpdated			= false;
	mConnected			= false;
	mTerminate          = false;
	mId					= -1;
}

/*!
	Inits this network connection.

	\param port is the network port (TCP)
	\param address is the hostname, ip4 address or ip6 address
	\param _isServer indicates if this connection is a server or client
	\param id is a unique id of this connection
	\param connectionType is the type of connection
	\param firmSync if set to true then firm framesync will be used for the whole cluster
*/
void sgct_core::SGCTNetwork::init(const std::string port, const std::string address, bool _isServer, int id, sgct_core::SGCTNetwork::ConnectionTypes connectionType, bool firmSync)
{
	mServer = _isServer;
	mConnectionType = connectionType;
	mBufferSize = static_cast<int>(sgct::SharedData::instance()->getBufferSize());
	mId = id;
	mFirmSync = firmSync;

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
		iResult = getaddrinfo(address.c_str(), port.c_str(), &hints, &result);
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
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Attempting to connect to server...\n");

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

			tthread::this_thread::sleep_for(tthread::chrono::seconds(1));; //wait for next attempt
		}
	}

	freeaddrinfo(result);
	mMainThread = new tthread::thread( connectionHandler, this );
}

void connectionHandler(void *arg)
{
	sgct_core::SGCTNetwork * nPtr = (sgct_core::SGCTNetwork *)arg;

	if( nPtr->isServer() )
	{
		while( !nPtr->isTerminated() )
		{
			if( !nPtr->isConnected() )
			{
                //first time the thread is NULL so the wait will not run
				if( nPtr->mCommThread != NULL )
				{
				    nPtr->mCommThread->join();
					delete nPtr->mCommThread;
				    nPtr->mCommThread = NULL;
				}

				//start a new connection enabling the client to reconnect
				nPtr->mCommThread = new tthread::thread( communicationHandler, nPtr );
			}
			//wait for signal until next iteration in loop
            if( !nPtr->isTerminated() )
            {
                #ifdef __SGCT_MUTEX_DEBUG__
					fprintf(stderr, "Locking mutex for connection %d...\n", nPtr->getId());
				#endif
				nPtr->mConnectionMutex.lock(); 
				nPtr->mStartConnectionCond.wait( nPtr->mConnectionMutex );
				nPtr->mConnectionMutex.unlock();
				#ifdef __SGCT_MUTEX_DEBUG__
					fprintf(stderr, "Mutex for connection %d is unlocked.\n", nPtr->getId());
				#endif
            }
		}
	}
	else //if client
	{
		nPtr->mCommThread = new tthread::thread( communicationHandler, nPtr );
	}

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Exiting connection handler for connection %d... \n", nPtr->getId());
}

void sgct_core::SGCTNetwork::setOptions(SGCT_SOCKET * socketPtr)
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
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTNetwork: Failed to set no delay with error: %d\nThis will reduce cluster performance!", SGCT_ERRNO);

		//set timeout
		int timeout = 0; //infinite
        iResult = setsockopt(
                *socketPtr,
                SOL_SOCKET,
                SO_SNDTIMEO,
                (char *)&timeout,
                sizeof(timeout));

		iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));
        if (iResult == SOCKET_ERROR)
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "SGCTNetwork: Failed to set reuse address with error: %d\n!", SGCT_ERRNO);

		/*
			The default buffer value is 8k (8192 bytes) which is good for external control
			but might be a bit to big for sync data.
		*/
		if( getTypeOfConnection() == sgct_core::SGCTNetwork::SyncConnection )
		{
			int bufferSize = SGCT_SOCKET_BUFFER_SIZE;
			iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(int));
			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTNetwork: Failed to set send buffer size to %d with error: %d\n!", bufferSize, SGCT_ERRNO);
			iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof(int));
			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTNetwork: Failed to set receive buffer size to %d with error: %d\n!", bufferSize, SGCT_ERRNO);
		}
		//set only on external control, cluster nodes sends data several times per second so there is no need so send alive packages
		else
		{
			iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int));
			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "SGCTNetwork: Failed to set keep alive with error: %d\n!", SGCT_ERRNO);
		}
	}
}

void sgct_core::SGCTNetwork::closeSocket(SGCT_SOCKET lSocket)
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

		#ifdef __SGCT_MUTEX_DEBUG__
			fprintf(stderr, "Locking mutex for connection %d...\n", mId);
		#endif
		mConnectionMutex.lock(); 
        
#ifdef __WIN32__
        shutdown(lSocket, SD_BOTH);
		closesocket( lSocket );
#else
		shutdown(lSocket, SHUT_RDWR);
		close( lSocket );
#endif

		lSocket = INVALID_SOCKET;
		mConnectionMutex.unlock();
		#ifdef __SGCT_MUTEX_DEBUG__
			fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
		#endif
	}
}

void sgct_core::SGCTNetwork::setBufferSize(unsigned int newSize)
{
	mRequestedSize = newSize;
}

/*!
	Iterates the send frame number and returns the new frame number
*/
int sgct_core::SGCTNetwork::iterateFrameCounter()
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::iterateFrameCounter\n");
#endif

	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
	
	mSendFrame[Previous] = mSendFrame[Current];
	
	if( mSendFrame[Current] < MAX_NET_SYNC_FRAME_NUMBER )
		mSendFrame[Current]++;
	else
		mSendFrame[Current] = 0;

	mUpdated = false;

	mTimeStamp[Send] = sgct::Engine::getTime();
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif

	return mSendFrame[Current];
}

/*!
	The client sends ack message to server + console messages
*/
void sgct_core::SGCTNetwork::pushClientMessage()
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::pushClientMessage\n");
#endif

	//The servers' render function is locked until a message starting with the ack-byte is received.
	int currentFrame = iterateFrameCounter();
	unsigned char *p = (unsigned char *)&currentFrame;

    if(sgct::MessageHandler::instance()->getDataSize() > mHeaderSize)
    {
        sgct::SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
		
		//Don't remove this pointer, somehow the send function doesn't
		//work during the first call without setting the pointer first!!!
		char * messageToSend = sgct::MessageHandler::instance()->getMessage();
		messageToSend[0] = SGCTNetwork::SyncByte;
		messageToSend[1] = p[0];
		messageToSend[2] = p[1];
		messageToSend[3] = p[2];
		messageToSend[4] = p[3];

        //crop if needed
		std::size_t currentMessageSize =
			sgct::MessageHandler::instance()->getDataSize() > static_cast<size_t>(mBufferSize) ?
			static_cast<size_t>(mBufferSize) :
			sgct::MessageHandler::instance()->getDataSize();

        std::size_t dataSize = currentMessageSize - mHeaderSize;
		unsigned char *currentMessageSizePtr = (unsigned char *)&dataSize;
		messageToSend[5] = currentMessageSizePtr[0];
		messageToSend[6] = currentMessageSizePtr[1];
		messageToSend[7] = currentMessageSizePtr[2];
		messageToSend[8] = currentMessageSizePtr[3];

		sendData((void*)messageToSend, static_cast<int>(currentMessageSize));
		
		sgct::SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

		sgct::MessageHandler::instance()->clearBuffer(); //clear the buffer
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

		sendData((void *)tmpca, mHeaderSize);
	}
}

int sgct_core::SGCTNetwork::getSendFrame(sgct_core::SGCTNetwork::ReceivedIndex ri)
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::getSendFrame\n");
#endif
	int tmpi;
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
		tmpi = mSendFrame[ri];
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
	return tmpi;
}

int sgct_core::SGCTNetwork::getRecvFrame(sgct_core::SGCTNetwork::ReceivedIndex ri)
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::getRecvFrame\n");
#endif
	int tmpi;
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
		tmpi = mRecvFrame[ri];
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
	return tmpi;
}

/*!
	Get the time in seconds from send to receive of sync data.
*/
double sgct_core::SGCTNetwork::getLoopTime()
{
	#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::getLoopTime\n");
#endif
	double tmpd;
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
		tmpd = mTimeStamp[Total];
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
	return tmpd;
}

/*!
This function compares the received frame number with the sent frame number.

The server starts by sending a frame sync number to the client.
The client receives the sync frame number and sends it back after drawing when ready for buffer swap.
When the server recieves a frame sync number equal to the send frame number it swaps buffers.

\returns true if updates has been received
*/
bool sgct_core::SGCTNetwork::isUpdated()
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::isUpdated\n");
#endif
	bool tmpb = false;
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
		if(mServer)
		{
			tmpb = mFirmSync ? 
				//master sends first -> so on reply they should be equal
				(mRecvFrame[Current] == mSendFrame[Current]) :
				//don't check if loose sync
				true;
		}
		else
		{
			tmpb = mFirmSync ? 
				//slaves receives first and then sends so the prevois should be equal to the send
				(mRecvFrame[Previous] == mSendFrame[Current]) :
				//if loose sync just check if updated
				mUpdated;
		}

	tmpb = (tmpb && mConnected);

	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif

	return tmpb;
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
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::setConnectedStatus = %s at syncframe %d\n", state ? "true" : "false", getSendFrame());
#endif

	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
		mConnected = state;
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
}

bool sgct_core::SGCTNetwork::isConnected()
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::isConnected\n");
#endif
	bool tmpb;
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif

	mConnectionMutex.lock();
		tmpb = mConnected;
	mConnectionMutex.unlock();
	
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
    return tmpb;
}

sgct_core::SGCTNetwork::ConnectionTypes sgct_core::SGCTNetwork::getTypeOfConnection()
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::getTypeOfServer\n");
#endif
	ConnectionTypes tmpct;
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
		tmpct = mConnectionType;
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
	return tmpct;
}

const int sgct_core::SGCTNetwork::getId()
{
#ifdef __SGCT_NETWORK_DEBUG__    
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::getId\n");
#endif

	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	
	int tmpi = 0;
	mConnectionMutex.lock();
		tmpi = mId;
	mConnectionMutex.unlock();
	
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
	
	return tmpi;
}

bool sgct_core::SGCTNetwork::isServer()
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::isServer\n");
#endif
	bool tmpb;
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
		tmpb = mServer;
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
	return tmpb;
}

bool sgct_core::SGCTNetwork::isTerminated()
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::isTerminated\n");
#endif
    bool tmpb;
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
		tmpb = mTerminate;
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
	return tmpb;
}

void sgct_core::SGCTNetwork::setRecvFrame(int i)
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::setRecvFrame\n");
#endif
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
	mRecvFrame[Previous] = mRecvFrame[Current];
	mRecvFrame[Current] = i;

	mTimeStamp[Total] = sgct::Engine::getTime() - mTimeStamp[Send];
	mUpdated = true;
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif
}

ssize_t sgct_core::SGCTNetwork::receiveData(SGCT_SOCKET & lsocket, char * buffer, int length, int flags)
{
    ssize_t iResult = 0;
    int attempts = 1;

    while( iResult < length )
    {
        ssize_t tmpRes = recv( lsocket, buffer + iResult, length - iResult, flags);
#ifdef __SGCT_NETWORK_DEBUG__
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Received %d bytes of %d...\n", tmpRes, length);
        for(int i=0; i<tmpRes; i++)
            sgct::MessageHandler::instance()->print("%u\t", buffer[i]);
        sgct::MessageHandler::instance()->print("\n");
#endif

        if( tmpRes > 0 )
            iResult += tmpRes;
#ifdef __WIN32__
        else if( SGCT_ERRNO == WSAEINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#else
        else if( SGCT_ERRNO == EINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#endif
        {
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Receiving data after interrupted system error (attempt %d)...\n", attempts);
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
void communicationHandler(void *arg)
{
	sgct_core::SGCTNetwork * nPtr = (sgct_core::SGCTNetwork *)arg;

	//exit if terminating
	if( nPtr->isTerminated() )
        return;

	//listen for client if server
	if( nPtr->isServer() )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Waiting for client to connect to connection %d...\n", nPtr->getId());

        nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);

		int accErr = SGCT_ERRNO;
#ifdef __WIN32__
        while( !nPtr->isTerminated() && nPtr->mSocket == INVALID_SOCKET && accErr == WSAEINTR)
#else
        while( !nPtr->isTerminated() && nPtr->mSocket == INVALID_SOCKET && accErr == EINTR)
#endif
		{
		    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Re-accept after interrupted system on connection %d...\n", nPtr->getId());

		    nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);
		}

		if (nPtr->mSocket == INVALID_SOCKET)
		{
            sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_ERROR, "Accept connection %d failed! Error: %d\n", nPtr->getId(), accErr);
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
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Connection %d established!\n", nPtr->getId());

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
	ssize_t iResult = 0;
	do
	{
		//resize buffer request
		if( nPtr->mRequestedSize > nPtr->mBufferSize )
		{
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Re-sizing tcp buffer size from %d to %d... ", nPtr->mBufferSize, nPtr->mRequestedSize);

			#ifdef __SGCT_MUTEX_DEBUG__
				fprintf(stderr, "Locking mutex for connection %d...\n", nPtr->getId());
			#endif
            nPtr->mConnectionMutex.lock();
				nPtr->mBufferSize = nPtr->mRequestedSize;

				//clean up
                delete [] recvBuf;
                recvBuf = NULL;

                //allocate
                bool allocError = false;
                recvBuf = new (std::nothrow) char[nPtr->mRequestedSize];
                if(recvBuf == NULL)
                    allocError = true;

			nPtr->mConnectionMutex.unlock();
			#ifdef __SGCT_MUTEX_DEBUG__
				fprintf(stderr, "Mutex for connection %d is unlocked.\n", nPtr->getId());
			#endif

			if(allocError)
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Network error: Buffer failed to resize!\n");
			else
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Network: Buffer resized successfully!\n");

            sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
		}
#ifdef __SGCT_NETWORK_DEBUG__
        sgct::MessageHandler::instance()->printDebug( sgct::MessageHandler::NOTIFY_ALL, "Receiving message header...\n");
#endif
        /*
            Get & parse the message header if not external control
        */
        int syncFrameNumber = -1;
        int dataSize = 0;
        unsigned char packageId = sgct_core::SGCTNetwork::FillByte;

		/*!
			==========================================================
			      READ SOCKET FOR SYNC DATA
			==========================================================
		*/
        if( nPtr->getTypeOfConnection() == sgct_core::SGCTNetwork::SyncConnection )
        {
            
			iResult = sgct_core::SGCTNetwork::receiveData(nPtr->mSocket,
                               recvHeader,
                               static_cast<int>(sgct_core::SGCTNetwork::mHeaderSize),
                               0);

#ifdef __SGCT_NETWORK_DEBUG__
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Header: %u | %u %u %u %u | %u %u %u %u\n",
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
#ifdef __SGCT_NETWORK_DEBUG__
				sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Package id=%d...\n", packageId);
#endif
				if( packageId == sgct_core::SGCTNetwork::SyncByte )
				{
					//parse the sync frame number
					syncFrameNumber = sgct_core::SGCTNetwork::parseInt(&recvHeader[1]);
					//parse the data size
					dataSize = sgct_core::SGCTNetwork::parseInt(&recvHeader[5]);

					//resize buffer if needed
					#ifdef __SGCT_MUTEX_DEBUG__
						fprintf(stderr, "Locking mutex for connection %d...\n", nPtr->getId());
					#endif
					nPtr->mConnectionMutex.lock();
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

					nPtr->mConnectionMutex.unlock();
					#ifdef __SGCT_MUTEX_DEBUG__
						fprintf(stderr, "Mutex for connection %d is unlocked.\n", nPtr->getId());
					#endif
				}
			}

#ifdef __SGCT_NETWORK_DEBUG__
			sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Receiving data (buffer size: %d)...\n", dataSize);
#endif
			/*
				Get the data/message
			*/
			if( dataSize > 0 )
			{
				iResult = sgct_core::SGCTNetwork::receiveData(nPtr->mSocket,
                                        recvBuf,
                                        dataSize,
                                        0);
#ifdef __SGCT_NETWORK_DEBUG__
				sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Data type: %d, %d bytes of %u...\n", packageId, iResult, dataSize);
#endif
			}
		}

		/*!
			==========================================================
			      READ SOCKET FOR EXTERNAL DATA
			==========================================================
		*/
		else
		{
			//do a normal read
			iResult = recv( nPtr->mSocket,
                                        recvBuf,
                                        nPtr->mBufferSize,
                                        0);

			//if read fails try for x attempts
			int attempts = 1;
#ifdef __WIN32__
			while( iResult <= 0 && SGCT_ERRNO == WSAEINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#else
			while( iResult <= 0 && SGCT_ERRNO == EINTR && attempts <= MAX_NUMBER_OF_ATTEMPS )
#endif
			{
				iResult = recv( nPtr->mSocket,
                                        recvBuf,
                                        nPtr->mBufferSize,
                                        0);

				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Receiving data after interrupted system error (attempt %d)...\n", attempts);
				attempts++;
			}
		}

		/*!
			==========================================================
			      IF DATA SUCCESSFULLY READ THAN DECODE IT 
			==========================================================
		*/
		if (iResult > 0)
		{
			if( nPtr->getTypeOfConnection() == sgct_core::SGCTNetwork::SyncConnection )
			{
				/*
					==========================================
					        HANDLE SYNC DISCONNECTION
					==========================================
				*/
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
						#ifdef __SGCT_MUTEX_DEBUG__
							fprintf(stderr, "Locking mutex for connection %d...\n", nPtr->getId());
						#endif
						nPtr->mConnectionMutex.lock();
						nPtr->mTerminate = true;
						nPtr->mConnectionMutex.unlock();
						#ifdef __SGCT_MUTEX_DEBUG__
							fprintf(stderr, "Mutex for connection %d is unlocked.\n", nPtr->getId());
						#endif
					}

					sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Network: Client %d terminated connection.\n", nPtr->getId());

					break; //exit loop
				}
				/*
					==========================================
					        HANDLE SYNC COMMUNICATION
					==========================================
				*/
				else
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
							sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Network: Error sync in sync frame: %d for connection %d\n", syncFrameNumber, nPtr->getId());
						}

#ifdef __SGCT_NETWORK_DEBUG__
						sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Package info: Frame = %d, Size = %u\n", syncFrameNumber, dataSize);
#endif
						//decode callback
						if(dataSize > 0)
							(nPtr->mDecoderCallbackFn)(recvBuf, dataSize, nPtr->getId());

						/*if(!nPtr->isServer())
						{
							nPtr->pushClientMessage();
						}*/
						sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::FrameSyncMutex );
						sgct_core::NetworkManager::gCond.notify_all();
						sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::FrameSyncMutex );

#ifdef __SGCT_NETWORK_DEBUG__
						sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif
					}
#if (_MSC_VER >= 1700) //visual studio 2012 or later
					else if( packageId == sgct_core::SGCTNetwork::ConnectedByte &&
						nPtr->mConnectedCallbackFn != nullptr)
#else
					else if( packageId == sgct_core::SGCTNetwork::ConnectedByte &&
						nPtr->mConnectedCallbackFn != NULL)
#endif
					{
#ifdef __SGCT_NETWORK_DEBUG__                    
						sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Signaling slave is connected... ");
#endif
						(nPtr->mConnectedCallbackFn)();
						sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::FrameSyncMutex );
						sgct_core::NetworkManager::gCond.notify_all();
						sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::FrameSyncMutex );
#ifdef __SGCT_NETWORK_DEBUG__
						sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif
					}
				}
			}
			/*
				================================================
					    HANDLE EXTERNAL ASCII COMMUNICATION
				================================================
			*/
			else if(nPtr->getTypeOfConnection() == sgct_core::SGCTNetwork::ExternalASCIIConnection)
			{
#ifdef __SGCT_NETWORK_DEBUG__
				sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Parsing external TCP ASCII data... ");
#endif
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

                    extBuffer = extBuffer.substr(found+2);//jump over \r\n
                    
#if (_MSC_VER >= 1700) //visual studio 2012 or later
                    if( nPtr->mDecoderCallbackFn != nullptr )
#else
                    if( nPtr->mDecoderCallbackFn != NULL )
#endif
                    {
                        (nPtr->mDecoderCallbackFn)(extMessage.c_str(), static_cast<int>(extMessage.size()), nPtr->getId());
                    }

					//reply
					nPtr->sendStr("OK\r\n");
                    found = extBuffer.find("\r\n");
				}
#ifdef __SGCT_NETWORK_DEBUG__
                sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif
			}
			/*
				================================================
					HANDLE EXTERNAL RAW/BINARY COMMUNICATION
				================================================
			*/
			else if(nPtr->getTypeOfConnection() == sgct_core::SGCTNetwork::ExternalRawConnection)
			{
#ifdef __SGCT_NETWORK_DEBUG__
				sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Parsing external TCP raw data... ");
#endif
		
#if (_MSC_VER >= 1700) //visual studio 2012 or later
				if( nPtr->mDecoderCallbackFn != nullptr )
#else
				if( nPtr->mDecoderCallbackFn != NULL )
#endif
				{
					(nPtr->mDecoderCallbackFn)(recvBuf, iResult, nPtr->getId());
				}

#ifdef __SGCT_NETWORK_DEBUG__
                sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif
			}
		}

		/*
			================================================
						HANDLE FAILED RECEIVE
			================================================
		*/
		else if (iResult == 0)
		{
#ifdef __SGCT_NETWORK_DEBUG__
			sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Setting connection status to false... ");
#endif
			nPtr->setConnectedStatus(false);
#ifdef __SGCT_NETWORK_DEBUG__
            sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif

			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "TCP Connection %d closed (error: %d)\n", nPtr->getId(), SGCT_ERRNO);
		}
		else //if negative
		{
#ifdef __SGCT_NETWORK_DEBUG__
			sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Setting connection status to false... ");
#endif
			nPtr->setConnectedStatus(false);
#ifdef __SGCT_NETWORK_DEBUG__
            sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif

			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "TCP connection %d recv failed: %d\n", nPtr->getId(), SGCT_ERRNO);
		}

	} while (iResult > 0 || nPtr->isConnected());


	//cleanup
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", nPtr->getId());
	#endif
	nPtr->mConnectionMutex.lock();
	if( recvBuf != NULL )
    {
        //free(recvbuf);
        delete [] recvBuf;
        recvBuf = NULL;
    }
    nPtr->mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", nPtr->getId());
	#endif

    //Close socket
    //contains mutex
	nPtr->closeSocket( nPtr->mSocket );

#if (_MSC_VER >= 1700) //visual studio 2012 or later
	if(nPtr->mUpdateCallbackFn != nullptr)
#else
	if(nPtr->mUpdateCallbackFn != NULL)
#endif
		nPtr->mUpdateCallbackFn( nPtr->getId() );

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Node %d disconnected!\n", nPtr->getId());
}

void sgct_core::SGCTNetwork::sendData(void * data, int length)
{
	//fprintf(stderr, "Send data size: %d\n", length);
#ifdef __SGCT_NETWORK_DEBUG__
	for(int i=0; i<length; i++)
        fprintf(stderr, "%u ", ((const char *)data)[i]);
    fprintf(stderr, "\n");
#endif

	ssize_t sendErr = send( mSocket, reinterpret_cast<const char *>(data), length, 0 );
	if (sendErr == SOCKET_ERROR)
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Send data failed!\n");
}

void sgct_core::SGCTNetwork::sendStr(std::string msg)
{
	ssize_t sendErr = send( mSocket, reinterpret_cast<const char *>(msg.c_str()), static_cast<int>(msg.size()), 0 );
	if (sendErr == SOCKET_ERROR)
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Send data failed!\n");
}

void sgct_core::SGCTNetwork::closeNetwork(bool forced)
{
    //clear callbacks	
#if (_MSC_VER >= 1700) //visual studio 2012 or later
	mDecoderCallbackFn	= nullptr;
	mUpdateCallbackFn	= nullptr;
	mConnectedCallbackFn = nullptr;
#else
	mDecoderCallbackFn	= NULL;
	mUpdateCallbackFn	= NULL;
	mConnectedCallbackFn = NULL;
#endif
	
	//release conditions
	sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	NetworkManager::gCond.notify_all();
	sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
	mStartConnectionCond.notify_all();
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif

	if( mCommThread != NULL )
    {
        if( !forced )
			mCommThread->join();

		delete mCommThread; //blocking sockets -> cannot wait for thread so just kill it brutally
		mCommThread = NULL;
    }

    if( mMainThread != NULL )
	{
        if( !forced )
			mMainThread->join();
       
		delete mMainThread;
		mMainThread = NULL;
    }

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Connection %d successfully terminated.\n", mId);
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

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Closing connection %d...\n", getId());

	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Locking mutex for connection %d...\n", mId);
	#endif
	mConnectionMutex.lock();
        mTerminate = true;
#if (_MSC_VER >= 1700) //visual studio 2012 or later
        mDecoderCallbackFn = nullptr;
#else
		 mDecoderCallbackFn = NULL;
#endif
        mConnected = false;
	mConnectionMutex.unlock();
	#ifdef __SGCT_MUTEX_DEBUG__
		fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
	#endif

	//wake up the connection handler thread (in order to finish)
	if( isServer() )
	{
		#ifdef __SGCT_MUTEX_DEBUG__
			fprintf(stderr, "Locking mutex for connection %d...\n", mId);
		#endif
		mConnectionMutex.lock();
		mStartConnectionCond.notify_all();
		mConnectionMutex.unlock();
		#ifdef __SGCT_MUTEX_DEBUG__
			fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
		#endif
	}

    closeSocket( mSocket );
    closeSocket( mListenSocket );
}
