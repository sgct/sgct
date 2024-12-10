/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__NETWORKMANAGER__H__
#define __SGCT__NETWORKMANAGER__H__

#include <sgct/sgctexports.h>
#include <sgct/network.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sgct {

class Network;

/**
 * The network manager manages all network connections for SGCT.
 */
class SGCT_EXPORT NetworkManager {
public:
    enum class SyncMode { SendDataToClients = 0, Acknowledge };
    enum class NetworkMode { Remote = 0, LocalServer, LocalClient };

    static NetworkManager& instance();
    static void create(NetworkMode nm,
        std::function<void(void*, int, int, int)> dataTransferDecode,
        std::function<void(bool, int)> dataTransferStatus,
        std::function<void(int, int)> dataTransferAcknowledge);
    static void destroy();

    static std::condition_variable cond;

    ~NetworkManager();

    void initialize();
    void clearCallbacks();

    /**
     * \param sm If this application is server/master in cluster then set to true
     * \return The min-max pair of the looping time to all connections if data was sent to
     *         the clients. If it was the acknowledge data call or no connections are
     *         available, a `nullopt` is returned
     */
    std::optional<std::pair<double, double>> sync(SyncMode sm);

    /**
     * Compare if the last frame and current frames are different -> data update and if
     * send frame == recieved frame
     */
    bool isSyncComplete() const;

    bool matchesAddress(std::string_view address) const;

    /**
     * Retrieve the node id if this node is part of the cluster configuration.
     */
    bool isComputerServer() const;
    bool isRunning() const;
    bool areAllNodesConnected() const;
    void transferData(const void* data, int length, int packageId);
    void transferData(const void* data, int length, int packageId, Network& connection);

    unsigned int activeConnectionsCount() const;
    int connectionsCount() const;
    int syncConnectionsCount() const;
    const Network& connection(int index) const;
    const Network& syncConnection(int index) const;

private:
    NetworkManager(NetworkMode nm,
        std::function<void(void*, int, int, int)> dataTransferDecode,
        std::function<void(bool, int)> dataTransferStatus,
        std::function<void(int, int)> dataTransferAcknowledge);
    NetworkManager(const NetworkManager&) = delete;
    NetworkManager(NetworkManager&&) = delete;
    NetworkManager& operator=(const NetworkManager&) = delete;
    NetworkManager& operator=(NetworkManager&&) = delete;


    void addConnection(int port, std::string address,
        Network::ConnectionType connectionType = Network::ConnectionType::SyncConnection);
    void updateConnectionStatus(Network* connection);
    void setAllNodesConnected();
    void prepareTransferData(const void* data, std::vector<char>& buffer, int& length,
        int packageId);

    static NetworkManager* _instance;

    std::function<void(void*, int, int, int)> _dataTransferDecodeFn;
    std::function<void(bool, int)> _dataTransferStatusFn;
    std::function<void(int, int)> _dataTransferAcknowledgeFn;

    // This could be a std::vector<Network>, but Network is not move-constructible
    // because of the std::condition_variable in it
    std::vector<std::unique_ptr<Network>> _networkConnections;
    std::vector<Network*> _syncConnections;
    std::vector<Network*> _dataTransferConnections;

    std::vector<std::string> _localAddresses; // stores this computers ip addresses

    bool _isServer = true;
    bool _isRunning = true;
    bool _allNodesConnected = false;
    const NetworkMode _mode;
    unsigned int _nActiveConnections = 0;
    unsigned int _nActiveSyncConnections = 0;
    unsigned int _nActiveDataTransferConnections = 0;
};

} // namespace sgct

#endif // __SGCT__NETWORKMANAGER__H__
