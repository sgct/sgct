/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
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

#ifndef SGCT_DONT_USE_EXTERNAL
#include "../include/external/zlib.h"
#else
#include <zlib.h>
#endif

#include <stdlib.h>
#include <stdio.h>

#define MAX_NUMBER_OF_ATTEMPS 10
#define SGCT_SOCKET_BUFFER_SIZE 4096

sgct_core::SGCTNetwork::SGCTNetwork()
{
	mCommThread		= NULL;
	mMainThread		= NULL;
	mRecvBuf		= NULL;
	mUncompressBuf	= NULL;
	mSocket			= INVALID_SOCKET;
	mListenSocket	= INVALID_SOCKET;
	
	mDecoderCallbackFn			= SGCT_NULL_PTR;
	mUpdateCallbackFn			= SGCT_NULL_PTR;
	mConnectedCallbackFn		= SGCT_NULL_PTR;
	mAcknowledgeCallbackFn		= SGCT_NULL_PTR;
	mPackageDecoderCallbackFn	= SGCT_NULL_PTR;

	mConnectionType		= SyncConnection;
	mBufferSize			= 1024;
	mRequestedSize		= mBufferSize;
    mUncompressedBufferSize = mBufferSize;
	mSendFrame[Current]	= 0;
	mSendFrame[Previous]= 0;
	mRecvFrame[Current]	= 0;
	mRecvFrame[Previous]= -1;
	mTimeStamp[Send]	= 0.0;
	mTimeStamp[Total]	= 0.0;

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
void sgct_core::SGCTNetwork::init(const std::string port, const std::string address, bool _isServer, int id, sgct_core::SGCTNetwork::ConnectionTypes connectionType)
{
	mServer = _isServer;
	mConnectionType = connectionType;
	if (mConnectionType == SyncConnection)
    {
		mBufferSize = static_cast<int>(sgct::SharedData::instance()->getBufferSize());
        mUncompressedBufferSize = mBufferSize;
    }
	mId = id;

	mPort.assign(port);
	mAddress.assign(address);

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
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Attempting to connect to server (id: %d, ip: %s, type: %s)...\n", mId, getAddress().c_str(), getTypeStr().c_str());

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
			else
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Connect error code: %d\n", SGCT_ERRNO);
			}

			tthread::this_thread::sleep_for(tthread::chrono::seconds(1)); //wait for next attempt
		}
	}

	freeaddrinfo(result);
	mMainThread = new tthread::thread( connectionHandler, this );
}

void sgct_core::SGCTNetwork::connectionHandler(void *arg)
{
	sgct_core::SGCTNetwork * nPtr = (sgct_core::SGCTNetwork *)arg;

	if( nPtr->mServer )
	{
		while( !nPtr->isTerminated() )
		{
			if( !nPtr->mConnected )
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
					fprintf(stderr, "Locking mutex for connection %d...\n", nPtr->mId);
				#endif
				nPtr->mConnectionMutex.lock();
				nPtr->mStartConnectionCond.wait( nPtr->mConnectionMutex );
				nPtr->mConnectionMutex.unlock();
				#ifdef __SGCT_MUTEX_DEBUG__
					fprintf(stderr, "Mutex for connection %d is unlocked.\n", nPtr->mId);
				#endif
            }
		}
	}
	else //if client
	{
		nPtr->mCommThread = new tthread::thread( communicationHandler, nPtr );
	}

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Exiting connection handler for connection %d... \n", nPtr->mId);
}

/*!
\return the port of this connection
*/
std::string sgct_core::SGCTNetwork::getPort()
{
	return mPort;
}

/*!
\return the address of this connection
*/
std::string sgct_core::SGCTNetwork::getAddress()
{
	return mAddress;
}

/*!
\return the connection type as string
*/
std::string sgct_core::SGCTNetwork::getTypeStr()
{
	return getTypeStr(getType());
}

