/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
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

core_sgct::SGCTNetwork::SGCTNetwork()
{
	mCommThreadId		= -1;
	mMainThreadId		= -1;
	mSocket				= INVALID_SOCKET;
	mListenSocket		= INVALID_SOCKET;
	mDecoderCallbackFn	= NULL;
	mUpdateCallbackFn	= NULL;
	mConnectedCallbackFn = NULL;
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
	mServer = _isServer;
	mServerType = serverType;
	mBufferSize = sgct::SharedData::Instance()->getBufferSize();
	mId = id;

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
}

void GLFWCALL connectionHandler(void *arg)
{
	core_sgct::SGCTNetwork * nPtr = (core_sgct::SGCTNetwork *)arg;

	if( nPtr->isServer() )
	{
		while(true)
		{
			if( !nPtr->isConnected() )
			{
				sgct::MessageHandler::Instance()->print("Listening for client at connection %d... \n", nPtr->getId());

				if( nPtr->mCommThreadId != -1 )
					glfwWaitThread( nPtr->mCommThreadId, GLFW_WAIT );

				nPtr->mCommThreadId = glfwCreateThread( communicationHandler, nPtr );
				if( nPtr->mCommThreadId < 0)
				{
					return;
				}
			}

			sgct::Engine::lockMutex(core_sgct::NetworkManager::gMutex);
				sgct::Engine::waitCond( core_sgct::NetworkManager::gStartConnectionCond,
					core_sgct::NetworkManager::gMutex,
					GLFW_INFINITY );
			sgct::Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
		}
	}
	else
	{
		nPtr->mCommThreadId = glfwCreateThread( communicationHandler, nPtr );
		if( nPtr->mCommThreadId < 0)
		{
			return;
		}
	}
}

void core_sgct::SGCTNetwork::setOptions(SOCKET * socketPtr)
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

        iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_REUSEADDR, (char*)&flag, sizeof(int));
        iResult = setsockopt(*socketPtr, SOL_SOCKET, SO_KEEPALIVE, (char*)&flag, sizeof(int));
	}
}

void core_sgct::SGCTNetwork::closeSocket(SOCKET lSocket)
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

	    int iResult = SOCKET_ERROR; //init

#ifdef __WIN32__
        iResult = shutdown(lSocket, SD_BOTH);
#else
		iResult = shutdown(lSocket, SHUT_RDWR);
#endif

		if (iResult == SOCKET_ERROR)
#ifdef __WIN32__
        sgct::MessageHandler::Instance()->print("Socket shutdown failed with error: %d\n", WSAGetLastError());
		closesocket( lSocket );
#else
        sgct::MessageHandler::Instance()->print("Socket shutdown failed with error: %d\n", errno);
		close( lSocket );
#endif
		lSocket = INVALID_SOCKET;
	}
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
    if( sgct::SharedData::Instance()->getDataSize() > mBufferSize && isConnected())
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

        if( sendData(resizeMessage,5) == SOCKET_ERROR )
            sgct::MessageHandler::Instance()->print("Failed to send resize message!\n");
    }
}

void core_sgct::SGCTNetwork::pushClientMessage()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::pushClientMessage\n");
#endif

	//The servers's render function is locked until a message starting with the ack-byte is received.
	int currentFrame = getSendFrame();
	unsigned char *p = (unsigned char *)&currentFrame;

	//check if message fits in buffer
    if(sgct::MessageHandler::Instance()->getDataSize() > syncHeaderSize)
    {
        //Don't remove this pointer, somehow the send function doesn't
		//work during the first call without setting the pointer first!!!
		char * messageToSend = sgct::MessageHandler::Instance()->getMessage();
		messageToSend[0] = SGCTNetwork::SyncHeader;
		messageToSend[1] = p[0];
		messageToSend[2] = p[1];
		messageToSend[3] = p[2];
		messageToSend[4] = p[3];

		unsigned int currentMessageSize =
			sgct::MessageHandler::Instance()->getDataSize() > mBufferSize ?
			mBufferSize :
			sgct::MessageHandler::Instance()->getDataSize();
		unsigned char *currentMessageSizePtr = (unsigned char *)&currentMessageSize;
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
		char tmpca[syncHeaderSize];
		tmpca[0] = SGCTNetwork::SyncHeader;
		tmpca[1] = p[0];
		tmpca[2] = p[1];
		tmpca[3] = p[2];
		tmpca[4] = p[3];

		unsigned int localSyncHeaderSize = syncHeaderSize;
		unsigned char *currentMessageSizePtr = (unsigned char *)&localSyncHeaderSize;
		tmpca[5] = currentMessageSizePtr[0];
		tmpca[6] = currentMessageSizePtr[1];
		tmpca[7] = currentMessageSizePtr[2];
		tmpca[8] = currentMessageSizePtr[3];

		if( sendData((void *)tmpca, syncHeaderSize) == SOCKET_ERROR )
            sgct::MessageHandler::Instance()->print("Failed to send sync header!\n");
	}
}

