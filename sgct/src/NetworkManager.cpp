#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/NodeManager.h"
#include "../include/sgct/SharedData.h"
#include <ws2tcpip.h>
#include <GL/glfw.h>

core_sgct::NetworkManager::NetworkManager(int mode)
{
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
		sgct::MessageHandler::Instance()->print("Initiating network API failed! Error: '%s'\n", err);
	}

	sgct::MessageHandler::Instance()->print("Getting host info...\n");
	try
	{
		getHostInfo();
	}
	catch(const char * err)
	{
		sgct::MessageHandler::Instance()->print("Getting host info failed! Error: '%s'\n", err);
	}

	if(mMode == NotLocal)
		mIsServer = matchAddress( (*NodeManager::Instance()->getMasterIp()));
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
		localAddresses.push_back(NodeManager::Instance()->getThisNodePtr()->ip);

	std::string tmpIp;
	if( mMode == NotLocal )
		tmpIp = *NodeManager::Instance()->getMasterIp();
	else
		tmpIp = "127.0.0.1";
	
	//add connection for external communication
	if( mIsServer )
	{
		if(!addConnection( (*NodeManager::Instance()->getExternalControlPort()),
		"127.0.0.1", SGCTNetwork::ExternalControl))
		{
			sgct::MessageHandler::Instance()->print("Failed to add external connection!\n");
		}
		else //bind
		{
			mIsExternalControlPresent = true;

			/*std::tr1::function< void(const char*, int, int) > callback;
			callback = std::tr1::bind(&sgct::Engine::decodeExternalControl, enginePtr,
				std::tr1::placeholders::_1,
				std::tr1::placeholders::_2,
				std::tr1::placeholders::_3);
			mNetworkConnections[mNetworkConnections.size()-1]->setDecodeFunction(callback);*/
		}
	}
	else //client
	{
		if(!addConnection(NodeManager::Instance()->getThisNodePtr()->port, tmpIp))
		{
			sgct::MessageHandler::Instance()->print("Failed to add network connection to %s!\n", NodeManager::Instance()->getMasterIp()->c_str());
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
	for(unsigned int i=0; i<NodeManager::Instance()->getNumberOfNodes(); i++)
	{
		//dont add itself if server
		if( mIsServer && !matchAddress( NodeManager::Instance()->getNodePtr(i)->ip ))
		{
			if(!addConnection(NodeManager::Instance()->getNodePtr(i)->port, tmpIp))
			{
				sgct::MessageHandler::Instance()->print("Failed to add network connection to %s!\n", NodeManager::Instance()->getNodePtr(i)->ip.c_str());
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
	int counter = 0;
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

void core_sgct::NetworkManager::updateConnectionStatus()
{
	int counter = 0;
	int specificCounter = 0;
	
	for(unsigned int i=0; i<mNetworkConnections.size(); i++)
	if( mNetworkConnections[i]->isConnected() )
	{
		counter++;
		if(mNetworkConnections[i]->getTypeOfServer() == SGCTNetwork::SyncServer)
			specificCounter++;
	}

	mNumberOfConnectedNodes = counter;

	mAllNodesConnected = (specificCounter == NodeManager::Instance()->getNumberOfNodes()-1);

	if(mNumberOfConnectedNodes == 0 && !mIsServer)
		mIsRunning = false;
}

void core_sgct::NetworkManager::close()
{
	mIsRunning = false;

	for(unsigned int i=0; i < mNetworkConnections.size(); i++)
	{
		if(mNetworkConnections[i] != NULL)
		{
			mNetworkConnections[i]->close();
			delete mNetworkConnections[i];
		}
	}

	mNetworkConnections.clear();

	WSACleanup();
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
			netPtr->close();
		return false;
	}

	try
	{
		sgct::MessageHandler::Instance()->print("Initiating network connection %d at port %s.\n", mNetworkConnections.size(), port.c_str());
		netPtr->init(port, ip, mIsServer, static_cast<int>(mNetworkConnections.size()), serverType);

		//bind callback
		std::tr1::function< void(void) > callback;
		callback = std::tr1::bind(&core_sgct::NetworkManager::updateConnectionStatus, this);
		netPtr->setDisconnectedFunction(callback);
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
}

void core_sgct::NetworkManager::getHostInfo()
{
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