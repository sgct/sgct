/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
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
	enum NetworkMode { Remote = 0, LocalServer, LocalClient };

	NetworkManager(NetworkMode nm);
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
	void transferData(const void * data, int length, int packageId);
	void transferData(const void * data, int length, int packageId, std::size_t nodeIndex);
	void transferData(const void * data, int length, int packageId, SGCTNetwork * connection);
    void setDataTransferCompression(bool state, int level = 1);

	unsigned int getActiveConnectionsCount();
	unsigned int getActiveSyncConnectionsCount();
	unsigned int getActiveDataTransferConnectionsCount();
    unsigned int getConnectionsCount();
	unsigned int getSyncConnectionsCount();
	unsigned int getDataTransferConnectionsCount();
	inline SGCTNetwork* getConnectionByIndex(unsigned int index) const { return mNetworkConnections[index]; }
    inline SGCTNetwork* getSyncConnectionByIndex(unsigned int index) const { return mSyncConnections[index]; }
	inline std::vector<std::string> getLocalAddresses() { return mLocalAddresses; }

private:
	bool addConnection(const std::string & port, const std::string & address, SGCTNetwork::ConnectionTypes connectionType = SGCTNetwork::SyncConnection);
	void initAPI();
	void getHostInfo();
	void updateConnectionStatus(SGCTNetwork * connection);
	void setAllNodesConnected();
	bool prepareTransferData(const void * data, char ** bufferPtr, int & length, int packageId);

public:
	static tthread::condition_variable gCond;

private:
	static NetworkManager * mInstance;
	std::vector<SGCTNetwork*> mNetworkConnections;
    std::vector<SGCTNetwork*> mSyncConnections;
	std::vector<SGCTNetwork*> mDataTransferConnections;
	SGCTNetwork* mExternalControlConnection;

	std::string mHostName; //stores this computers hostname
	std::vector<std::string> mDNSNames;
	std::vector<std::string> mLocalAddresses; //stors this computers ip addresses

	bool mIsServer;
	bool mIsRunning;
	bool mAllNodesConnected;
	tthread::atomic<bool> mCompress;
	tthread::atomic<int> mCompressionLevel;
	int mMode;
	unsigned int mNumberOfActiveConnections;
	unsigned int mNumberOfActiveSyncConnections;
	unsigned int mNumberOfActiveDataTransferConnections;
};

}

#endif
