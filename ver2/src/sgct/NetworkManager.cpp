/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
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

//#define __SGCT_NETWORK_DEBUG__

tthread::condition_variable sgct_core::NetworkManager::gCond;

sgct_core::NetworkManager::NetworkManager(int mode)
{
	mNumberOfConnections = 0;
	mNumberOfSyncConnections = 0;
	mAllNodesConnected = false;
	mIsExternalControlPresent = false;
	mIsRunning = true;
	mIsServer = true;

	mMode = mode;

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Initiating network API...\n");
	try
	{
		initAPI();
	}
	catch(const char * err)
	{
		throw err;
	}

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Getting host info...\n");
	try
	{
		getHostInfo();
	}
	catch(const char * err)
	{
		throw err;
	}

	if(mMode == NotLocal)
		mIsServer = matchAddress( (*ClusterManager::instance()->getMasterIp()));
	else if(mMode == LocalServer)
		mIsServer = true;
	else
		mIsServer = false;

	if( mIsServer )
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "This computer is the network server.\n");
	else
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "This computer is the network client.\n");
}

sgct_core::NetworkManager::~NetworkManager()
{
	close();
}

bool sgct_core::NetworkManager::init()
{
	//if faking an address (running local) then add it to the search list
	if(mMode != NotLocal)
		localAddresses.push_back(ClusterManager::instance()->getThisNodePtr()->ip);

	std::string tmpIp;
	if( mMode == NotLocal )
		tmpIp = *ClusterManager::instance()->getMasterIp();
	else
		tmpIp = "127.0.0.1";

	//if client
	if( !mIsServer )
	{
		if(addConnection(ClusterManager::instance()->getThisNodePtr()->port, tmpIp))
		{
			//bind
			sgct_cppxeleven::function< void(const char*, int, int) > callback;
			callback = sgct_cppxeleven::bind(&sgct::SharedData::decode, sgct::SharedData::instance(),
				sgct_cppxeleven::placeholders::_1,
				sgct_cppxeleven::placeholders::_2,
				sgct_cppxeleven::placeholders::_3);
			mNetworkConnections[mNetworkConnections.size()-1]->setDecodeFunction(callback);
		}
		else
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to add network connection to %s!\n", ClusterManager::instance()->getMasterIp()->c_str());
			return false;
		}
	}

	//add all connections from config file
	for(unsigned int i=0; i<ClusterManager::instance()->getNumberOfNodes(); i++)
	{
		//dont add itself if server
		if( mIsServer && !matchAddress( ClusterManager::instance()->getNodePtr(i)->ip ))
		{
			if(!addConnection(ClusterManager::instance()->getNodePtr(i)->port, tmpIp))
			{
				sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to add network connection to %s!\n", ClusterManager::instance()->getNodePtr(i)->ip.c_str());
				return false;
			}
			else //bind
			{
				sgct_cppxeleven::function< void(const char*, int, int) > callback;
				callback = sgct_cppxeleven::bind(&sgct::MessageHandler::decode, sgct::MessageHandler::instance(),
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
		if(addConnection( (*ClusterManager::instance()->getExternalControlPort()),
            "127.0.0.1", SGCTNetwork::ExternalControlConnection))
		{
			mIsExternalControlPresent = true;

			sgct_cppxeleven::function< void(const char*, int, int) > callback;
			callback = sgct_cppxeleven::bind(&sgct::Engine::decodeExternalControl, sgct::Engine::instance(),
				sgct_cppxeleven::placeholders::_1,
				sgct_cppxeleven::placeholders::_2,
				sgct_cppxeleven::placeholders::_3);
			mNetworkConnections[mNetworkConnections.size()-1]->setDecodeFunction(callback);
		}
	}

	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Cluster sync is set to %s\n",
        ClusterManager::instance()->getFirmFrameLockSyncStatus() ? "firm/strict" : "loose" );

	return true;
}

/*!
	\param if this application is server/master in cluster then set to true
*/
void sgct_core::NetworkManager::sync(sgct_core::NetworkManager::SyncMode sm, sgct_core::Statistics * statsPtr)
{
	if(sm == SendDataToClients)
	{
		double maxTime = -999999.0;
		double minTime = 999999.0;

		for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		{
			if( mNetworkConnections[i]->isServer() &&
				mNetworkConnections[i]->isConnected() &&
				mNetworkConnections[i]->getTypeOfConnection() == SGCTNetwork::SyncConnection)
			{
				//fprintf(stderr, "Connection: %u time: %lf ms\n", i, mNetworkConnections[i]->getLoopTime()*1000.0);

				double currentTime = mNetworkConnections[i]->getLoopTime();
				if( currentTime > maxTime )
					maxTime = currentTime;
				if( currentTime < minTime )
					minTime = currentTime;

				std::size_t currentSize =
					sgct::SharedData::instance()->getDataSize() - sgct_core::SGCTNetwork::mHeaderSize;

				//iterate counter
				int currentFrame = mNetworkConnections[i]->iterateFrameCounter();

				/* The server only writes the sync data and never reads, no need for mutex protection
				sgct::Engine::lockMutex(gMutex);*/
				unsigned char *currentFrameDataPtr = (unsigned char *)&currentFrame;
				unsigned char *currentSizeDataPtr = (unsigned char *)&currentSize;

				sgct::SharedData::instance()->getDataBlock()[0] = SGCTNetwork::SyncByte;
				sgct::SharedData::instance()->getDataBlock()[1] = currentFrameDataPtr[0];
				sgct::SharedData::instance()->getDataBlock()[2] = currentFrameDataPtr[1];
				sgct::SharedData::instance()->getDataBlock()[3] = currentFrameDataPtr[2];
				sgct::SharedData::instance()->getDataBlock()[4] = currentFrameDataPtr[3];
				sgct::SharedData::instance()->getDataBlock()[5] = currentSizeDataPtr[0];
				sgct::SharedData::instance()->getDataBlock()[6] = currentSizeDataPtr[1];
				sgct::SharedData::instance()->getDataBlock()[7] = currentSizeDataPtr[2];
				sgct::SharedData::instance()->getDataBlock()[8] = currentSizeDataPtr[3];

				//sgct::MessageHandler::instance()->print("NetworkManager::sync size %u\n", currentSize);

				//send
				mNetworkConnections[i]->sendData(
					sgct::SharedData::instance()->getDataBlock(),
					static_cast<int>(sgct::SharedData::instance()->getDataSize()) );
				//sgct::Engine::unlockMutex(gMutex);
			}
		}//end for

		if( isComputerServer() )
			statsPtr->setLoopTime(minTime, maxTime);
	}

	else if(sm == AcknowledgeData)
		for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		{
			//Client
			if( !mNetworkConnections[i]->isServer() &&
				mNetworkConnections[i]->isConnected() &&
				mNetworkConnections[i]->getTypeOfConnection() == SGCTNetwork::SyncConnection)
			{
				//The servers's render function is locked until a message starting with the ack-byte is received.

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
		if(mNetworkConnections[i]->getTypeOfConnection() == SGCTNetwork::SyncConnection &&
			mNetworkConnections[i]->isUpdated()) //has all data been received?
		{
			counter++;
		}

#ifdef __SGCT_NETWORK_DEBUG__
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SGCTNetworkManager::isSyncComplete: counter %u of %u\n",
		counter, getSyncConnectionsCount());
#endif

	return counter == getSyncConnectionsCount();
}

sgct_core::SGCTNetwork * sgct_core::NetworkManager::getExternalControlPtr()
{
	sgct_core::SGCTNetwork * netPtr = NULL;

	if( mIsExternalControlPresent )
	{
		for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		{
			if( mNetworkConnections[i]->getTypeOfConnection() == sgct_core::SGCTNetwork::ExternalControlConnection )
				return mNetworkConnections[i];
		}
	}

	return netPtr;
}

unsigned int sgct_core::NetworkManager::getConnectionsCount()
{
	unsigned int retVal;
	sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::MainMutex );
		retVal = mNumberOfConnections;
	sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::MainMutex );
	return retVal;
}
unsigned int sgct_core::NetworkManager::getSyncConnectionsCount()
{
	unsigned int retVal;
	sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::MainMutex );
		retVal = mNumberOfSyncConnections;
	sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::MainMutex );
	return retVal;
}

