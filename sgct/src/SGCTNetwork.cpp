#if !(_MSC_VER >= 1400) //if not visual studio 2005 or later
    #define _WIN32_WINNT 0x501
#endif

#include "../include/sgct/SGCTNetwork.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/NodeManager.h"
#include <stdlib.h>
#include <stdio.h>
#include <ws2tcpip.h>

GLFWmutex gDecoderMutex;

void GLFWCALL communicationHandler(void *arg);

core_sgct::SGCTNetwork::SGCTNetwork()
{
	mainThreadID		= -1;
	mSocket				= INVALID_SOCKET;
	mListenSocket		= INVALID_SOCKET;
	mDecoderCallbackFn	= NULL;
	mDisconnectedCallbackFn = NULL;
	mServerType			= SyncServer;
	mBufferSize			= 1024;
	mRequestedSize		= mBufferSize;
	mSendFrame			= 0;
	mRecvFrame[0]		= 0;
	mRecvFrame[1]		= 0;
	mConnected			= false;
	mId					= -1;
}

void core_sgct::SGCTNetwork::init(const std::string port, const std::string ip, bool _isServer, int id, int serverType)
{
	gDecoderMutex = glfwCreateMutex();
	mServer = _isServer;
	mServerType = serverType;
	mBufferSize = sgct::SharedData::Instance()->getBufferSize();
	mId = id;

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

		// Setup the TCP listening socket
		iResult = bind( mListenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (iResult == SOCKET_ERROR)
		{
			freeaddrinfo(result);
			closesocket(mListenSocket);
			throw "Bind listen socket failed!";
		}
	}
	else
	{
		//Client socket
		mSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (mSocket == INVALID_SOCKET)
		{
			freeaddrinfo(result);
			throw "Failed to init client socket!";
		}

		if(!setNoDelay(&mSocket))
		{
			freeaddrinfo(result);
			throw "Failed to set client socket to TCP_NODELAY!";
		}

		// Connect to server.
		while( true )
		{
			sgct::MessageHandler::Instance()->print("Attempting to connect to server...\n");
			iResult = connect( mSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
			if (iResult != SOCKET_ERROR)
				break;

			glfwSleep(0.25); //wait for next attempt
		}
	}

	freeaddrinfo(result);
	mainThreadID = glfwCreateThread( communicationHandler, this );
	if( mainThreadID < 0)
	{
		mServer ? closesocket(mListenSocket) : closesocket(mSocket);
		throw "Failed to start communication thread!";
	}
}

bool core_sgct::SGCTNetwork::setNoDelay(SOCKET * socketPtr)
{
	if(socketPtr != NULL)
	{
		int flag = 1;
		int iResult = setsockopt(*socketPtr, /* socket affected */
		IPPROTO_TCP,     /* set option at TCP level */
		TCP_NODELAY,     /* name of option */
		(char *) &flag,  /* the cast is historical cruft */
		sizeof(int));    /* length of option value */

		if (iResult < 0)
		{
			return false;
		}
	}

	return true;
}

void core_sgct::SGCTNetwork::setBufferSize(unsigned int newSize)
{
	mRequestedSize = newSize;
}

void core_sgct::SGCTNetwork::iterateFrameCounter()
{
	if( mSendFrame < MAX_NET_SYNC_FRAME_NUMBER )
		mSendFrame++;
	else
		mSendFrame = 0;
}

void core_sgct::SGCTNetwork::checkIfBufferNeedsResizing()
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

        sendData(resizeMessage,5);
    }
}

void core_sgct::SGCTNetwork::pushClientMessage()
{
	//The servers's render function is locked until a message starting with the ack-byte is received.
	int currentFrame = getSendFrame();
	unsigned char *p = (unsigned char *)&currentFrame;

	//check if message fits in buffer
    if(sgct::MessageHandler::Instance()->getDataSize() > syncHeaderSize &&
        sgct::MessageHandler::Instance()->getDataSize() < mBufferSize)
    {
        //Don't remove this pointer, somehow the send function doesn't
		//work during the first call without setting the pointer first!!!
		char * messageToSend = sgct::MessageHandler::Instance()->getMessage();
		messageToSend[0] = SGCTNetwork::SyncHeader;
		messageToSend[1] = p[0];
		messageToSend[2] = p[1];
		messageToSend[3] = p[2];
		messageToSend[4] = p[3];
		sendData((void*)messageToSend, sgct::MessageHandler::Instance()->getDataSize());
		sgct::MessageHandler::Instance()->clearBuffer(); //clear the buffer
    }
    else if(sgct::MessageHandler::Instance()->getDataSize() > syncHeaderSize &&
            sgct::MessageHandler::Instance()->getDataSize() >= mBufferSize )
    {
		//Don't remove this pointer, somehow the send function doesn't
		//work during the first call without setting the pointer first!!!
		char * messageToSend = sgct::MessageHandler::Instance()->getTrimmedMessage(mBufferSize-syncHeaderSize);
		messageToSend[0] = SGCTNetwork::SyncHeader;
		messageToSend[1] = p[0];
		messageToSend[2] = p[1];
		messageToSend[3] = p[2];
		messageToSend[4] = p[3];
		sendData((void*)messageToSend, sgct::MessageHandler::Instance()->getTrimmedDataSize());
    }
	else
	{
		char tmpca[syncHeaderSize];
		tmpca[0] = SGCTNetwork::SyncHeader;
		tmpca[1] = p[0];
		tmpca[2] = p[1];
		tmpca[3] = p[2];
		tmpca[4] = p[3];
		sendData((void *)tmpca,syncHeaderSize);
	}
}

