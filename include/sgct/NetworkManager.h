/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__NETWORK_MANAGER__H__
#define __SGCT__NETWORK_MANAGER__H__

#include <sgct/SGCTNetwork.h>
#include <atomic>
#include <string>
#include <vector>

namespace sgct_core {

class Statistics;

/*!
    The network manager manages all network connections for SGCT.
*/
class NetworkManager {
public:
    /*!
        Different sync stages. Server sync data to clients and clients syncs acknowlagement
    */
    enum SyncMode { SendDataToClients = 0, AcknowledgeData };
    enum NetworkMode { Remote = 0, LocalServer, LocalClient };

    static std::condition_variable gCond;

    NetworkManager(NetworkMode nm);
    ~NetworkManager();
    bool init();
    void sync(SyncMode sm, Statistics* statsPtr);
    bool isSyncComplete();
    void close();

    /*!
        \returns the static pointer to the NetworkManager instance
    */
    static NetworkManager* instance();

    bool matchAddress(const std::string& address);
    void retrieveNodeId();
    bool isComputerServer();
    bool isRunning();
    bool areAllNodesConnected();
    SGCTNetwork* getExternalControlPtr();
    void transferData(const void* data, int length, int packageId);
    void transferData(const void* data, int length, int packageId, size_t nodeIndex);
    void transferData(const void* data, int length, int packageId, SGCTNetwork* connection);
    void setDataTransferCompression(bool state, int level = 1);

    unsigned int getActiveConnectionsCount();
    unsigned int getActiveSyncConnectionsCount();
    unsigned int getActiveDataTransferConnectionsCount();
    unsigned int getConnectionsCount();
    unsigned int getSyncConnectionsCount();
    unsigned int getDataTransferConnectionsCount();
    SGCTNetwork* getConnectionByIndex(unsigned int index) const;
    SGCTNetwork* getSyncConnectionByIndex(unsigned int index) const;
    std::vector<std::string> getLocalAddresses();

private:
    bool addConnection(const std::string& port, const std::string& address,
        SGCTNetwork::ConnectionTypes connectionType = SGCTNetwork::SyncConnection);
    void initAPI();
    void getHostInfo();
    void updateConnectionStatus(SGCTNetwork* connection);
    void setAllNodesConnected();
    bool prepareTransferData(const void* data, char** bufferPtr, int& length,
        int packageId);

    static NetworkManager* mInstance;
    std::vector<SGCTNetwork*> mNetworkConnections;
    std::vector<SGCTNetwork*> mSyncConnections;
    std::vector<SGCTNetwork*> mDataTransferConnections;
    SGCTNetwork* mExternalControlConnection = nullptr;

    std::string mHostName; //stores this computers hostname
    std::vector<std::string> mDNSNames;
    std::vector<std::string> mLocalAddresses; //stors this computers ip addresses

    bool mIsServer = true;
    bool mIsRunning = true;
    bool mAllNodesConnected = false;
    std::atomic<bool> mCompress = false;
    std::atomic<int> mCompressionLevel;
    int mMode;
    unsigned int mNumberOfActiveConnections = 0;
    unsigned int mNumberOfActiveSyncConnections = 0;
    unsigned int mNumberOfActiveDataTransferConnections = 0;
};

} // namespace _sgct_core

#endif // __SGCT__NETWORK_MANAGER__H__