void sgct_core::NetworkManager::updateConnectionStatus(int index)
{
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "NetworkManager: updating connection status %d\n", index);
	
	unsigned int numberOfConnectionsCounter = 0;
	unsigned int numberOfConnectedSyncNodesCounter = 0;

    //count connections
	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
	{
		if( mNetworkConnections[i]->isConnected() )
		{
			numberOfConnectionsCounter++;
			if(mNetworkConnections[i]->getTypeOfConnection() == SGCTNetwork::SyncConnection)
				numberOfConnectedSyncNodesCounter++;
		}
	}

	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "NetworkManager: Number of active connections %u\n", numberOfConnectionsCounter);
	sgct::MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "NetworkManager: Number of connected sync nodes %u\n", numberOfConnectedSyncNodesCounter);

	sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::MainMutex );
        mNumberOfConnections = numberOfConnectionsCounter;
		mNumberOfSyncConnections = numberOfConnectedSyncNodesCounter;
        //create a local copy to use so we don't need mutex on several locations
        bool isServer = mIsServer;

        //if all clients disconnect it's not longer running
        if(mNumberOfConnections == 0 && !isServer)
            mIsRunning = false;
    sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::MainMutex );

	if(isServer)
	{
		sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::MainMutex );
            //local copy (thread safe) -- don't count server, therefore -1
            std::size_t numberOfSlavesInConfig = ClusterManager::instance()->getNumberOfNodes()-1;
            //local copy (thread safe)
            bool allNodesConnectedCopy =
                (numberOfConnectedSyncNodesCounter == numberOfSlavesInConfig);
            mAllNodesConnected = allNodesConnectedCopy;
        sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::MainMutex );

        //send cluster connected message to nodes/slaves
		if(allNodesConnectedCopy)
			for(unsigned int i=0; i<mNetworkConnections.size(); i++)
				if( mNetworkConnections[i]->isConnected() )
				{
					if( mNetworkConnections[i]->getTypeOfConnection() == sgct_core::SGCTNetwork::SyncConnection )
					{
						char tmpc[SGCTNetwork::mHeaderSize];
						tmpc[0] = SGCTNetwork::ConnectedByte;
						for(unsigned int j=1; j<SGCTNetwork::mHeaderSize; j++)
							tmpc[j] = SGCTNetwork::FillByte;

						mNetworkConnections[i]->sendData(&tmpc, SGCTNetwork::mHeaderSize);
					}
				}

		if( mNetworkConnections[index]->getTypeOfConnection() == sgct_core::SGCTNetwork::ExternalControlConnection &&
			 mNetworkConnections[index]->isConnected())
		{
			mNetworkConnections[index]->sendStr("Connected to SGCT!\r\n");
		}

        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Number of connections: %u (IG slaves %u of %u)\n",
             numberOfConnectionsCounter,
             numberOfConnectedSyncNodesCounter,
             numberOfSlavesInConfig);
	}

	//wake up the connection handler thread on server
	//if node disconnects to enable reconnection
	if( isServer )
	{
		mNetworkConnections[index]->mConnectionMutex.lock();
		mNetworkConnections[index]->mStartConnectionCond.notify_all();
		mNetworkConnections[index]->mConnectionMutex.unlock();
	}

	//signal done to caller
	sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::MainMutex );
	gCond.notify_all();
	sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::MainMutex );
}

void sgct_core::NetworkManager::setAllNodesConnected()
{
	mAllNodesConnected = true;
}

void sgct_core::NetworkManager::close()
{
	mIsRunning = false;

	//release condition variables
	sgct_core::NetworkManager::gCond.notify_all();

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
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Network API closed!\n");
}

bool sgct_core::NetworkManager::addConnection(const std::string port, const std::string ip, SGCTNetwork::ConnectionTypes connectionType)
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
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Network error: %s\n", err);
		if(netPtr != NULL)
		{
		    netPtr->initShutdown();
		    tthread::this_thread::sleep_for(tthread::chrono::seconds(1));
		    netPtr->closeNetwork(true);
		}
		return false;
	}

	try
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Initiating network connection %d at port %s.\n", mNetworkConnections.size(), port.c_str());
		netPtr->init(port, ip, mIsServer, static_cast<int>(mNetworkConnections.size()), connectionType,
			ClusterManager::instance()->getFirmFrameLockSyncStatus());

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
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Network error: %s\n", err);
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
