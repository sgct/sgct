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

#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/Engine.h"

#ifdef __WIN32__ //WinSock
    #include <ws2tcpip.h>
#else //Use BSD sockets
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
	#define SOCKET_ERROR (-1)
#endif

#include <GL/glfw.h>

GLFWmutex core_sgct::NetworkManager::gMutex = NULL;
GLFWmutex core_sgct::NetworkManager::gSyncMutex = NULL;
GLFWcond core_sgct::NetworkManager::gCond = NULL;

core_sgct::NetworkManager::NetworkManager(int mode)
{
	mNumberOfConnections = 0;
	mAllNodesConnected = false;
	mIsExternalControlPresent = false;
	mIsRunning = true;

	mMode = mode;

	sgct::MessageHandler::Instance()->print("Initiating network API...\n");
	try
	{
		initAPI();
	}
	catch(const char * err)
	{
		throw err;
	}

	sgct::MessageHandler::Instance()->print("Getting host info...\n");
	try
	{
		getHostInfo();
	}
	catch(const char * err)
	{
		throw err;
	}

	if(mMode == NotLocal)
		mIsServer = matchAddress( (*ClusterManager::Instance()->getMasterIp()));
	else if(mMode == LocalServer)
		mIsServer = true;
	else
		mIsServer = false;

	if( mIsServer )
		sgct::MessageHandler::Instance()->print("This computer is the network server.\n");
	else
		sgct::MessageHandler::Instance()->print("This computer is the network client.\n");
}

core_sgct::NetworkManager::~NetworkManager()
{
	close();
}

bool core_sgct::NetworkManager::init()
{
	//if faking an address (running local) then add it to the search list
	if(mMode != NotLocal)
		localAddresses.push_back(ClusterManager::Instance()->getThisNodePtr()->ip);

	std::string tmpIp;
	if( mMode == NotLocal )
		tmpIp = *ClusterManager::Instance()->getMasterIp();
	else
		tmpIp = "127.0.0.1";

	//if client
	if( !mIsServer )
	{
		if(addConnection(ClusterManager::Instance()->getThisNodePtr()->port, tmpIp))
		{
			//bind
			std::tr1::function< void(const char*, int, int) > callback;
			callback = std::tr1::bind(&sgct::SharedData::decode, sgct::SharedData::Instance(),
				std::tr1::placeholders::_1,
				std::tr1::placeholders::_2,
				std::tr1::placeholders::_3);
			mNetworkConnections[mNetworkConnections.size()-1]->setDecodeFunction(callback);
		}
		else
		{
			sgct::MessageHandler::Instance()->print("Failed to add network connection to %s!\n", ClusterManager::Instance()->getMasterIp()->c_str());
			return false;
		}
	}

	//add all connections from config file
	for(unsigned int i=0; i<ClusterManager::Instance()->getNumberOfNodes(); i++)
	{
		//dont add itself if server
		if( mIsServer && !matchAddress( ClusterManager::Instance()->getNodePtr(i)->ip ))
		{
			if(!addConnection(ClusterManager::Instance()->getNodePtr(i)->port, tmpIp))
			{
				sgct::MessageHandler::Instance()->print("Failed to add network connection to %s!\n", ClusterManager::Instance()->getNodePtr(i)->ip.c_str());
				return false;
			}
			else //bind
			{
				std::tr1::function< void(const char*, int, int) > callback;
				callback = std::tr1::bind(&sgct::MessageHandler::decode, sgct::MessageHandler::Instance(),
					std::tr1::placeholders::_1,
					std::tr1::placeholders::_2,
					std::tr1::placeholders::_3);
				mNetworkConnections[mNetworkConnections.size()-1]->setDecodeFunction(callback);
			}
		}
	}

	//add connection for external communication
	if( mIsServer )
	{
		if(addConnection( (*ClusterManager::Instance()->getExternalControlPort()),
            "127.0.0.1", SGCTNetwork::ExternalControl))
		{
			mIsExternalControlPresent = true;

			std::tr1::function< void(const char*, int, int) > callback;
			callback = std::tr1::bind(&sgct::Engine::decodeExternalControl, sgct::Engine::getPtr(),
				std::tr1::placeholders::_1,
				std::tr1::placeholders::_2,
				std::tr1::placeholders::_3);
			mNetworkConnections[mNetworkConnections.size()-1]->setDecodeFunction(callback);
		}
	}

	return true;
}