int core_sgct::SGCTNetwork::getSendFrame()
{
	int tmpi;
	glfwLockMutex( gDecoderMutex );
		tmpi = mSendFrame;
	glfwUnlockMutex( gDecoderMutex );
	return tmpi;
}

bool core_sgct::SGCTNetwork::compareFrames()
{
	bool tmpb;
	glfwLockMutex( gDecoderMutex );
		tmpb = (mRecvFrame[0] == mRecvFrame[1]);
	glfwUnlockMutex( gDecoderMutex );
	return tmpb;
}

void core_sgct::SGCTNetwork::swapFrames()
{
	glfwLockMutex( gDecoderMutex );
		mRecvFrame[1] = mRecvFrame[0]; 
	glfwUnlockMutex( gDecoderMutex );
}

void core_sgct::SGCTNetwork::setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback)
{
	mDecoderCallbackFn = callback;
}

void core_sgct::SGCTNetwork::setDisconnectedFunction(std::tr1::function<void (void)> callback)
{
	mDisconnectedCallbackFn = callback;
}

void core_sgct::SGCTNetwork::setConnectedStatus(bool state)
{
	glfwLockMutex( gDecoderMutex );
		mConnected = state;
	glfwUnlockMutex( gDecoderMutex );
}

/*
function to decode messages
*/
void GLFWCALL communicationHandler(void *arg)
{
	core_sgct::SGCTNetwork * nPtr = (core_sgct::SGCTNetwork *)arg;

	//listen for client if server
	if( nPtr->isServer() )
	{
		glfwLockMutex( gDecoderMutex );
			sgct::MessageHandler::Instance()->print("Waiting for client to connect to connection %d...\n", nPtr->getId());
		glfwUnlockMutex( gDecoderMutex );
		
		if( listen( nPtr->mListenSocket, SOMAXCONN ) == SOCKET_ERROR )
		{
			return;
		}

		nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);
		if (nPtr->mSocket == INVALID_SOCKET )//&& nPtr->setNoDelay(&(nPtr->mSocket)))
		{
			nPtr->setConnectedStatus(false);
			return;	
		}
	}
	
	nPtr->setConnectedStatus(true);

	glfwLockMutex( gDecoderMutex );
		sgct::MessageHandler::Instance()->print("Connection %d established!\n", nPtr->getId());
		if(nPtr->mDisconnectedCallbackFn != NULL)
			nPtr->mDisconnectedCallbackFn();
	glfwUnlockMutex( gDecoderMutex );

	//say hi
	send(nPtr->mSocket, "Connected..\r\n", 13, 0);

	//init buffer
	int recvbuflen = nPtr->mBufferSize;
	char * recvbuf;
	recvbuf = (char *) malloc(nPtr->mBufferSize);

	std::string extBuffer;

	// Receive data until the server closes the connection
	int iResult;
	do
	{
		//resize buffer request
		if( nPtr->mRequestedSize > nPtr->mBufferSize )
		{
			glfwLockMutex( gDecoderMutex );
				sgct::MessageHandler::Instance()->print("Network: New package size is %d\n", nPtr->mRequestedSize);
				nPtr->mBufferSize = nPtr->mRequestedSize;
				recvbuflen = nPtr->mRequestedSize;
				free(recvbuf);
				recvbuf = (char *)malloc(nPtr->mRequestedSize);
			glfwUnlockMutex( gDecoderMutex );
		}
		
		iResult = recv( nPtr->mSocket, recvbuf, recvbuflen, 0);

		if (iResult > 0)
		{
			if( nPtr->getTypeOfServer() == core_sgct::SGCTNetwork::SyncServer )
			{
				//check type of message
				//Resize if needed
				if( recvbuf[0] == core_sgct::SGCTNetwork::SizeHeader &&
					recvbuflen > 4)
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
					nPtr->mBufferSize = cui.newSize;
					recvbuflen = cui.newSize;
					free(recvbuf);
					recvbuf = (char *)malloc(cui.newSize);
					glfwUnlockMutex( gDecoderMutex );
				}
				else if( recvbuf[0] == core_sgct::SGCTNetwork::SyncHeader &&
					iResult >= core_sgct::SGCTNetwork::syncHeaderSize &&
					nPtr->mDecoderCallbackFn != NULL)
				{
					//convert from uchar to int32
					union
					{
						int i;
						unsigned char c[4];
					} ci;

					ci.c[0] = recvbuf[1];
					ci.c[1] = recvbuf[2];
					ci.c[2] = recvbuf[3];
					ci.c[3] = recvbuf[4];
					
					glfwLockMutex( gDecoderMutex );
						nPtr->setRecvFrame( ci.i );
						(nPtr->mDecoderCallbackFn)(recvbuf+core_sgct::SGCTNetwork::syncHeaderSize,
							iResult-core_sgct::SGCTNetwork::syncHeaderSize,
							nPtr->getId());
					glfwUnlockMutex( gDecoderMutex );
				}
			}
			else if(nPtr->getTypeOfServer() == core_sgct::SGCTNetwork::ExternalControl)
			{
				std::string tmpStr(recvbuf);
				extBuffer += tmpStr.substr(0, iResult);
				std::size_t found = extBuffer.find("\r\n");
				if( found != std::string::npos )
				{
					std::string extMessage = extBuffer.substr(0,found);
					extBuffer = extBuffer.substr(found+2);//jump over \r\n

					glfwLockMutex( gDecoderMutex );
						if( nPtr->mDecoderCallbackFn != NULL )
						{
							(nPtr->mDecoderCallbackFn)(extMessage.c_str(), extMessage.size(), nPtr->getId());
						}
						nPtr->sendStr("OK\r\n");
					glfwUnlockMutex( gDecoderMutex );
				}
			}
		}
		else if (iResult == 0)
		{
			nPtr->setConnectedStatus(false);
			
			glfwLockMutex( gDecoderMutex );
				sgct::MessageHandler::Instance()->print("TCP Connection %d closed.\n", nPtr->getId());
			glfwUnlockMutex( gDecoderMutex );
		}
		else
		{
			nPtr->setConnectedStatus(false);

			glfwLockMutex( gDecoderMutex );
				sgct::MessageHandler::Instance()->print("TCP connection %d recv failed: %d\n", nPtr->getId(), WSAGetLastError());
			glfwUnlockMutex( gDecoderMutex );
		}
	} while (iResult > 0 || nPtr->isConnected());

	//cleanup
	free(recvbuf);
	
	glfwLockMutex( gDecoderMutex );
	if(nPtr->mDisconnectedCallbackFn != NULL)
		nPtr->mDisconnectedCallbackFn();
	glfwUnlockMutex( gDecoderMutex );
}

