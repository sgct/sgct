/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__NETWORK_MANAGER__H__
#define __SGCT__NETWORK_MANAGER__H__

#include <sgct/network.h>
#include <atomic>
#include <string>
#include <vector>

namespace sgct::core {

class Statistics;

/**
 * The network manager manages all network connections for SGCT.
 */
class NetworkManager {
public:
    enum class SyncMode { SendDataToClients = 0, AcknowledgeData };
    enum class NetworkMode { Remote = 0, LocalServer, LocalClient };

    static std::condition_variable cond;

    NetworkManager(NetworkMode nm);
    ~NetworkManager();
    bool init();

    ///  \param if this application is server/master in cluster then set to true
    void sync(SyncMode sm, Statistics& statsPtr);

    /**
     * Compare if the last frame and current frames are different -> data update
     * And if send frame == recieved frame
     */
    bool isSyncComplete() const;
    void close();

    /// \returns the static pointer to the NetworkManager instance
    static NetworkManager* instance();

    bool matchAddress(const std::string& address) const;

    /// Retrieve the node id if this node is part of the cluster configuration
    void retrieveNodeId() const;
    bool isComputerServer() const;
    bool isRunning() const;
    bool areAllNodesConnected() const;
    Network* getExternalControlConnection();
    void transferData(const void* data, int length, int packageId);
    void transferData(const void* data, int length, int packageId, size_t nodeIndex);
    void transferData(const void* data, int length, int packageId, Network* connection);

    /**
     * Compression levels 1-9.
     *   -1 = Default compression
     *    0 = No compression
     *    1 = Best speed
     *    9 = Best compression
     */
    void setDataTransferCompression(bool state, int level = 1);

    unsigned int getActiveConnectionsCount() const;
    unsigned int getActiveSyncConnectionsCount() const;
    unsigned int getActiveDataTransferConnectionsCount() const;
    int getConnectionsCount() const;
    int getSyncConnectionsCount() const;
    int getDataTransferConnectionsCount() const;
    const Network& getConnectionByIndex(unsigned int index) const;
    Network* getSyncConnectionByIndex(unsigned int index) const;
    const std::vector<std::string>& getLocalAddresses() const;

private:
    bool addConnection(int port, const std::string& address,
        Network::ConnectionType connectionType = Network::ConnectionType::SyncConnection);
    void initAPI();
    void getHostInfo();
    void updateConnectionStatus(Network* connection);
    void setAllNodesConnected();
    bool prepareTransferData(const void* data, std::vector<char>& buffer, int& length,
        int packageId);

    static NetworkManager* _instance;

    // This could be a std::vector<Network>, but Network is not move-constructible
    // because of the std::condition_variable in it
    std::vector<std::unique_ptr<Network>> _networkConnections;
    std::vector<Network*> _syncConnections;
    std::vector<Network*> _dataTransferConnections;
    Network* _externalControlConnection = nullptr;

    std::string _hostName; // stores this computers hostname
    std::vector<std::string> _dnsNames;
    std::vector<std::string> _localAddresses; // stores this computers ip addresses

    bool _isServer = true;
    bool _isRunning = true;
    bool _allNodesConnected = false;
    std::atomic_bool _compress = false;
    std::atomic_int _compressionLevel;
    NetworkMode _mode;
    unsigned int _nActiveConnections = 0;
    unsigned int _nActiveSyncConnections = 0;
    unsigned int _nActiveDataTransferConnections = 0;
};

} // namespace sgct::core

#endif // __SGCT__NETWORK_MANAGER__H__