/*!
\return the connection type as string
*/
std::string sgct_core::SGCTNetwork::getTypeStr(sgct_core::SGCTNetwork::ConnectionTypes ct)
{
	std::string tmpStr;

	switch (ct)
	{
	case SyncConnection:
	default:
		tmpStr.assign("sync");
		break;

	case ExternalASCIIConnection:
		tmpStr.assign("external ASCII control");
		break;

	case ExternalRawConnection:
		tmpStr.assign("external binary control");
		break;

	case DataTransfer:
		tmpStr.assign("data transfer");
		break;
	}

	return tmpStr;
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
		if( getType() == sgct_core::SGCTNetwork::SyncConnection )
		{
			int bufferSize = SGCT_SOCKET_BUFFER_SIZE;
			iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(int));
			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTNetwork: Failed to set send buffer size to %d with error: %d\n!", bufferSize, SGCT_ERRNO);
			iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof(int));
			if (iResult == SOCKET_ERROR)
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SGCTNetwork: Failed to set receive buffer size to %d with error: %d\n!", bufferSize, SGCT_ERRNO);
		}

		//set on all connections types, cluster nodes sends data several times per second so there is no need so send alive packages
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

	mSendFrame[Previous].store(mSendFrame[Current].load());

	if( mSendFrame[Current] < MAX_NET_SYNC_FRAME_NUMBER )
		mSendFrame[Current]++;
	else
		mSendFrame[Current] = 0;

	mUpdated = false;


#ifdef __SGCT_MUTEX_DEBUG__
	fprintf(stderr, "Locking mutex for connection %d...\n", mId);
#endif
	mConnectionMutex.lock();
	mTimeStamp[Send] = sgct::Engine::getTime();
	mConnectionMutex.unlock();
#ifdef __SGCT_MUTEX_DEBUG__
	fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
#endif

	return mSendFrame[Current].load();
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
		messageToSend[0] = SGCTNetwork::DataId;
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
        
        //fill rest of header with DefaultId
        memset(messageToSend+9, DefaultId, 4);

		sendData((void*)messageToSend, static_cast<int>(currentMessageSize));

		sgct::SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

		sgct::MessageHandler::instance()->clearBuffer(); //clear the buffer
    }
	else
	{
		char tmpca[mHeaderSize];
		tmpca[0] = SGCTNetwork::DataId;
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
        
        //fill rest of header with DefaultId
        memset(tmpca+9, DefaultId, 4);

		sendData((void *)tmpca, mHeaderSize);
	}
}

int sgct_core::SGCTNetwork::getSendFrame(sgct_core::SGCTNetwork::ReceivedIndex ri)
{
	return mSendFrame[ri].load();
}

int sgct_core::SGCTNetwork::getRecvFrame(sgct_core::SGCTNetwork::ReceivedIndex ri)
{
	return mRecvFrame[ri].load();
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

	bool state = false;
	if(mServer)
	{
		state = ClusterManager::instance()->getFirmFrameLockSyncStatus() ?
			//master sends first -> so on reply they should be equal
			(mRecvFrame[Current] == mSendFrame[Current]) :
			//don't check if loose sync
			true;
	}
	else
	{
		state = ClusterManager::instance()->getFirmFrameLockSyncStatus() ?
			//slaves receives first and then sends so the prevois should be equal to the send
			(mRecvFrame[Previous] == mSendFrame[Current]) :
			//if loose sync just check if updated
			mUpdated.load();
	}

	return (state && mConnected);
}

void sgct_core::SGCTNetwork::setDecodeFunction(sgct_cppxeleven::function<void (const char*, int, int)> callback)
{
	mDecoderCallbackFn = callback;
}

void sgct_core::SGCTNetwork::setPackageDecodeFunction(sgct_cppxeleven::function<void(void*, int, int, int)> callback)
{
	mPackageDecoderCallbackFn = callback;
}

void sgct_core::SGCTNetwork::setUpdateFunction(sgct_cppxeleven::function<void (int)> callback)
{
	mUpdateCallbackFn = callback;
}

void sgct_core::SGCTNetwork::setConnectedFunction(sgct_cppxeleven::function<void (void)> callback)
{
	mConnectedCallbackFn = callback;
}

void sgct_core::SGCTNetwork::setAcknowledgeFunction(sgct_cppxeleven::function<void(int, int)> callback)
{
	mAcknowledgeCallbackFn = callback;
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
	return mConnected.load();
}

sgct_core::SGCTNetwork::ConnectionTypes sgct_core::SGCTNetwork::getType()
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