void core_sgct::NetworkManager::sync()
{
	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
	{
		if( mNetworkConnections[i]->isConnected() &&
			mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer &&
			mNetworkConnections[i]->isServer())
		{
			unsigned int currentSize =
                sgct::SharedData::Instance()->getDataSize() - core_sgct::SGCTNetwork::mHeaderSize;

			//iterate counter
			mNetworkConnections[i]->iterateFrameCounter();

            //set bytes in header
			int currentFrame = mNetworkConnections[i]->getSendFrame();

			sgct::Engine::lockMutex(gMutex);
				unsigned char *currentFrameDataPtr = (unsigned char *)&currentFrame;
				unsigned char *currentSizeDataPtr = (unsigned char *)&currentSize;

				sgct::SharedData::Instance()->getDataBlock()[0] = SGCTNetwork::SyncByte;
				sgct::SharedData::Instance()->getDataBlock()[1] = currentFrameDataPtr[0];
				sgct::SharedData::Instance()->getDataBlock()[2] = currentFrameDataPtr[1];
				sgct::SharedData::Instance()->getDataBlock()[3] = currentFrameDataPtr[2];
				sgct::SharedData::Instance()->getDataBlock()[4] = currentFrameDataPtr[3];
				sgct::SharedData::Instance()->getDataBlock()[5] = currentSizeDataPtr[0];
				sgct::SharedData::Instance()->getDataBlock()[6] = currentSizeDataPtr[1];
				sgct::SharedData::Instance()->getDataBlock()[7] = currentSizeDataPtr[2];
				sgct::SharedData::Instance()->getDataBlock()[8] = currentSizeDataPtr[3];

				//sgct::MessageHandler::Instance()->print("NetworkManager::sync size %u\n", currentSize);

				//send
				int sendErr = mNetworkConnections[i]->sendData(
                                sgct::SharedData::Instance()->getDataBlock(),
                                sgct::SharedData::Instance()->getDataSize() );
			sgct::Engine::unlockMutex(gMutex);

            if (sendErr == SOCKET_ERROR)
                sgct::MessageHandler::Instance()->print("Send data failed!\n");

		}
		//Client
		else if( mNetworkConnections[i]->isConnected() &&
				 mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer)
		{
			//The servers's render function is locked until a message starting with the ack-byte is received.
			//iterate counter
			mNetworkConnections[i]->iterateFrameCounter();

			mNetworkConnections[i]->pushClientMessage();
        }
	}
}

bool core_sgct::NetworkManager::isSyncComplete()
{
	unsigned int counter = 0;
	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		if(mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer &&
			!mNetworkConnections[i]->compareFrames())
		{
			counter++;
		}

	return counter == getConnectionsCount();
}

core_sgct::SGCTNetwork * core_sgct::NetworkManager::getExternalControlPtr()
{ 
	core_sgct::SGCTNetwork * netPtr = NULL;

	if( mIsExternalControlPresent )
	{
		for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		{
			if( mNetworkConnections[i]->getTypeOfServer() == core_sgct::SGCTNetwork::ExternalControl )
				return mNetworkConnections[i];
		}
	}

	return netPtr;
}

void core_sgct::NetworkManager::swapData()
{
	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		mNetworkConnections[i]->swapFrames();
}

void core_sgct::NetworkManager::updateConnectionStatus(int index)
{
	unsigned int numberOfConnectionsCounter = 0;
	unsigned int numberOfConnectedSyncNodesCounter = 0;

    //count connections
	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
	{
		if( mNetworkConnections[i]->isConnected() )
		{
			numberOfConnectionsCounter++;
			if(mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer)
				numberOfConnectedSyncNodesCounter++;
		}
	}

	sgct::Engine::lockMutex(gMutex);
        mNumberOfConnections = numberOfConnectionsCounter;
        //create a local copy to use so we don't need mutex on several locations
        bool isServer = mIsServer;

        //if clients disconnect it's not longer running
        if(mNumberOfConnections == 0 && !isServer)
            mIsRunning = false;
    sgct::Engine::unlockMutex(gMutex);

	if(isServer)
	{
		sgct::Engine::lockMutex(gMutex);
            //local copy (thread safe) -- don't count server, therefore -1
            unsigned int numberOfSlavesInConfig = ClusterManager::Instance()->getNumberOfNodes()-1;
            //local copy (thread safe)
            bool allNodesConnectedCopy =
                (numberOfConnectedSyncNodesCounter == numberOfSlavesInConfig);
            mAllNodesConnected = allNodesConnectedCopy;
        sgct::Engine::unlockMutex(gMutex);

        //send cluster connected message to nodes/slaves
		if(allNodesConnectedCopy)
			for(unsigned int i=0; i<mNetworkConnections.size(); i++)
				if( mNetworkConnections[i]->isConnected() )
				{
					if( mNetworkConnections[i]->getTypeOfServer() == core_sgct::SGCTNetwork::SyncServer )
					{
						char tmpc[SGCTNetwork::mHeaderSize];
						tmpc[0] = SGCTNetwork::ConnectedByte;
						for(unsigned int j=1; j<SGCTNetwork::mHeaderSize; j++)
							tmpc[j] = SGCTNetwork::FillByte;
					
						int sendErr = mNetworkConnections[i]->sendData(&tmpc, SGCTNetwork::mHeaderSize);
						if (sendErr == SOCKET_ERROR)
							sgct::MessageHandler::Instance()->print("Send connect data failed!\n");
					}
				}

		if( mNetworkConnections[index]->getTypeOfServer() == core_sgct::SGCTNetwork::ExternalControl &&
			 mNetworkConnections[index]->isConnected())
		{
			int sendErr = mNetworkConnections[index]->sendStr("Connected to SGCT!\r\n");
			if (sendErr == SOCKET_ERROR)
							sgct::MessageHandler::Instance()->print("Send connect data failed!\n");
		}

        sgct::MessageHandler::Instance()->print("Number of connections: %u (IG slaves %u of %u)\n",
             numberOfConnectionsCounter,
             numberOfConnectedSyncNodesCounter,
             numberOfSlavesInConfig);
	}

	//wake up the connection handler thread on server
	//if node disconnects to enable reconnection
	if( isServer )
	{
		sgct::Engine::signalCond( mNetworkConnections[index]->mStartConnectionCond );
	}

	//signal done to caller
	sgct::Engine::signalCond( mNetworkConnections[index]->mDoneCond );
}

