/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/networkmanager.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define VC_EXTRALEAN
#include <windows.h>
#endif

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/fmt.h>
#include <sgct/log.h>
#include <sgct/mutexes.h>
#include <sgct/node.h>
#include <sgct/profiling.h>
#include <sgct/shareddata.h>
#include <algorithm>
#include <cstring>
#include <numeric>

#ifdef WIN32
    #include <ws2tcpip.h>
#else // Use BSD sockets
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <unistd.h>
#endif

#define Error(code, msg) Error(Error::Component::Network, code, msg)

namespace sgct {

std::condition_variable NetworkManager::cond;

NetworkManager* NetworkManager::_instance = nullptr;

NetworkManager& NetworkManager::instance() {
    return *_instance;
}

void NetworkManager::create(NetworkMode nm,
                            std::function<void(void*, int, int, int)> dataTransferDecode,
                            std::function<void(bool, int)> dataTransferStatus,
                            std::function<void(int, int)> dataTransferAcknowledge)
{
    ZoneScoped;

    if (_instance) {
        throw std::logic_error("NetworkManager has already been created");
    }
    _instance = new NetworkManager(
        nm,
        std::move(dataTransferDecode),
        std::move(dataTransferStatus),
        std::move(dataTransferAcknowledge)
    );
}

void NetworkManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

NetworkManager::NetworkManager(NetworkMode nm,
                             std::function<void(void*, int, int, int)> dataTransferDecode,
                                        std::function<void(bool, int)> dataTransferStatus,
                                    std::function<void(int, int)> dataTransferAcknowledge)
    : _dataTransferDecodeFn(std::move(dataTransferDecode))
    , _dataTransferStatusFn(std::move(dataTransferStatus))
    , _dataTransferAcknowledgeFn(std::move(dataTransferAcknowledge))
    , _mode(nm)
{
    ZoneScoped;

    Log::Debug("Initiating network API");
#ifdef WIN32
    WORD version = MAKEWORD(2, 2);

    {
        ZoneScopedN("WSAStartup");
        WSADATA wsaData;
        const int err = WSAStartup(version, &wsaData);
        if (err != 0 || LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
            // incorrect WinSock version
            WSACleanup();
            throw Error(5020, "Winsock 2.2 startup failed");
        }
    }
#endif

    Log::Debug("Getting host info");

    //
    // Get host info
    //
    // get name & local IPs. retrieves the standard host name for the local computer
    std::array<char, 256> Buffer;
    {
        ZoneScopedN("gethostname");
#ifdef WIN32
        const int res = gethostname(Buffer.data(), static_cast<int>(Buffer.size()));
#else // WIN32
        const size_t res = gethostname(Buffer.data(), Buffer.size());
#endif // WIN32
        if (res != 0) {
#ifdef WIN32
            WSACleanup();
#endif
            throw Error(5027, "Failed to get local host name");
        }
    }

    std::string hostName = Buffer.data();
    // add hostname and adress in lower case
    std::transform(
        hostName.cbegin(),
        hostName.cend(),
        hostName.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    _localAddresses.push_back(hostName);

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    // hints.ai_family = AF_UNSPEC; // either IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    addrinfo* info;
    {
        ZoneScopedN("getaddrinfo");
        //TODO: micah - why does getaddrinfo fail for 'macbookpro.local'
        #ifdef __APPLE__
            int result = getaddrinfo("localhost", "http", &hints, &info);
        #else
            int result = getaddrinfo(Buffer.data(), "http", &hints, &info);
        #endif // __APPLE__
    if (result != 0) {
            std::string err = std::to_string(Network::lastError());
            throw Error(5028, fmt::format("Failed to get address info: {}", err));
        }
    }
    std::vector<std::string> dnsNames;
    char addr_str[INET_ADDRSTRLEN];
    for (addrinfo* p = info; p != nullptr; p = p->ai_next) {
        ZoneScopedN("inet_ntop");
        sockaddr_in* sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
        inet_ntop(AF_INET, &sockaddr_ipv4->sin_addr, addr_str, INET_ADDRSTRLEN);
        if (p->ai_canonname) {
            dnsNames.emplace_back(p->ai_canonname);
        }
        _localAddresses.emplace_back(addr_str);
    }

    freeaddrinfo(info);

    for (std::string& dns : dnsNames) {
        std::transform(
            dns.cbegin(),
            dns.cend(),
            dns.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );
        _localAddresses.push_back(std::move(dns));
    }

    // add the loop-back
    _localAddresses.emplace_back("127.0.0.1");
    _localAddresses.emplace_back("localhost");
}

NetworkManager::~NetworkManager() {
    ZoneScoped;

    _isRunning = false;
    cond.notify_all();

    // signal to terminate
    for (std::unique_ptr<Network>& connection : _networkConnections) {
        connection->initShutdown();
    }

    // wait for all nodes callbacks to run
    {
        // @TODO (abock, 2019-12-20) We can probably remove this waiting altogether. The
        // node callbacks are run synchronously with this function, and either way, what
        // if the callbacks take longer and we crash anyway?
        ZoneScopedN("Sleeping");
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    _networkConnections.clear();
    _syncConnections.clear();
    _dataTransferConnections.clear();

#ifdef WIN32
    WSACleanup();
#endif
    Log::Info("Network API closed");
}

void NetworkManager::initialize() {
    ZoneScoped;

    ClusterManager& cm = ClusterManager::instance();

    _isServer = [&](NetworkMode nm) {
        switch (nm) {
            case NetworkMode::Remote: return matchesAddress(cm.masterAddress());
            case NetworkMode::LocalServer: return true;
            case NetworkMode::LocalClient: return false;
            default: throw std::logic_error("Unhandled case label");
        }
    }(_mode);

    if (_isServer) {
        Log::Info("This computer is the network server");
    }
    else {
        Log::Info("This computer is the network client");
    }

    if (cm.thisNode().address().empty()) {
        throw Error(5021, "No address information for this node available");
    }

    std::string remoteAddress;
    if (_mode == NetworkMode::Remote) {
        if (cm.masterAddress().empty()) {
            throw Error(5022, "No address information for master available");
        }
        remoteAddress = cm.masterAddress();
    }
    else {
        // local (not remote)
        remoteAddress = "127.0.0.1";
    }

    // if faking an address (running local) then add it to the search list
    if (_mode != NetworkMode::Remote) {
        _localAddresses.push_back(cm.thisNode().address());
    }

    // Add Cluster Functionality
    if (ClusterManager::instance().numberOfNodes() > 1) {
        ZoneScopedN("Create cluster connections");
        // sanity check if port is used somewhere else
        for (size_t i = 0; i < _networkConnections.size(); i++) {
            const int port = _networkConnections[i]->port();
            if (port == cm.thisNode().syncPort() ||
                port == cm.thisNode().dataTransferPort())
            {
                throw Error(
                    5023,
                    fmt::format(
                        "Port {} is already used by connection {}",
                        cm.thisNode().syncPort(), i
                    )
                );
            }
        }

        // if client
        if (!_isServer) {
            addConnection(cm.thisNode().syncPort(), remoteAddress);
            _networkConnections.back()->setDecodeFunction(
                // @TODO (abock, 2019-12-06) This can be replaced with std::bind_front
                // when switching to C++20
                [](const char* data, int length) {
                    SharedData::instance().decode(data, length);
                }
            );

            // add data transfer connection
            if (cm.thisNode().dataTransferPort() > 0 && !remoteAddress.empty()) {
                addConnection(
                    cm.thisNode().dataTransferPort(),
                    remoteAddress,
                    Network::ConnectionType::DataTransfer
                );
                if (_dataTransferDecodeFn) {
                    _networkConnections.back()->setPackageDecodeFunction(
                        _dataTransferDecodeFn
                    );
                }

                // acknowledge callback
                if (_dataTransferAcknowledgeFn) {
                    _networkConnections.back()->setAcknowledgeFunction(
                        _dataTransferAcknowledgeFn
                    );
                }
            }
        }

        // add all connections from config file
        for (int i = 0; i < cm.numberOfNodes(); i++) {
            const Node& n = cm.node(i);

            // don't add itself if server
            if (_isServer && !matchesAddress(n.address())) {
                addConnection(n.syncPort(), remoteAddress);

                _networkConnections.back()->setDecodeFunction(
                    [](const char* data, int length) {
                        std::vector<char> d(data, data + length);
                        d.push_back('\0');
                        Log::Info(fmt::format("[client]: {} [end]", d.data()));
                    }
                );

                // add data transfer connection
                if (n.dataTransferPort() != 0 && !remoteAddress.empty()) {
                    addConnection(
                        n.dataTransferPort(),
                        remoteAddress,
                        Network::ConnectionType::DataTransfer
                    );
                    if (_dataTransferDecodeFn) {
                        _networkConnections.back()->setPackageDecodeFunction(
                            _dataTransferDecodeFn
                        );
                    }

                    // acknowledge callback
                    if (_dataTransferAcknowledgeFn) {
                        _networkConnections.back()->setAcknowledgeFunction(
                            _dataTransferAcknowledgeFn
                        );
                    }
                }
            }
        }
    }

    Log::Debug(
        fmt::format("Cluster sync: {}", cm.firmFrameLockSyncStatus() ? "firm" : "loose")
    );
}

void NetworkManager::clearCallbacks() {
    _dataTransferDecodeFn = nullptr;
    _dataTransferStatusFn = nullptr;
    _dataTransferAcknowledgeFn = nullptr;
}

std::optional<std::pair<double, double>> NetworkManager::sync(SyncMode sm) {
    if (_syncConnections.empty()) {
        return std::nullopt;
    }
    if (sm == SyncMode::SendDataToClients) {
        double maxTime = -std::numeric_limits<double>::max();
        double minTime = std::numeric_limits<double>::max();

        bool hasFoundConnection = false;
        for (Network* connection : _syncConnections) {
            if (!connection->isServer() || !connection->isConnected()) {
                continue;
            }

            hasFoundConnection = true;

            const double currentTime = connection->loopTime();
            maxTime = std::max(currentTime, maxTime);
            minTime = std::min(currentTime, minTime);

            const int currentSize =
                SharedData::instance().dataSize() - static_cast<int>(Network::HeaderSize);

            // iterate counter
            const int currentFrame = connection->iterateFrameCounter();

            unsigned char* dataBlock = SharedData::instance().dataBlock();
            std::memcpy(dataBlock + 1, &currentFrame, sizeof(currentFrame));
            std::memcpy(dataBlock + 5, &currentSize, sizeof(currentSize));

            connection->sendData(
                SharedData::instance().dataBlock(),
                SharedData::instance().dataSize()
            );
        }

        if (hasFoundConnection) {
            return std::make_pair(minTime, maxTime);
        }
    }
    else if (sm == SyncMode::Acknowledge) {
        for (Network* connection : _syncConnections) {
            if (!connection->isServer() && connection->isConnected()) {
                // The servers's render function is locked until a message starting with
                // the ack-byte is received.
                connection->pushClientMessage();
            }
        }
    }
    return std::nullopt;
}

bool NetworkManager::isSyncComplete() const {
    const unsigned int counter = static_cast<unsigned int>(std::count_if(
        _syncConnections.cbegin(),
        _syncConnections.cend(),
        [](Network* n) { return n->isUpdated(); }
    ));
    return (counter == _nActiveSyncConnections);
}

void NetworkManager::transferData(const void* data, int length, int packageId) {
    std::vector<char> buffer;
    prepareTransferData(data, buffer, length, packageId);
    for (Network* connection : _dataTransferConnections) {
        if (connection->isConnected()) {
            connection->sendData(buffer.data(), length);
        }
    }
}

void NetworkManager::transferData(const void* data, int length, int packageId,
                                  Network& connection)
{
    if (connection.isConnected()) {
        std::vector<char> buffer;
        prepareTransferData(data, buffer, length, packageId);
        connection.sendData(buffer.data(), length);
    }
}

void NetworkManager::prepareTransferData(const void* data, std::vector<char>& buffer,
                                         int& length, int packageId)
{
    int messageLength = length;

    length += static_cast<int>(Network::HeaderSize);
    buffer.resize(length);

    buffer[0] = Network::DataId;
    std::memcpy(buffer.data() + 1, &packageId, sizeof(packageId));

    // set uncompressed size to DefaultId since compression is not used
    std::memset(buffer.data() + 9, Network::DefaultId, sizeof(int));

    // add data to buffer
    std::memcpy(
        buffer.data() + Network::HeaderSize,
        data,
        length - Network::HeaderSize
    );

    std::memcpy(buffer.data() + 5, &messageLength, sizeof(messageLength));
}

unsigned int NetworkManager::activeConnectionsCount() const {
    std::unique_lock lock(mutex::DataSync);
    return _nActiveConnections;
}

int NetworkManager::connectionsCount() const {
    std::unique_lock lock(mutex::DataSync);
    return static_cast<int>(_networkConnections.size());
}

int NetworkManager::syncConnectionsCount() const {
    std::unique_lock lock(mutex::DataSync);
    return static_cast<int>(_syncConnections.size());
}

const Network& NetworkManager::connection(int index) const {
    return *_networkConnections[index];
}

const Network& NetworkManager::syncConnection(int index) const {
    return *_syncConnections[index];
}

void NetworkManager::updateConnectionStatus(Network* connection) {
    Log::Debug(fmt::format("Updating status for connection {}", connection->id()));

    int nConnections = 0;
    int nConnectedSync = 0;
    int nConnectedDataTransfer = 0;

    mutex::DataSync.lock();
    int totalNConnections = static_cast<int>(_networkConnections.size());
    int totalNSyncConnections = static_cast<int>(_syncConnections.size());
    int totalNTransferConnections = static_cast<int>(_dataTransferConnections.size());
    mutex::DataSync.unlock();

    // count connections
    for (const std::unique_ptr<Network>& conn : _networkConnections) {
        if (conn->isConnected()) {
            nConnections++;
            if (conn->type() == Network::ConnectionType::SyncConnection) {
                nConnectedSync++;
            }
            else if (conn->type() == Network::ConnectionType::DataTransfer) {
                nConnectedDataTransfer++;
            }
        }
    }

    Log::Info(fmt::format(
        "Number of active connections {} of {}", nConnections, totalNConnections
    ));
    Log::Debug(fmt::format(
        "Number of connected sync nodes {} of {}", nConnectedSync, totalNSyncConnections
    ));
    Log::Debug(fmt::format(
        "Number of connected data transfer nodes {} of {}",
        nConnectedDataTransfer, totalNTransferConnections
    ));

    mutex::DataSync.lock();
    _nActiveConnections = nConnections;
    _nActiveSyncConnections = nConnectedSync;
    _nActiveDataTransferConnections = nConnectedDataTransfer;

    // if client disconnects then it cannot run anymore
    if (_nActiveSyncConnections == 0 && !_isServer) {
        _isRunning = false;
    }
    mutex::DataSync.unlock();

    if (_isServer) {
        mutex::DataSync.lock();
        // local copy (thread safe)
        bool allNodesConnected = (nConnectedSync == totalNSyncConnections) &&
                                (nConnectedDataTransfer == totalNTransferConnections);
        _allNodesConnected = allNodesConnected;
        mutex::DataSync.unlock();

        // send cluster connected message to clients
        if (allNodesConnected) {
            for (Network* syncConnection : _syncConnections) {
                if (!syncConnection->isConnected()) {
                    continue;
                }
                std::array<char, Network::HeaderSize> data;
                std::fill(data.begin(), data.end(), Network::DefaultId);
                data[0] = Network::ConnectedId;
                syncConnection->sendData(&data, Network::HeaderSize);
            }
            for (Network* dataConnection : _dataTransferConnections) {
                if (dataConnection->isConnected()) {
                    std::array<char, Network::HeaderSize> data;
                    std::fill(data.begin(), data.end(), Network::DefaultId);
                    data[0] = Network::ConnectedId;
                    dataConnection->sendData(&data, Network::HeaderSize);
                }
            }
        }

        // wake up the connection handler thread on server
        connection->startConnectionConditionVar().notify_all();
    }

    if (connection->type() == Network::ConnectionType::DataTransfer) {
        if (_dataTransferStatusFn) {
            _dataTransferStatusFn(connection->isConnected(), connection->id());
        }
    }

    // signal done to caller
    cond.notify_all();
}

void NetworkManager::setAllNodesConnected() {
    std::unique_lock lock(mutex::DataSync);

    if (!_isServer) {
        unsigned int nConn = static_cast<unsigned int>(_dataTransferConnections.size());
        _allNodesConnected = (_nActiveSyncConnections == 1) &&
                             (_nActiveDataTransferConnections == nConn);
    }
}

void NetworkManager::addConnection(int port, std::string address,
                                   Network::ConnectionType connectionType)
{
    ZoneScoped;

    if (port == 0) {
        throw Error(5025, fmt::format("No port provided for connection to {}", address));
    }

    if (address.empty()) {
        throw Error(5026, fmt::format("Empty address for connection to {}", port));
    }

    auto net = std::make_unique<Network>(
        port,
        std::move(address),
        _isServer,
        connectionType
    );
    Log::Debug(fmt::format(
        "Initiating connection {} at port {}", _networkConnections.size(), port
    ));
    net->setUpdateFunction([this](Network* c) { updateConnectionStatus(c); });
    net->setConnectedFunction([this]() { setAllNodesConnected(); });

    // must be initialized after binding
    net->initialize();
    _networkConnections.push_back(std::move(net));

    // Update the previously existing shortcuts (maybe remove them altogether?)
    _syncConnections.clear();
    _dataTransferConnections.clear();

    for (const std::unique_ptr<Network>& connection : _networkConnections) {
        switch (connection->type()) {
            case Network::ConnectionType::SyncConnection:
                _syncConnections.push_back(connection.get());
                break;
            case Network::ConnectionType::DataTransfer:
                _dataTransferConnections.push_back(connection.get());
                break;
        }
    }
}

bool NetworkManager::matchesAddress(std::string_view address) const {
    ZoneScoped;

    const auto it = std::find(_localAddresses.cbegin(), _localAddresses.cend(), address);
    return it != _localAddresses.cend();
}

bool NetworkManager::isComputerServer() const {
    return _isServer;
}

bool NetworkManager::isRunning() const {
    std::unique_lock lock(mutex::DataSync);
    return _isRunning;
}

bool NetworkManager::areAllNodesConnected() const {
    std::unique_lock lock(mutex::DataSync);
    return _allNodesConnected;
}

} // namespace sgct