void core_sgct::SGCTNetwork::sendData(void * data, int lenght)
{
	int iResult = send(mSocket, (const char *)data, lenght, 0);
	if (iResult == SOCKET_ERROR)
		sgct::MessageHandler::Instance()->print("Send data failed with error: %d\n", WSAGetLastError());
}

void core_sgct::SGCTNetwork::sendStr(std::string msg)
{
	int iResult = send(mSocket, msg.c_str(), msg.size(), 0);
	if (iResult == SOCKET_ERROR)
		sgct::MessageHandler::Instance()->print("Send data failed with error: %d\n", WSAGetLastError());
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
	sendData(gameOver, 7);
	
	mDecoderCallbackFn = NULL;

	mConnected = false;
	sgct::MessageHandler::Instance()->print("Closing server connection...");
	if( mainThreadID != -1 )
		glfwDestroyThread(mainThreadID); //blocking sockets -> cannot wait for thread so just kill it brutally
	if( mSocket != INVALID_SOCKET )
	{
		shutdown(mSocket, SD_BOTH);
		closesocket( mSocket );
	}

	if( mListenSocket != INVALID_SOCKET )
	{
		shutdown(mListenSocket, SD_BOTH);
		closesocket( mListenSocket );
	}
	sgct::MessageHandler::Instance()->print(" Done!\n");
}

void core_sgct::SGCTNetwork::setMutexState(bool lock)
{
	lock ? glfwLockMutex( gDecoderMutex ) : glfwUnlockMutex( gDecoderMutex );
}
