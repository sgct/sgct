/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/Engine.h"

#ifdef __WIN32__ //WinSock
    #include <ws2tcpip.h>
#else //Use BSD sockets
    #ifdef _XCODE
        #include <unistd.h>
    #endif
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
	#define SOCKET_ERROR (-1)
#endif

#include <GL/glfw.h>

GLFWmutex sgct_core::NetworkManager::gMutex = NULL;
GLFWmutex sgct_core::NetworkManager::gSyncMutex = NULL;
GLFWcond sgct_core::NetworkManager::gCond = NULL;

sgct_core::NetworkManager::NetworkManager(int mode)
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

sgct_core::NetworkManager::~NetworkManager()
{
	close();
}

bool sgct_core::NetworkManager::init()
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
			sgct_cppxeleven::function< void(const char*, int, int) > callback;
			callback = sgct_cppxeleven::bind(&sgct::SharedData::decode, sgct::SharedData::Instance(),
				sgct_cppxeleven::placeholders::_1,
				sgct_cppxeleven::placeholders::_2,
				sgct_cppxeleven::placeholders::_3);
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
				sgct_cppxeleven::function< void(const char*, int, int) > callback;
				callback = sgct_cppxeleven::bind(&sgct::MessageHandler::decode, sgct::MessageHandler::Instance(),
					sgct_cppxeleven::placeholders::_1,
					sgct_cppxeleven::placeholders::_2,
					sgct_cppxeleven::placeholders::_3);
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

			sgct_cppxeleven::function< void(const char*, int, int) > callback;
			callback = sgct_cppxeleven::bind(&sgct::Engine::decodeExternalControl, sgct::Engine::getPtr(),
				sgct_cppxeleven::placeholders::_1,
				sgct_cppxeleven::placeholders::_2,
				sgct_cppxeleven::placeholders::_3);
			mNetworkConnections[mNetworkConnections.size()-1]->setDecodeFunction(callback);
		}
	}

	return true;
}

/*!
	\param if this application is server/master in cluster then set to true
*/
void sgct_core::NetworkManager::sync(sgct_core::NetworkManager::SyncMode sm)
{
	if(sm == SendDataToClients)
		for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		{
			if( mNetworkConnections[i]->isServer() &&
				mNetworkConnections[i]->isConnected() &&
				mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer)
			{
				std::size_t currentSize =
					sgct::SharedData::Instance()->getDataSize() - sgct_core::SGCTNetwork::mHeaderSize;

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
					ssize_t sendErr = mNetworkConnections[i]->sendData(
									sgct::SharedData::Instance()->getDataBlock(),
									static_cast<int>(sgct::SharedData::Instance()->getDataSize()) );
				sgct::Engine::unlockMutex(gMutex);

				if (sendErr == SOCKET_ERROR)
					sgct::MessageHandler::Instance()->print("Send data failed!\n");

			}
		}

	else if(sm == AcknowledgeData)
		for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		{
			//Client
			if( !mNetworkConnections[i]->isServer() &&
				mNetworkConnections[i]->isConnected() &&
				mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer)
			{
				//The servers's render function is locked until a message starting with the ack-byte is received.
				//iterate counter
				mNetworkConnections[i]->iterateFrameCounter();

				//send message to server
				mNetworkConnections[i]->pushClientMessage();
			}
		}
}

/*!
	Compare if the last frame and current frames are different -> data update
	And if send frame == recieved frame
*/
bool sgct_core::NetworkManager::isSyncComplete()
{
	unsigned int counter = 0;
	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		if(mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer &&
			mNetworkConnections[i]->isUpdated()) //has all data been received?
		{
			counter++;
		}

	return counter == getConnectionsCount();
}

sgct_core::SGCTNetwork * sgct_core::NetworkManager::getExternalControlPtr()
{ 
	sgct_core::SGCTNetwork * netPtr = NULL;

	if( mIsExternalControlPresent )
	{
		for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		{
			if( mNetworkConnections[i]->getTypeOfServer() == sgct_core::SGCTNetwork::ExternalControl )
				return mNetworkConnections[i];
		}
	}

	return netPtr;
}

void sgct_core::NetworkManager::updateConnectionStatus(int index)
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
            std::size_t numberOfSlavesInConfig = ClusterManager::Instance()->getNumberOfNodes()-1;
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
					if( mNetworkConnections[i]->getTypeOfServer() == sgct_core::SGCTNetwork::SyncServer )
					{
						char tmpc[SGCTNetwork::mHeaderSize];
						tmpc[0] = SGCTNetwork::ConnectedByte;
						for(unsigned int j=1; j<SGCTNetwork::mHeaderSize; j++)
							tmpc[j] = SGCTNetwork::FillByte;
					
						ssize_t sendErr = mNetworkConnections[i]->sendData(&tmpc, SGCTNetwork::mHeaderSize);
						if (sendErr == SOCKET_ERROR)
							sgct::MessageHandler::Instance()->print("Send connect data failed!\n");
					}
				}

		if( mNetworkConnections[index]->getTypeOfServer() == sgct_core::SGCTNetwork::ExternalControl &&
			 mNetworkConnections[index]->isConnected())
		{
			ssize_t sendErr = mNetworkConnections[index]->sendStr("Connected to SGCT!\r\n");
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

void sgct_core::NetworkManager::setAllNodesConnected()
{
	mAllNodesConnected = true;
}

void sgct_core::NetworkManager::close()
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

bool sgct_core::NetworkManager::addConnection(const std::string port, const std::string ip, int serverType)
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
		sgct_cppxeleven::function< void(int) > updateCallback;
		updateCallback = sgct_cppxeleven::bind(&sgct_core::NetworkManager::updateConnectionStatus, this,
			sgct_cppxeleven::placeholders::_1);
		netPtr->setUpdateFunction(updateCallback);

		//bind callback
		sgct_cppxeleven::function< void(void) > connectedCallback;
		connectedCallback = sgct_cppxeleven::bind(&sgct_core::NetworkManager::setAllNodesConnected, this);
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

void sgct_core::NetworkManager::initAPI()
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

void sgct_core::NetworkManager::getHostInfo()
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

bool sgct_core::NetworkManager::matchHostName(const std::string name)
{
	return strcmp(name.c_str(), hostName.c_str() ) == 0;
}

bool sgct_core::NetworkManager::matchAddress(const std::string ip)
{
	for( unsigned int i=0; i<localAddresses.size(); i++)
		if( strcmp(ip.c_str(), localAddresses[i].c_str()) == 0 )
			return true;
	//No match
	return false;
}