bool sgct_core::SGCTNetwork::isServer()
{
	return mServer.load();
}

bool sgct_core::SGCTNetwork::isTerminated()
{
	return mTerminate.load();
}

void sgct_core::SGCTNetwork::setRecvFrame(int i)
{
#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetwork::setRecvFrame\n");
#endif
	
	mRecvFrame[Previous].store(mRecvFrame[Current].load());
	mRecvFrame[Current] = i;
	mUpdated = true;

#ifdef __SGCT_MUTEX_DEBUG__
	fprintf(stderr, "Locking mutex for connection %d...\n", mId);
#endif
	mConnectionMutex.lock();
	mTimeStamp[Total] = sgct::Engine::getTime() - mTimeStamp[Send];
	mConnectionMutex.unlock();
#ifdef __SGCT_MUTEX_DEBUG__
	fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
#endif
}

_ssize_t sgct_core::SGCTNetwork::receiveData(SGCT_SOCKET & lsocket, char * buffer, int length, int flags)
{
    _ssize_t iResult = 0;
    int attempts = 1;

    while( iResult < length )
    {
        _ssize_t tmpRes = recv( lsocket, buffer + iResult, length - iResult, flags);
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

int sgct_core::SGCTNetwork::readSyncMessage(char * _header, int & _syncFrameNumber, int & _dataSize, int & _uncompressedDataSize)
{
	int iResult = sgct_core::SGCTNetwork::receiveData(mSocket,
		_header,
		static_cast<int>(sgct_core::SGCTNetwork::mHeaderSize),
		0);

	if (iResult == static_cast<int>(sgct_core::SGCTNetwork::mHeaderSize))
	{
		mHeaderId = _header[0];
#ifdef __SGCT_NETWORK_DEBUG__
		sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Header id=%d...\n", headerId);
#endif
		if (mHeaderId == sgct_core::SGCTNetwork::DataId || mHeaderId == sgct_core::SGCTNetwork::CompressedDataId)
		{
			//parse the sync frame number
			_syncFrameNumber = sgct_core::SGCTNetwork::parseInt(&_header[1]);
			//parse the data size
			_dataSize = sgct_core::SGCTNetwork::parseInt(&_header[5]);
			//parse the uncompressed size if compression is used
			_uncompressedDataSize = sgct_core::SGCTNetwork::parseInt(&_header[9]);

			setRecvFrame(_syncFrameNumber);
			if (_syncFrameNumber < 0)
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Network: Error sync in sync frame: %d for connection %d\n", _syncFrameNumber, mId);
			}

#ifdef __SGCT_NETWORK_DEBUG__
			sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Network: Package info: Frame = %d, Size = %u for connection %d\n", _syncFrameNumber, _dataSize, mId);
#endif

			//resize buffer if needed
#ifdef __SGCT_MUTEX_DEBUG__
			fprintf(stderr, "Locking mutex for connection %d...\n", mId);
#endif
			mConnectionMutex.lock();

			//grow only
			if (_dataSize > mBufferSize)
			{
				if (!mRecvBuf)
				{
					delete[] mRecvBuf;
					mRecvBuf = NULL;
				}

				//allocate
				mRecvBuf = new (std::nothrow) char[_dataSize];
				if (mRecvBuf != NULL)
				{
					mBufferSize = _dataSize;
				}
			}

			if (_uncompressedDataSize > mUncompressedBufferSize)
			{
				if (!mUncompressBuf)
				{
					delete[] mUncompressBuf;
					mUncompressBuf = NULL;
				}

				//allocate
				mUncompressBuf = new (std::nothrow) char[_uncompressedDataSize];
				if (mUncompressBuf != NULL)
				{
					mUncompressedBufferSize = _uncompressedDataSize;
				}
			}

			mConnectionMutex.unlock();
#ifdef __SGCT_MUTEX_DEBUG__
			fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
#endif
		}
	}

#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Receiving data (buffer size: %d)...\n", _dataSize);
#endif

	/*
	Get the data/message
	*/
	if (_dataSize > 0)
	{
		iResult = sgct_core::SGCTNetwork::receiveData(mSocket,
			mRecvBuf,
			_dataSize,
			0);
	}

	return iResult;
}

int sgct_core::SGCTNetwork::readDataTransferMessage(char * _header, int & _packageId, int & _dataSize, int & _uncompressedDataSize)
{
	int iResult = sgct_core::SGCTNetwork::receiveData(mSocket,
		_header,
		static_cast<int>(sgct_core::SGCTNetwork::mHeaderSize),
		0);

	if (iResult == static_cast<int>(sgct_core::SGCTNetwork::mHeaderSize))
	{
		mHeaderId = _header[0];
#ifdef __SGCT_NETWORK_DEBUG__
		sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Header id=%d...\n", headerId);
#endif
		if (mHeaderId == sgct_core::SGCTNetwork::DataId || mHeaderId == sgct_core::SGCTNetwork::CompressedDataId)
		{
			//parse the package id
			_packageId = sgct_core::SGCTNetwork::parseInt(&_header[1]);
			//parse the data size
			_dataSize = sgct_core::SGCTNetwork::parseInt(&_header[5]);
			//parse the uncompressed size if compression is used
			_uncompressedDataSize = sgct_core::SGCTNetwork::parseInt(&_header[9]);

			//resize buffer if needed
#ifdef __SGCT_MUTEX_DEBUG__
			fprintf(stderr, "Locking mutex for connection %d...\n", mId);
#endif
			mConnectionMutex.lock();

			//resize
			if (_dataSize != mBufferSize)
			{
				//clean up
				if (!mRecvBuf)
				{
					delete[] mRecvBuf;
					mRecvBuf = NULL;
				}

				//allocate
				mRecvBuf = new (std::nothrow) char[_dataSize];
				if (mRecvBuf != NULL)
				{
					mBufferSize = _dataSize;
				}
			}

			if (_uncompressedDataSize != mUncompressedBufferSize)
			{
				if (!mUncompressBuf)
				{
					delete[] mUncompressBuf;
					mUncompressBuf = NULL;
				}

				//allocate
				mUncompressBuf = new (std::nothrow) char[_uncompressedDataSize];
				if (mUncompressBuf != NULL)
				{
					mUncompressedBufferSize = _uncompressedDataSize;
				}
			}

			mConnectionMutex.unlock();
#ifdef __SGCT_MUTEX_DEBUG__
			fprintf(stderr, "Mutex for connection %d is unlocked.\n", nPtr->mId);
#endif
		}
		else if (mHeaderId == sgct_core::SGCTNetwork::Ack &&
			mAcknowledgeCallbackFn != SGCT_NULL_PTR)
		{
			//parse the package id
			_packageId = sgct_core::SGCTNetwork::parseInt(&_header[1]);
			(mAcknowledgeCallbackFn)(_packageId, mId);
		}
	}

#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Receiving data (buffer size: %d)...\n", _dataSize);
#endif
	/*
	Get the data/message
	*/
	if (_dataSize > 0 && _packageId > -1)
	{
		iResult = sgct_core::SGCTNetwork::receiveData(mSocket,
			mRecvBuf,
			_dataSize,
			0);
#ifdef __SGCT_NETWORK_DEBUG__
		sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Data type: %d, %d bytes of %u...\n", _packageId, iResult, _dataSize);
#endif
	}

	return iResult;
}

int sgct_core::SGCTNetwork::readExternalMessage()
{
	//do a normal read
	int iResult = recv(mSocket,
		mRecvBuf,
		mBufferSize,
		0);

	//if read fails try for x attempts
	int attempts = 1;
#ifdef __WIN32__
	while (iResult <= 0 && SGCT_ERRNO == WSAEINTR && attempts <= MAX_NUMBER_OF_ATTEMPS)
#else
	while (iResult <= 0 && SGCT_ERRNO == EINTR && attempts <= MAX_NUMBER_OF_ATTEMPS)
#endif
	{
		iResult = recv(mSocket,
			mRecvBuf,
			mBufferSize,
			0);

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Receiving data after interrupted system error (attempt %d)...\n", attempts);
		attempts++;
	}
	
	return iResult;
}

/*
function to decode messages
*/
void sgct_core::SGCTNetwork::communicationHandler(void *arg)
{
	sgct_core::SGCTNetwork * nPtr = (sgct_core::SGCTNetwork *)arg;

	//exit if terminating
	if( nPtr->isTerminated() )
        return;

	//listen for client if server
	if( nPtr->mServer )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Waiting for client to connect to connection %d (port %s)...\n", nPtr->mId, nPtr->getPort().c_str());

        nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);

		int accErr = SGCT_ERRNO;
#ifdef __WIN32__
        while( !nPtr->isTerminated() && nPtr->mSocket == INVALID_SOCKET && accErr == WSAEINTR)
#else
        while( !nPtr->isTerminated() && nPtr->mSocket == INVALID_SOCKET && accErr == EINTR)
#endif
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Re-accept after interrupted system on connection %d...\n", nPtr->mId);

		    nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);
		}

		if (nPtr->mSocket == INVALID_SOCKET)
		{
			sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_ERROR, "Accept connection %d failed! Error: %d\n", nPtr->mId, accErr);

			if (nPtr->mUpdateCallbackFn != SGCT_NULL_PTR)
				nPtr->mUpdateCallbackFn(nPtr->mId);
			return;
		}
	}

	nPtr->setConnectedStatus(true);
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Connection %d established!\n", nPtr->mId);

	if (nPtr->mUpdateCallbackFn != SGCT_NULL_PTR)
		nPtr->mUpdateCallbackFn(nPtr->mId);

	//init buffers
	char recvHeader[sgct_core::SGCTNetwork::mHeaderSize];
	memset(recvHeader, sgct_core::SGCTNetwork::DefaultId, sgct_core::SGCTNetwork::mHeaderSize);

    nPtr->mConnectionMutex.lock();
	nPtr->mRecvBuf = new (std::nothrow) char[nPtr->mBufferSize];
	nPtr->mUncompressBuf = new (std::nothrow) char[nPtr->mUncompressedBufferSize];
    nPtr->mConnectionMutex.unlock();
    
	std::string extBuffer; //for external comm

	// Receive data until the server closes the connection
	_ssize_t iResult = 0;
	do
	{
		//resize buffer request
		if (nPtr->getType() != sgct_core::SGCTNetwork::DataTransfer && nPtr->mRequestedSize > nPtr->mBufferSize)
		{
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Re-sizing tcp buffer size from %d to %d... ", nPtr->mBufferSize, nPtr->mRequestedSize);

			#ifdef __SGCT_MUTEX_DEBUG__
				fprintf(stderr, "Locking mutex for connection %d...\n", nPtr->mId);
			#endif
            nPtr->mConnectionMutex.lock();
				nPtr->mBufferSize = nPtr->mRequestedSize;

				//clean up
				if (!nPtr->mRecvBuf)
				{
					delete[] nPtr->mRecvBuf;
					nPtr->mRecvBuf = NULL;
				}

                //allocate
                bool allocError = false;
				nPtr->mRecvBuf = new (std::nothrow) char[nPtr->mRequestedSize];
				if (nPtr->mRecvBuf == NULL)
                    allocError = true;

			nPtr->mConnectionMutex.unlock();
			#ifdef __SGCT_MUTEX_DEBUG__
				fprintf(stderr, "Mutex for connection %d is unlocked.\n", nPtr->mId);
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
        
		int packageId = -1;
		int syncFrameNumber = -1;
        int dataSize = 0;
        int uncompressedDataSize = 0;
        
		nPtr->mHeaderId = sgct_core::SGCTNetwork::DefaultId;

		if( nPtr->getType() == sgct_core::SGCTNetwork::SyncConnection )
        {
			iResult = nPtr->readSyncMessage(recvHeader, syncFrameNumber, dataSize, uncompressedDataSize);
		}
		else if (nPtr->getType() == sgct_core::SGCTNetwork::DataTransfer)
		{
			iResult = nPtr->readDataTransferMessage(recvHeader, packageId, dataSize, uncompressedDataSize);
		}
		else
		{
			iResult = nPtr->readExternalMessage();
		}

		/*!
			==========================================================
			      IF DATA SUCCESSFULLY READ THAN DECODE IT
			==========================================================
		*/
		if (iResult > 0)
		{
			if (nPtr->getType() == sgct_core::SGCTNetwork::SyncConnection)
			{
				/*
					==========================================
							HANDLE SYNC DISCONNECTION
					==========================================
				*/
				if ( parseDisconnectPackage(recvHeader) )
				{
					nPtr->setConnectedStatus(false);

					/*
						Terminate client only. The server only resets the connection,
						allowing clients to connect.
					*/
					if( !nPtr->mServer )
					{
						nPtr->mTerminate = true;
					}

					sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Network: Client %d terminated connection.\n", nPtr->mId);

					break; //exit loop
				}
				/*
					==========================================
					        HANDLE SYNC COMMUNICATION
					==========================================
				*/
				else
				{
				if( nPtr->mHeaderId == sgct_core::SGCTNetwork::DataId &&
					nPtr->mDecoderCallbackFn != SGCT_NULL_PTR)
					{
						//decode callback
						if(dataSize > 0)
							(nPtr->mDecoderCallbackFn)(nPtr->mRecvBuf, dataSize, nPtr->mId);

						/*if(!nPtr->mServer)
						{
							nPtr->pushClientMessage();
						}*/
						sgct_core::NetworkManager::gCond.notify_all();

#ifdef __SGCT_NETWORK_DEBUG__
						sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif
					}
                    else if( nPtr->mHeaderId == sgct_core::SGCTNetwork::CompressedDataId &&
						nPtr->mDecoderCallbackFn != SGCT_NULL_PTR)
                    {
                        //decode callback
						if(dataSize > 0)
                        {
							//parse the package id
                            uLongf uncompressedSize = static_cast<uLongf>(uncompressedDataSize);
    
                            int err = uncompress(
                                                 reinterpret_cast<Bytef*>(nPtr->mUncompressBuf),
                                                 &uncompressedSize,
												 reinterpret_cast<Bytef*>(nPtr->mRecvBuf),
                                                 static_cast<uLongf>(dataSize));
                            
                            if(err == Z_OK)
                            {
                                //decode callback
								(nPtr->mDecoderCallbackFn)(nPtr->mUncompressBuf, static_cast<int>(uncompressedSize), nPtr->mId);
                            }
                            else
                            {
								sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Network: Failed to uncompress data for connection %d! Error: %s\n", nPtr->mId, getUncompressionErrorAsStr(err).c_str());
                            }
                        }
                        
						/*if(!nPtr->mServer)
                         {
                         nPtr->pushClientMessage();
                         }*/
						sgct_core::NetworkManager::gCond.notify_all();
                    }
					else if (nPtr->mHeaderId == sgct_core::SGCTNetwork::ConnectedId &&
						nPtr->mConnectedCallbackFn != SGCT_NULL_PTR)
					{
#ifdef __SGCT_NETWORK_DEBUG__
						sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Signaling slave is connected... ");
#endif
						(nPtr->mConnectedCallbackFn)();
						sgct_core::NetworkManager::gCond.notify_all();
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
			else if (nPtr->getType() == sgct_core::SGCTNetwork::ExternalASCIIConnection)
			{
#ifdef __SGCT_NETWORK_DEBUG__
				sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Parsing external TCP ASCII data... ");
#endif
				std::string tmpStr(nPtr->mRecvBuf);
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

					if (nPtr->mDecoderCallbackFn != SGCT_NULL_PTR)
                    {
						(nPtr->mDecoderCallbackFn)(extMessage.c_str(), static_cast<int>(extMessage.size()), nPtr->mId);
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
			else if (nPtr->getType() == sgct_core::SGCTNetwork::ExternalRawConnection)
			{
#ifdef __SGCT_NETWORK_DEBUG__
				sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Parsing external TCP raw data... ");
#endif
				if (nPtr->mDecoderCallbackFn != SGCT_NULL_PTR)
				{
					(nPtr->mDecoderCallbackFn)(nPtr->mRecvBuf, iResult, nPtr->mId);
				}

#ifdef __SGCT_NETWORK_DEBUG__
                sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif
			}
			/*
				==========================================
					HANDLE DATA TRANSFER COMMUNICATION
				==========================================
			*/
			else if (nPtr->getType() == sgct_core::SGCTNetwork::DataTransfer)
			{
				/*
					Disconnect if requested
				*/
				if (parseDisconnectPackage(recvHeader))
				{
					nPtr->setConnectedStatus(false);
					sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Network: File transfer %d terminated connection.\n", nPtr->mId);
				}
				/*
					Handle communication
				*/
				else
				{
					if ((nPtr->mHeaderId == sgct_core::SGCTNetwork::DataId || nPtr->mHeaderId == sgct_core::SGCTNetwork::CompressedDataId) &&
						nPtr->mPackageDecoderCallbackFn != SGCT_NULL_PTR && dataSize > 0)
                    {
						bool recvOk = false;
                        
                        //uncompressed
						if (nPtr->mHeaderId == sgct_core::SGCTNetwork::DataId)
                        {
                            //decode callback
							(nPtr->mPackageDecoderCallbackFn)(nPtr->mRecvBuf, dataSize, packageId, nPtr->mId);
                            recvOk = true;
                        }
                        else //compressed
                        {
                            //parse the package id
                            uLongf uncompressedSize = static_cast<uLongf>(uncompressedDataSize);
                            
                            int err = uncompress(
                                                 reinterpret_cast<Bytef*>(nPtr->mUncompressBuf),
                                                 &uncompressedSize,
												 reinterpret_cast<Bytef*>(nPtr->mRecvBuf),
                                                 static_cast<uLongf>(dataSize));
                            
                            if(err == Z_OK)
                            {
                                //decode callback
								(nPtr->mPackageDecoderCallbackFn)(nPtr->mUncompressBuf, static_cast<int>(uncompressedSize), packageId, nPtr->mId);
                                recvOk = true;
                            }
                            else
                            {
								sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Network: Failed to uncompress data for connection %d! Error: %s\n", nPtr->mId, getUncompressionErrorAsStr(err).c_str());
                            }
                        }
                        
                        if(recvOk)
                        {
                            //send acknowledge
                            char sendBuff[sgct_core::SGCTNetwork::mHeaderSize];
                            int pLenght = 0;
                            char *packageIdPtr = (char *)&packageId;
                            char *sizeDataPtr = (char *)&pLenght;
                            
                            sendBuff[0] = sgct_core::SGCTNetwork::Ack;
                            sendBuff[1] = packageIdPtr[0];
                            sendBuff[2] = packageIdPtr[1];
                            sendBuff[3] = packageIdPtr[2];
                            sendBuff[4] = packageIdPtr[3];
                            sendBuff[5] = sizeDataPtr[0];
                            sendBuff[6] = sizeDataPtr[1];
                            sendBuff[7] = sizeDataPtr[2];
                            sendBuff[8] = sizeDataPtr[3];

                            nPtr->sendData(sendBuff, sgct_core::SGCTNetwork::mHeaderSize);
                        }

						//Clear the buffer
						nPtr->mConnectionMutex.lock();

						//clean up
						delete[] nPtr->mRecvBuf;
						nPtr->mRecvBuf = NULL;
                        
						if (nPtr->mUncompressBuf)
                        {
							delete[] nPtr->mUncompressBuf;
							nPtr->mUncompressBuf = NULL;
                        }

						nPtr->mBufferSize = 0;
                        nPtr->mUncompressedBufferSize = 0;
						nPtr->mConnectionMutex.unlock();
					}
					else if (nPtr->mHeaderId == sgct_core::SGCTNetwork::ConnectedId &&
						nPtr->mConnectedCallbackFn != SGCT_NULL_PTR)
					{
#ifdef __SGCT_NETWORK_DEBUG__
						sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Signaling slave is connected... ");
#endif
						(nPtr->mConnectedCallbackFn)();
						sgct_core::NetworkManager::gCond.notify_all();
                        
#ifdef __SGCT_NETWORK_DEBUG__
						sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Done.\n");
#endif
					}
				}
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

			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "TCP Connection %d closed (error: %d)\n", nPtr->mId, SGCT_ERRNO);
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

			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "TCP connection %d recv failed: %d\n", nPtr->mId, SGCT_ERRNO);
		}

	} while (iResult > 0 || nPtr->mConnected);


	//cleanup
	if (nPtr->mRecvBuf != NULL)
    {
		delete[] nPtr->mRecvBuf;
		nPtr->mRecvBuf = NULL;
    }
    
	if (nPtr->mUncompressBuf != NULL)
    {
		delete[] nPtr->mUncompressBuf;
		nPtr->mUncompressBuf = NULL;
    }

    //Close socket
    //contains mutex
	nPtr->closeSocket( nPtr->mSocket );

	if (nPtr->mUpdateCallbackFn != SGCT_NULL_PTR)
		nPtr->mUpdateCallbackFn( nPtr->mId );

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Node %d disconnected!\n", nPtr->mId);
}

void sgct_core::SGCTNetwork::sendData(void * data, int length)
{
	//fprintf(stderr, "Send data size: %d\n", length);
#ifdef __SGCT_NETWORK_DEBUG__
	for(int i=0; i<length; i++)
        fprintf(stderr, "%u ", ((const char *)data)[i]);
    fprintf(stderr, "\n");
#endif
	_ssize_t sentLen;
	int sendSize = length;

	while (sendSize > 0)
	{
		int offset = length - sendSize;
		sentLen = send(mSocket, reinterpret_cast<const char *>(data)+offset, sendSize, 0);
		if (sentLen == SOCKET_ERROR)
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Send data failed!\n");
			break;
		}
		else
			sendSize -= sentLen;
	}
}

void sgct_core::SGCTNetwork::sendStr(std::string msg)
{
	//sendData(static_cast<void *>(&msg), static_cast<int>(msg.size())); //doesn't work
	sendData((void *)(msg.c_str()), static_cast<int>(msg.size()));
}

void sgct_core::SGCTNetwork::closeNetwork(bool forced)
{
    //clear callbacks
	mDecoderCallbackFn			= SGCT_NULL_PTR;
	mUpdateCallbackFn			= SGCT_NULL_PTR;
	mConnectedCallbackFn		= SGCT_NULL_PTR;
	mAcknowledgeCallbackFn		= SGCT_NULL_PTR;
	mPackageDecoderCallbackFn	= SGCT_NULL_PTR;

	//release conditions
	NetworkManager::gCond.notify_all();
	mStartConnectionCond.notify_all();

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
	if( mConnected )
	{
        char gameOver[9];
		gameOver[0] = DisconnectId;
        gameOver[1] = 24; //ASCII for cancel
        gameOver[2] = '\r';
        gameOver[3] = '\n';
        gameOver[4] = 27; //ASCII for Esc
        gameOver[5] = '\r';
        gameOver[6] = '\n';
        gameOver[7] = '\0';
		gameOver[8] = DefaultId;
        sendData(gameOver, mHeaderSize);
	}

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Closing connection %d...\n", mId);

#ifdef __SGCT_MUTEX_DEBUG__
	fprintf(stderr, "Locking mutex for connection %d...\n", mId);
#endif

	mConnectionMutex.lock();
		mDecoderCallbackFn = SGCT_NULL_PTR;
	mConnectionMutex.unlock();

#ifdef __SGCT_MUTEX_DEBUG__
	fprintf(stderr, "Mutex for connection %d is unlocked.\n", mId);
#endif

	mConnected = false;
	mTerminate = true;

	//wake up the connection handler thread (in order to finish)
	if( mServer )
	{
		mStartConnectionCond.notify_all();
	}

    closeSocket( mSocket );
    closeSocket( mListenSocket );
}

bool sgct_core::SGCTNetwork::parseDisconnectPackage(char * headerPtr)
{
	if (headerPtr[0] == sgct_core::SGCTNetwork::DisconnectId &&
		headerPtr[1] == 24 &&
		headerPtr[2] == '\r' &&
		headerPtr[3] == '\n' &&
		headerPtr[4] == 27 &&
		headerPtr[5] == '\r' &&
		headerPtr[6] == '\n' &&
		headerPtr[7] == '\0')
		return true;
	else
		return false;
}

std::string sgct_core::SGCTNetwork::getUncompressionErrorAsStr(int err)
{
    std::string errStr;
    switch(err)
    {
        case Z_BUF_ERROR:
            errStr.assign("Dest. buffer not large enough.");
            break;
            
        case Z_MEM_ERROR:
            errStr.assign("Insufficient memory.");
            break;
            
        case Z_DATA_ERROR:
            errStr.assign("Corrupted data.");
            break;
            
        default:
            errStr.assign("Unknown error.");
            break;
    }
    
    return errStr;
}