void core_sgct::NetworkManager::setAllNodesConnected()
{
	mAllNodesConnected = true;
}

void core_sgct::NetworkManager::close()
{
	mIsRunning = false;

    //signal to terminate
	for(unsigned int i=0; i < mNetworkConnections.size(); i++)
		if(mNetworkConnections[i] != NULL)
		{
			mNetworkConnections[i]->initShutdown();
		}

    //wait for threads to die
	for(unsigned int i=0; i < mNetworkConnections.size(); i++)
		if(mNetworkConnections[i] != NULL)
		{
			mNetworkConnections[i]->closeNetwork(false);
			delete mNetworkConnections[i];
		}

	mNetworkConnections.clear();

#ifdef __WIN32__
    WSACleanup();
#else
    //No cleanup needed
#endif
	sgct::MessageHandler::Instance()->print("Network API closed!\n");
}

bool core_sgct::NetworkManager::addConnection(const std::string port, const std::string ip, int serverType)
{
	SGCTNetwork * netPtr = NULL;

	if(port.empty() || ip.empty())
		return false;

	try
	{
		netPtr = new SGCTNetwork();
	}
	catch( const char * err )
	{
		sgct::MessageHandler::Instance()->print("Network error: %s\n", err);
		if(netPtr != NULL)
		{
		    netPtr->initShutdown();
		    glfwSleep(1.0);
		    netPtr->closeNetwork(true);
		}
		return false;
	}

	try
	{
		sgct::MessageHandler::Instance()->print("Initiating network connection %d at port %s.\n", mNetworkConnections.size(), port.c_str());
		netPtr->init(port, ip, mIsServer, static_cast<int>(mNetworkConnections.size()), serverType);

		//bind callback
		std::tr1::function< void(int) > updateCallback;
		updateCallback = std::tr1::bind(&core_sgct::NetworkManager::updateConnectionStatus, this,
			std::tr1::placeholders::_1);
		netPtr->setUpdateFunction(updateCallback);

		//bind callback
		std::tr1::function< void(void) > connectedCallback;
		connectedCallback = std::tr1::bind(&core_sgct::NetworkManager::setAllNodesConnected, this);
		netPtr->setConnectedFunction(connectedCallback);
    }
    catch( const char * err )
    {
        sgct::MessageHandler::Instance()->print("Network error: %s\n", err);
        return false;
    }

	mNetworkConnections.push_back(netPtr);
	return true;
}

void core_sgct::NetworkManager::initAPI()
{

#ifdef __WIN32__
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
#else
    //No init needed
#endif

}

void core_sgct::NetworkManager::getHostInfo()
{
	//get name & local ips
	char tmpStr[128];
    if (gethostname(tmpStr, sizeof(tmpStr)) == SOCKET_ERROR)
	{
#ifdef __WIN32__
        WSACleanup();
#else
        //No cleanup needed
#endif
		throw "Failed to get host name!";
    }
	hostName.assign(tmpStr);

	struct hostent *phe = gethostbyname(tmpStr);
    if (phe == 0)
	{
#ifdef __WIN32__
        WSACleanup();
#else
        //No cleanup needed
#endif
      	throw "Bad host lockup!";
    }

    for (int i = 0; phe->h_addr_list[i] != 0; ++i)
	{
        struct in_addr addr;
        memcpy(&addr, phe->h_addr_list[i], sizeof(struct in_addr));
		localAddresses.push_back( inet_ntoa(addr) );
    }

	//add the loop-back
	localAddresses.push_back("127.0.0.1");
}

bool core_sgct::NetworkManager::matchHostName(const std::string name)
{
	return strcmp(name.c_str(), hostName.c_str() ) == 0;
}

bool core_sgct::NetworkManager::matchAddress(const std::string ip)
{
	for( unsigned int i=0; i<localAddresses.size(); i++)
		if( strcmp(ip.c_str(), localAddresses[i].c_str()) == 0 )
			return true;
	//No match
	return false;
}
