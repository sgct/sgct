/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _NETWORK_MANAGER_H_
#define _NETWORK_MANAGER_H_

#include "SGCTNetwork.h"
#include "Statistics.h"
#include <vector>
#include <string>

#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/tinythread.h"
#else
#include <tinythread.h>
#endif

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

	/*!
		\returns the static pointer to the NetworkManager instance
	*/
	static NetworkManager * instance() { return mInstance; }

	bool matchAddress(const std::string address);
	void retrieveNodeId();
	bool isComputerServer();
	bool isRunning();
	bool areAllNodesConnected();
	SGCTNetwork * getExternalControlPtr();

	unsigned int getActiveConnectionsCount();
	unsigned int getActiveSyncConnectionsCount();
    unsigned int getConnectionsCount();
	unsigned int getSyncConnectionsCount();
	inline SGCTNetwork* getConnection(unsigned int index) { return mNetworkConnections[index]; }
    inline SGCTNetwork* getSyncConnection(unsigned int index) { return mSyncConnections[index]; }
	inline std::vector<std::string> getLocalAddresses() { return localAddresses; }

private:
	bool addConnection(const std::string & port, const std::string & address, SGCTNetwork::ConnectionTypes connectionType = SGCTNetwork::SyncConnection);
	void initAPI();
	void getHostInfo();
	void updateConnectionStatus(int index);
	void setAllNodesConnected();

public:
	enum ManagerMode { Remote = 0, LocalServer, LocalClient };
	static tthread::condition_variable gCond;

private:
	static NetworkManager * mInstance;
	std::vector<SGCTNetwork*> mNetworkConnections;
    std::vector<SGCTNetwork*> mSyncConnections;
    std::vector<SGCTNetwork*> mExternalConnections;

	std::string mHostName; //stores this computers hostname
	std::string mDNSName;
	std::vector<std::string> localAddresses; //stors this computers ip addresses

	bool mIsServer;
	bool mIsRunning;
	bool mAllNodesConnected;
	int mMode;
	unsigned int mNumberOfActiveConnections;
	unsigned int mNumberOfActiveSyncConnections;
};

}

#endif
