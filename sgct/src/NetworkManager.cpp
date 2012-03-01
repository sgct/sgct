#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/Engine.h"
#include <GL/glfw.h>

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

GLFWmutex core_sgct::NetworkManager::gDecoderMutex = NULL;
GLFWcond core_sgct::NetworkManager::gCond = NULL;
GLFWcond core_sgct::NetworkManager::gStartConnectionCond = NULL;

core_sgct::NetworkManager::NetworkManager(int mode)
{
	gDecoderMutex = glfwCreateMutex();
	gCond = glfwCreateCond();
	gStartConnectionCond = glfwCreateCond();
	if(gDecoderMutex == NULL ||
		gCond == NULL ||
		gStartConnectionCond == NULL)
		throw "Failed to create thread objects!";

	mNumberOfConnectedNodes = 0;
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

	//add connection for external communication
	if( mIsServer )
	{
		if(!addConnection( (*ClusterManager::Instance()->getExternalControlPort()),
		"127.0.0.1", SGCTNetwork::ExternalControl))
		{
			sgct::MessageHandler::Instance()->print("Failed to add external connection!\n");
		}
		else //bind
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
	else //client
	{
		if(!addConnection(ClusterManager::Instance()->getThisNodePtr()->port, tmpIp))
		{
			sgct::MessageHandler::Instance()->print("Failed to add network connection to %s!\n", ClusterManager::Instance()->getMasterIp()->c_str());
			return false;
		}
		else //bind
		{
			std::tr1::function< void(const char*, int, int) > callback;
			callback = std::tr1::bind(&sgct::SharedData::decode, sgct::SharedData::Instance(),
				std::tr1::placeholders::_1,
				std::tr1::placeholders::_2,
				std::tr1::placeholders::_3);
			mNetworkConnections[mNetworkConnections.size()-1]->setDecodeFunction(callback);
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
			mNetworkConnections[i]->checkIfBufferNeedsResizing();

			//iterate counter
			mNetworkConnections[i]->iterateFrameCounter();

			glfwLockMutex( gDecoderMutex );
				//set bytes in header
				int currentFrame = mNetworkConnections[i]->getSendFrame();
				unsigned char *p = (unsigned char *)&currentFrame;
				sgct::SharedData::Instance()->getDataBlock()[0] = SGCTNetwork::SyncHeader;
				sgct::SharedData::Instance()->getDataBlock()[1] = p[0];
				sgct::SharedData::Instance()->getDataBlock()[2] = p[1];
				sgct::SharedData::Instance()->getDataBlock()[3] = p[2];
				sgct::SharedData::Instance()->getDataBlock()[4] = p[3];

				//send
				mNetworkConnections[i]->sendData( sgct::SharedData::Instance()->getDataBlock(), sgct::SharedData::Instance()->getDataSize() );
			glfwUnlockMutex( gDecoderMutex );
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

	return counter == getConnectedNodeCount();
}

void core_sgct::NetworkManager::swapData()
{
	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
		mNetworkConnections[i]->swapFrames();
}

void core_sgct::NetworkManager::updateConnectionStatus(int index, bool connected)
{
	unsigned int counter = 0;
	unsigned int specificCounter = 0;

	if( connected )
	{
		counter++;
		if(mNetworkConnections[index]->getTypeOfServer() == SGCTNetwork::SyncServer)
			specificCounter++;
	}

	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
	{
		if( i != static_cast<unsigned int>(index) &&
            mNetworkConnections[i]->isConnected() )
		{
			counter++;
			if(mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer)
				specificCounter++;
		}
	}
	mNumberOfConnectedNodes = counter;

	if(mNumberOfConnectedNodes == 0 && !mIsServer)
		mIsRunning = false;

	sgct::MessageHandler::Instance()->print("Number of connections: %d\n", counter);

	if(mIsServer)
	{
		mAllNodesConnected = (specificCounter == (ClusterManager::Instance()->getNumberOfNodes()-1));

		if(mAllNodesConnected)
			for(unsigned int i=0; i<mNetworkConnections.size(); i++)
				if( mNetworkConnections[i]->isConnected() )
				{
					char tmpc = SGCTNetwork::ConnectedHeader;
					mNetworkConnections[i]->sendData(&tmpc, 1);
				}
	}

	//wake up the connection handler thread
	if( mNetworkConnections[index]->isServer() )
	{
		glfwSignalCond( gStartConnectionCond );
	}
}

void core_sgct::NetworkManager::setAllNodesConnected()
{
	mAllNodesConnected = true;
}

void core_sgct::NetworkManager::close()
{
	mIsRunning = false;

	for(unsigned int i=0; i < mNetworkConnections.size(); i++)
	{
		if(mNetworkConnections[i] != NULL)
		{
			mNetworkConnections[i]->closeSGCT();
			delete mNetworkConnections[i];
		}
	}

	mNetworkConnections.clear();

	if( gCond != NULL )
	{
		glfwDestroyCond( gCond );
		gCond = NULL;
	}
	if( gStartConnectionCond != NULL )
	{
		glfwDestroyCond( gStartConnectionCond );
		gStartConnectionCond = NULL;
	}
	if( gDecoderMutex != NULL )
	{
		glfwDestroyMutex( gDecoderMutex );
		gDecoderMutex = NULL;
	}

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
			netPtr->closeSGCT();
		return false;
	}

	try
	{
		sgct::MessageHandler::Instance()->print("Initiating network connection %d at port %s.\n", mNetworkConnections.size(), port.c_str());
		netPtr->init(port, ip, mIsServer, static_cast<int>(mNetworkConnections.size()), serverType);

		//bind callback
		std::tr1::function< void(int,bool) > updateCallback;
		updateCallback = std::tr1::bind(&core_sgct::NetworkManager::updateConnectionStatus,
			this,
			std::tr1::placeholders::_1,
			std::tr1::placeholders::_2);
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
