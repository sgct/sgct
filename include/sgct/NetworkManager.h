/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _NETWORK_MANAGER_H_
#define _NETWORK_MANAGER_H_

#include "SGCTNetwork.h"
#include "Statistics.h"
#include <vector>
#include <string>

typedef void * GLFWmutex;
typedef void * GLFWcond;

namespace sgct_core
{
/*!
	The network manager manages all network connections for SGCT.
*/
class NetworkManager
{
public:
	/*!
		Different sync stages. Server sync data to clients and clients syncs acknowlagement
	*/
	enum SyncMode { SendDataToClients = 0, AcknowledgeData };

	NetworkManager(int mode);
	~NetworkManager();
	bool init();
	void sync(SyncMode sm, Statistics * statsPtr);
	bool isSyncComplete();
	void close();

	bool matchHostName(const std::string name);
	bool matchAddress(const std::string ip);
	bool isComputerServer() { return mIsServer; }
	bool isRunning() { return mIsRunning; }
	bool areAllNodesConnected() { return mAllNodesConnected; }
	SGCTNetwork * getExternalControlPtr();

	unsigned int getConnectionsCount() { return mNumberOfConnections; }
	unsigned int getSyncConnectionsCount() { return mNumberOfSyncConnections; }
	inline SGCTNetwork* getConnection(unsigned int index) { return mNetworkConnections[index]; }

private:
	bool addConnection(const std::string port, const std::string ip, int serverType = SGCTNetwork::SyncServer);
	void initAPI();
	void getHostInfo();
	void updateConnectionStatus(int index);
	void setAllNodesConnected();

public:
	enum ManagerMode { NotLocal = 0, LocalServer, LocalClient };
	static GLFWmutex gMutex;
	static GLFWmutex gSyncMutex;
	static GLFWcond gCond;

private:
	std::vector<SGCTNetwork*> mNetworkConnections;

	std::string hostName; //stores this computers hostname
	std::vector<std::string> localAddresses; //stors this computers ip addresses

	bool mIsServer;
	bool mIsRunning;
	bool mIsExternalControlPresent;
	bool mAllNodesConnected;
	int mMode;
	unsigned int mNumberOfConnections;
	unsigned int mNumberOfSyncConnections;
};

}

#endif