int core_sgct::SGCTNetwork::getSendFrame()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::getSendFrame\n");
#endif
	int tmpi;
	sgct::Engine::lockMutex(NetworkManager::gMutex);
		tmpi = mSendFrame;
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
	return tmpi;
}

bool core_sgct::SGCTNetwork::compareFrames()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::compareFrames\n");
#endif
	bool tmpb;
	sgct::Engine::lockMutex(NetworkManager::gMutex);
		tmpb = (mRecvFrame[0] == mRecvFrame[1]);
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
	return tmpb;
}

void core_sgct::SGCTNetwork::swapFrames()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::swapFrames\n");
#endif
	sgct::Engine::lockMutex(NetworkManager::gMutex);
		mRecvFrame[1] = mRecvFrame[0];
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
}

void core_sgct::SGCTNetwork::setDecodeFunction(std::tr1::function<void (const char*, int, int)> callback)
{
	mDecoderCallbackFn = callback;
}

void core_sgct::SGCTNetwork::setUpdateFunction(std::tr1::function<void (int, bool)> callback)
{
	mUpdateCallbackFn = callback;
}

void core_sgct::SGCTNetwork::setConnectedFunction(std::tr1::function<void (void)> callback)
{
	mConnectedCallbackFn = callback;
}

void core_sgct::SGCTNetwork::setConnectedStatus(bool state)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::setConnectedStatus = %s at syncframe %d\n", state ? "true" : "false", getSendFrame());
#endif
	sgct::Engine::lockMutex(NetworkManager::gMutex);
		mConnected = state;
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
}

bool core_sgct::SGCTNetwork::isConnected()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::isConnected\n");
#endif
	bool tmpb;
	sgct::Engine::lockMutex(NetworkManager::gMutex);
		tmpb = mConnected;
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
	return tmpb;
}

int core_sgct::SGCTNetwork::getTypeOfServer()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::getTypeOfServer\n");
#endif
	int tmpi;
	sgct::Engine::lockMutex(NetworkManager::gMutex);
		tmpi = mServerType;
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
	return tmpi;
}

int core_sgct::SGCTNetwork::getId()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::getId\n");
#endif
	int tmpi;
	sgct::Engine::lockMutex(NetworkManager::gMutex);
		tmpi = mId;
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
	return tmpi;
}

bool core_sgct::SGCTNetwork::isServer()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::isServer\n");
#endif
	bool tmpb;
	sgct::Engine::lockMutex(NetworkManager::gMutex);
		tmpb = mServer;
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
	return tmpb;
}

void core_sgct::SGCTNetwork::setRecvFrame(int i)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SGCTNetwork::setRecvFrame\n");
#endif
	sgct::Engine::lockMutex(NetworkManager::gMutex);
	mRecvFrame[0] = i;
	sgct::Engine::unlockMutex(NetworkManager::gMutex);
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
		sgct::MessageHandler::Instance()->print("Waiting for client to connect to connection %d...\n", nPtr->getId());

		nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);

#ifdef __WIN32__
		while( nPtr->mSocket == INVALID_SOCKET && errno == WSAEINTR)
#else
		while( nPtr->mSocket == INVALID_SOCKET && errno == EINTR)
