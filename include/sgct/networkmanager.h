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
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sgct::core {

/// The network manager manages all network connections for SGCT.
class NetworkManager {
public:
    enum class SyncMode { SendDataToClients = 0, Acknowledge };
    enum class NetworkMode { Remote = 0, LocalServer, LocalClient };

    static NetworkManager& instance();
    static void create(NetworkMode nm);
    static void destroy();

    static std::condition_variable cond;

    ~NetworkManager();
    bool init();

    /**
     * \param if this application is server/master in cluster then set to true
     * \return min-max pair of the looping time to all connections if data was sent to the
     *         clients. If it was the acknowledge data call or no connections are
     *         available, a nullopt is returned
     */
    std::optional<std::pair<double, double>> sync(SyncMode sm);

    /**
     * Compare if the last frame and current frames are different -> data update
     * And if send frame == recieved frame
     */
    bool isSyncComplete() const;

    bool matchesAddress(const std::string& address) const;

    /// Retrieve the node id if this node is part of the cluster configuration
    bool isComputerServer() const;
    bool isRunning() const;
    bool areAllNodesConnected() const;
    Network* getExternalControlConnection();
    void transferData(const void* data, int length, int packageId);
    void transferData(const void* data, int length, int packageId, Network& connection);

    /**
     * Compression levels 1-9.
     *   -1 = Default compression
     *    0 = No compression
     *    1 = Best speed
     *    9 = Best compression
     */
    void setDataTransferCompression(bool state, int level = 1);

    unsigned int getActiveConnectionsCount() const;
    int getConnectionsCount() const;
    int getSyncConnectionsCount() const;
    const Network& getConnectionByIndex(int index) const;
    Network* getSyncConnectionByIndex(int index) const;

private:
    NetworkManager(NetworkMode nm);

    bool addConnection(int port, const std::string& address,
        Network::ConnectionType connectionType = Network::ConnectionType::SyncConnection);
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