#endif
		{
		    sgct::MessageHandler::Instance()->print("Re-accept communication after interrupted system error...\n");
		    nPtr->mSocket = accept(nPtr->mListenSocket, NULL, NULL);
		}

		if (nPtr->mSocket == INVALID_SOCKET)
		{
#ifdef __WIN32__
            sgct::MessageHandler::Instance()->print("Accept connection %d failed! Error: %d\n", nPtr->getId(), WSAGetLastError());
#else
            sgct::MessageHandler::Instance()->print("Accept connection %d failed! Error: %d\n", nPtr->getId(), errno);
#endif

			if(nPtr->mUpdateCallbackFn != NULL)
                nPtr->mUpdateCallbackFn(nPtr->getId(), false);
			return;
		}
	}

	nPtr->setConnectedStatus(true);
	sgct::MessageHandler::Instance()->print("Connection %d established!\n", nPtr->getId());

	if(nPtr->mUpdateCallbackFn != NULL)
		nPtr->mUpdateCallbackFn(nPtr->getId(), true);

	//init buffer
	int recvbuflen = nPtr->mBufferSize;
	char * recvbuf;
	recvbuf = reinterpret_cast<char *>( malloc(nPtr->mBufferSize) );

	std::string extBuffer;

	// Receive data until the server closes the connection
	int iResult;
	do
	{
		//resize buffer request
		if( nPtr->mRequestedSize > nPtr->mBufferSize )
		{
			#ifdef __SGCT_DEBUG__
                sgct::MessageHandler::Instance()->print("Re-sizing tcp buffer size from %d to %d... ", nPtr->mBufferSize, nPtr->mRequestedSize);
            #endif

            sgct::MessageHandler::Instance()->print("Network: New package size is %d\n", nPtr->mRequestedSize);
			sgct::Engine::lockMutex(core_sgct::NetworkManager::gMutex);
				nPtr->mBufferSize = nPtr->mRequestedSize;
				recvbuflen = nPtr->mRequestedSize;

				recvbuf = reinterpret_cast<char *>( realloc(recvbuf, nPtr->mRequestedSize) );
			sgct::Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

			if(recvbuf != NULL)
				sgct::MessageHandler::Instance()->print("Network: Buffer resized successfully!\n");
			else
				sgct::MessageHandler::Instance()->print("Network error: Buffer failed to resize!\n");
			#ifdef __SGCT_DEBUG__
                sgct::MessageHandler::Instance()->print("Done.\n");
            #endif
		}

#ifdef __SGCT_DEBUG__
        sgct::MessageHandler::Instance()->print("Receiving data (buffer size: %d)...\n", recvbuflen);
#endif
		iResult = recv( nPtr->mSocket, recvbuf, recvbuflen, 0);
#ifdef __WIN32__
		while( iResult < 0 && errno == WSAEINTR)
#else
		while( iResult < 0 && errno == EINTR)
#endif
		{
		    sgct::MessageHandler::Instance()->print("Receiving data after interrupted system error...\n");
		    iResult = recv( nPtr->mSocket, recvbuf, recvbuflen, 0);
		}
#ifdef __SGCT_DEBUG__
        sgct::MessageHandler::Instance()->print("Done. Received %d bytes.\n", iResult);
#endif
		if (iResult > 0)
		{

#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Received bytes: %d.\n", iResult);
            sgct::MessageHandler::Instance()->print("First byte: %d. Header size: %d\n",
                                                    recvbuf[0],
                                                    core_sgct::SGCTNetwork::syncHeaderSize);

#endif

			if( nPtr->getTypeOfServer() == core_sgct::SGCTNetwork::SyncServer )
			{
				//check type of message
				//Resize if needed
				if( recvbuf[0] == core_sgct::SGCTNetwork::SizeHeader &&
					recvbuflen > 4)
				{
#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Re-sizing shared data buffer... ");
#endif

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

                    /*
                        MessageHandler contains a mutex object and cannot be called when the mutex is locked.
                    */
					sgct::MessageHandler::Instance()->print("Network: New package size is %d\n", cui.newSize);
					sgct::Engine::lockMutex(core_sgct::NetworkManager::gMutex);
					nPtr->mBufferSize = cui.newSize;
					recvbuflen = static_cast<int>(cui.newSize);

                    recvbuf = reinterpret_cast<char *>( realloc(recvbuf, cui.newSize) );
                    //free(recvbuf);
					//recvbuf = (char *)malloc(cui.newSize);

					sgct::Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

					if(recvbuf != NULL)
						sgct::MessageHandler::Instance()->print("Network: Buffer resized successfully!\n");
					else
						sgct::MessageHandler::Instance()->print("Network error: Buffer failed to resize!\n");

#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Done.\n");
#endif
				}
				else if( recvbuf[0] == core_sgct::SGCTNetwork::SyncHeader &&
					iResult >= static_cast<int>(core_sgct::SGCTNetwork::syncHeaderSize) &&
					nPtr->mDecoderCallbackFn != NULL)
				{
#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Parsing sync data...\n");
#endif
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

					nPtr->setRecvFrame( ci.i );

					//convert from uchar to uint32
					union
					{
						unsigned int ui;
						unsigned char c[4];
					} cui;

					cui.c[0] = recvbuf[5];
					cui.c[1] = recvbuf[6];
					cui.c[2] = recvbuf[7];
					cui.c[3] = recvbuf[8];

#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Package info: Frame = %d, Size = %u\n", ci.i, cui.ui);
#endif

					while( static_cast<unsigned int>(iResult) < cui.ui && iResult > 0)
					{
						sgct::MessageHandler::Instance()->print("Network: Waiting for additional data (Frame %d, data %d of %u)...\n", ci.i, iResult, cui.ui);

						int tempResult = recv( nPtr->mSocket, recvbuf + iResult, recvbuflen, 0);

						if(tempResult > 0)
							iResult += tempResult;
						else
						{
							sgct::MessageHandler::Instance()->print("Network: Fatal network error!\n");
							//0 or SOCKET_ERROR (-1)
							iResult = tempResult;
							break;
						}

					}

					if( iResult > 0 )
						(nPtr->mDecoderCallbackFn)(recvbuf+core_sgct::SGCTNetwork::syncHeaderSize,
							iResult-core_sgct::SGCTNetwork::syncHeaderSize,
							nPtr->getId());

					sgct::Engine::signalCond( core_sgct::NetworkManager::gCond );
#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Done.\n");
#endif
				}
				else if( recvbuf[0] == core_sgct::SGCTNetwork::ConnectedHeader &&
					nPtr->mConnectedCallbackFn != NULL)
				{
#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Signaling slave is connected... ");
#endif
					(nPtr->mConnectedCallbackFn)();
					sgct::Engine::signalCond( core_sgct::NetworkManager::gCond );
#ifdef __SGCT_DEBUG__
                    sgct::MessageHandler::Instance()->print("Done.\n");
#endif
				}
			}
			else if(nPtr->getTypeOfServer() == core_sgct::SGCTNetwork::ExternalControl)
			{
#ifdef __SGCT_DEBUG__
                sgct::MessageHandler::Instance()->print("Parsing external tcp data... ");
#endif
				std::string tmpStr(recvbuf);
				extBuffer += tmpStr.substr(0, iResult);
				std::size_t found = extBuffer.find("\r\n");
				while( found != std::string::npos )
				{
					std::string extMessage = extBuffer.substr(0,found);
					extBuffer = extBuffer.substr(found+2);//jump over \r\n

					sgct::Engine::lockMutex(core_sgct::NetworkManager::gMutex);
					if( nPtr->mDecoderCallbackFn != NULL )
					{
						(nPtr->mDecoderCallbackFn)(extMessage.c_str(), extMessage.size(), nPtr->getId());
					}
					nPtr->sendStr("OK\r\n");
					sgct::Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
                    found = extBuffer.find("\r\n");
				}
#ifdef __SGCT_DEBUG__
                sgct::MessageHandler::Instance()->print("Done.\n");
#endif
			}
		}
		else if (iResult == 0)
		{
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Setting connection status to false... ");
#endif
			nPtr->setConnectedStatus(false);
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Done.\n");
#endif
			sgct::MessageHandler::Instance()->print("TCP Connection %d closed.\n", nPtr->getId());
		}
		else
		{
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Setting connection status to false... ");
#endif
			nPtr->setConnectedStatus(false);
#ifdef __SGCT_DEBUG__
            sgct::MessageHandler::Instance()->print("Done.\n");
#endif

#ifdef __WIN32__
            sgct::MessageHandler::Instance()->print("TCP connection %d recv failed: %d\n", nPtr->getId(), WSAGetLastError());
#else
            sgct::MessageHandler::Instance()->print("TCP connection %d recv failed: %d\n", nPtr->getId(), errno);
#endif
		}
	} while (iResult > 0 || nPtr->isConnected());

	//wait
	glfwSleep(1.0);

	//cleanup
	sgct::Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	if( recvbuf != NULL )
		free(recvbuf);
	sgct::Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	//wait
	glfwSleep(0.25);

	//Close socket
	nPtr->closeSocket( nPtr->mSocket );

	//wait
	glfwSleep(0.25);

	if(nPtr->mUpdateCallbackFn != NULL)
		nPtr->mUpdateCallbackFn(nPtr->getId(), false);

	sgct::MessageHandler::Instance()->print("Network: Disconnected!\n");
}

int core_sgct::SGCTNetwork::sendData(void * data, int length)
{
	//fprintf(stderr, "Send data size: %d\n", length);
	return send(mSocket, (const char *)data, length, 0);
}

int core_sgct::SGCTNetwork::sendStr(std::string msg)
{
	//fprintf(stderr, "Send message: %s, size: %d\n", msg.c_str(), msg.size());
	return send(mSocket, msg.c_str(), msg.size(), 0);
}

void core_sgct::SGCTNetwork::closeNetwork()
{
	if( isConnected() )
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
	}

	mDecoderCallbackFn = NULL;

	mConnected = false;
	sgct::MessageHandler::Instance()->print("Closing connection %d... ", getId());
	if( mCommThreadId != -1 )
		glfwDestroyThread(mCommThreadId); //blocking sockets -> cannot wait for thread so just kill it brutally

	if( mMainThreadId != -1 )
		glfwDestroyThread( mMainThreadId );

	closeSocket( mSocket );
	closeSocket( mListenSocket );

	sgct::MessageHandler::Instance()->print(" Done!\n");
}
