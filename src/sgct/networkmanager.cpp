/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/networkmanager.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#endif

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/mutexes.h>
#include <sgct/shareddata.h>
#include <sgct/statistics.h>
#include <algorithm>
#include <numeric>

#include <zlib.h>

#ifdef WIN32
    #include <ws2tcpip.h>
#else //Use BSD sockets
    #ifdef _XCODE
        #include <unistd.h>
    #endif
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #define SOCKET_ERROR (-1)
#endif

//missing function on mingw
#if defined(__MINGW32__) || defined(__MINGW64__)
const char* inet_ntop(int af, const void* src, char* dst, int cnt) {
    sockaddr_in srcaddr;

    memset(&srcaddr, 0, sizeof(sockaddr_in));
    memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));

    srcaddr.sin_family = af;
    int res = WSAAddressToString(
        reinterpret_cast<sockaddr*>(&srcaddr),
        sizeof(sockaddr_in),
        0,
        dst,
        reinterpret_cast<LPDWORD>(&cnt)
    );
    if (res != 0) {
        DWORD rv = WSAGetLastError();
        printf("WSAAddressToString() : %d\n",rv);
        return nullptr;
    }
    return dst;
}
#endif // defined(__MINGW32__) || defined(__MINGW64__)

namespace sgct::core {

std::condition_variable NetworkManager::cond;
NetworkManager* NetworkManager::_instance = nullptr;

NetworkManager* NetworkManager::instance() {
    return _instance;
}

NetworkManager::NetworkManager(NetworkMode nm) 
    : _compressionLevel(Z_BEST_SPEED)
    , _mode(nm)
{
    _instance = this;

    MessageHandler::printDebug("NetworkManager: Initiating network API");
    initAPI();

    MessageHandler::printDebug("NetworkManager: Getting host info");
    getHostInfo();

    if (_mode == NetworkMode::Remote) {
        _isServer = matchAddress(ClusterManager::instance()->getMasterAddress());
    }
    else if (_mode == NetworkMode::LocalServer) {
        _isServer = true;
    }
    else {
        _isServer = false;
    }

    if (_isServer) {
        MessageHandler::printInfo("NetworkManager: This computer is the network server");
    }
    else {
        MessageHandler::printInfo("NetworkManager: This computer is the network client");
    }
}

NetworkManager::~NetworkManager() {
    close();
}

bool NetworkManager::init() {
    ClusterManager& cm = *ClusterManager::instance();
    const std::string& thisAddress = cm.getThisNode().getAddress();
    if (thisAddress.empty()) {
        MessageHandler::printError(
            "NetworkManager: No address information for this node available"
        );
        return false;
    }

    std::string remoteAddress;
    if (_mode == NetworkMode::Remote) {
        remoteAddress = cm.getMasterAddress();

        if (remoteAddress.empty()) {
            MessageHandler::printError(
                "NetworkManager: No address information for master/host availible"
            );
            return false;
        }
    }
    else {
        // local (not remote)
        remoteAddress = "127.0.0.1";
    }

    // if faking an address (running local) then add it to the search list
    if (_mode != NetworkMode::Remote) {
        _localAddresses.push_back(cm.getThisNode().getAddress());
    }

    // Add Cluster Functionality
    if (ClusterManager::instance()->getNumberOfNodes() > 1) {
        // sanity check if port is used somewhere else
        for (size_t i = 0; i < _networkConnections.size(); i++) {
            const int port = _networkConnections[i]->getPort();
            if (port == cm.getThisNode().getSyncPort() ||
                port == cm.getThisNode().getDataTransferPort() ||
                port == cm.getExternalControlPort())
            {
                MessageHandler::printError(
                    "NetworkManager: Port %d is already used by connection %u",
                    cm.getThisNode().getSyncPort(), i
                );
                return false;
            }
        }

        // if client
        if (!_isServer) {
            const bool addSyncPort = addConnection(
                cm.getThisNode().getSyncPort(),
                remoteAddress
            );
            if (addSyncPort) {
                _networkConnections.back()->setDecodeFunction(
                    [](const char* data, int length, int index) {
                        SharedData::instance()->decode(data, length, index);
                    }
                );
            }
            else {
                MessageHandler::printError(
                    "NetworkManager: Failed to add network connection to %s",
                    cm.getMasterAddress().c_str()
                );
                return false;
            }

            // add data transfer connection
            bool addTransferPort = addConnection(
                cm.getThisNode().getDataTransferPort(),
                remoteAddress,
                Network::ConnectionType::DataTransfer
            );
            if (addTransferPort) {
                _networkConnections.back()->setPackageDecodeFunction(
                    [](void* data, int length, int packageId, int clientId) {
                        Engine::instance()->invokeDecodeCallbackForDataTransfer(
                            data,
                            length,
                            packageId,
                            clientId
                        );
                    }
                );

                // acknowledge callback
                _networkConnections.back()->setAcknowledgeFunction(
                    [](int packageId, int clientId) {
                        Engine::instance()->invokeAcknowledgeCallbackForDataTransfer(
                            packageId,
                            clientId
                        );
                    }
                );
            }
        }

        // add all connections from config file
        for (int i = 0; i < cm.getNumberOfNodes(); i++) {
            // don't add itself if server
            if (_isServer && !matchAddress(cm.getNode(i)->getAddress())) {
                const bool addSyncPort = addConnection(
                    cm.getNode(i)->getSyncPort(),
                    remoteAddress
                );
                if (!addSyncPort) {
                    MessageHandler::printError(
                        "NetworkManager: Failed to add network connection to %s",
                        cm.getNode(i)->getAddress().c_str()
                    );
                    return false;
                }
                else {
                    _networkConnections.back()->setDecodeFunction(
                        [](const char* data, int length, int index) {
                            MessageHandler::instance()->decode(
                                std::vector<char>(data, data + length),
                                index
                            );
                        }
                    );
                }

                // add data transfer connection
                const bool addTransferPort = addConnection(
                    cm.getNode(i)->getDataTransferPort(),
                    remoteAddress,
                    Network::ConnectionType::DataTransfer
                );
                if (addTransferPort) {
                    _networkConnections.back()->setPackageDecodeFunction(
                        [](void* data, int length, int packageId, int clientId) {
                            Engine::instance()->invokeDecodeCallbackForDataTransfer(
                                data,
                                length,
                                packageId,
                                clientId
                            );
                        }
                    );

                    // acknowledge callback
                    _networkConnections.back()->setAcknowledgeFunction(
                        [](int packageId, int clientId) {
                            Engine::instance()->invokeAcknowledgeCallbackForDataTransfer(
                                packageId,
                                clientId
                            );
                        }
                    );
                }
            }
        }
    }

    // add connection for external communication
    if (_isServer) {
        const bool addExternalPort = addConnection(
            cm.getExternalControlPort(),
            "127.0.0.1",
            cm.getUseASCIIForExternalControl() ?
                Network::ConnectionType::ExternalASCIIConnection :
                Network::ConnectionType::ExternalRawConnection
        );
        if (addExternalPort) {
            _networkConnections.back()->setDecodeFunction(
                [](const char* data, int length, int client) {
                    Engine::instance()->invokeDecodeCallbackForExternalControl(
                        data,
                        length,
                        client
                    );
                }
            );
        }
    }

    MessageHandler::printDebug(
        "NetworkManager: Cluster sync is set to %s",
        cm.getFirmFrameLockSyncStatus() ? "firm/strict" : "loose"
    );

    return true;
}

void NetworkManager::sync(SyncMode sm, Statistics& stats) {
    if (sm == SyncMode::SendDataToClients) {
        double maxTime = -std::numeric_limits<double>::max();
        double minTime = std::numeric_limits<double>::max();

        for (Network* connection : _syncConnections) {
            if (!connection->isServer() || !connection->isConnected()) {
                continue;
            }

            const double currentTime = connection->getLoopTime();
            maxTime = std::max(currentTime, maxTime);
            minTime = std::min(currentTime, minTime);

            const int currentSize =
                static_cast<int>(SharedData::instance()->getDataSize()) -
                Network::HeaderSize;

            // iterate counter
            const int currentFrame = connection->iterateFrameCounter();

            unsigned char* dataBlock = SharedData::instance()->getDataBlock();
            std::memcpy(dataBlock + 1, &currentFrame, sizeof(int));
            std::memcpy(dataBlock + 5, &currentSize, sizeof(int));

            connection->sendData(
                SharedData::instance()->getDataBlock(),
                static_cast<int>(SharedData::instance()->getDataSize())
            );
        }

        if (isComputerServer()) {
            stats.setLoopTime(static_cast<float>(minTime), static_cast<float>(maxTime));
        }
    }
    else if (sm == SyncMode::AcknowledgeData) {
        for (Network* connection : _syncConnections) {
            // Client
            if (!connection->isServer() && connection->isConnected()) {
                // The servers's render function is locked until a message starting with
                // the ack-byte is received.
                // send message to server
                connection->pushClientMessage();
            }
        }
    }
}

bool NetworkManager::isSyncComplete() const {
    const unsigned int counter = static_cast<unsigned int>(
        std::count_if(
            _syncConnections.cbegin(),
            _syncConnections.cend(),
            [](Network* n) { return n->isUpdated(); }
        )
    );
    return (counter == getActiveSyncConnectionsCount());
}

Network* NetworkManager::getExternalControlConnection() {
    return _externalControlConnection;
}

void NetworkManager::transferData(const void* data, int length, int packageId) {
    std::vector<char> buffer;
    const bool success = prepareTransferData(data, buffer, length, packageId);
    if (success) {
        for (Network* connection : _dataTransferConnections) {
            if (connection->isConnected()) {
                connection->sendData(buffer.data(), length);
            }
        }
    }
}

void NetworkManager::transferData(const void* data, int length, int id, size_t node) {
    if (node >= _dataTransferConnections.size() ||
        !_dataTransferConnections[node]->isConnected())
    {
        return;

    }
    std::vector<char> buffer;
    const bool success = prepareTransferData(data, buffer, length, id);
    if (success) {
        _dataTransferConnections[node]->sendData(buffer.data(), length);
    }
}

void NetworkManager::transferData(const void* data, int length, int packageId,
                                  Network* connection)
{
    if (connection->isConnected()) {
        std::vector<char> buffer;
        const bool success = prepareTransferData(data, buffer, length, packageId);
        if (success) {
            connection->sendData(buffer.data(), length);
        }
    }
}

bool NetworkManager::prepareTransferData(const void* data, std::vector<char>& buffer,
                                         int& length, int packageId)
{
    int messageLength = length;

    if (_compress) {
        length = static_cast<int>(compressBound(static_cast<uLong>(length)));
    }
    length += static_cast<int>(Network::HeaderSize);

    buffer.resize(length);

    buffer[0] = static_cast<char>(
        _compress ? Network::CompressedDataId : Network::DataId
    );
    memcpy(buffer.data() + 1, &packageId, sizeof(int));

    if (_compress) {
        char* compDataPtr = buffer.data() + Network::HeaderSize;
        uLong compressedSize = static_cast<uLongf>(length - Network::HeaderSize);
        int err = compress2(
            reinterpret_cast<Bytef*>(compDataPtr),
            &compressedSize,
            reinterpret_cast<const Bytef*>(data),
            static_cast<uLong>(length),
            _compressionLevel
        );

        if (err != Z_OK) {
            std::string errStr;
            switch (err) {
                case Z_BUF_ERROR:
                    errStr = "Dest. buffer not large enough";
                    break;
                case Z_MEM_ERROR:
                    errStr = "Insufficient memory";
                    break;
                case Z_STREAM_ERROR:
                    errStr = "Incorrect compression level";
                    break;
                default:
                    errStr = "Unknown error";
                    break;
            }

            MessageHandler::printError(
                "NetworkManager: Failed to compress data! Error: %s", errStr.c_str()
            );
            return false;
        }

        // send original size
        std::memcpy(buffer.data() + 9, &length, sizeof(int));

        length = static_cast<int>(compressedSize);
        // re-calculate the true send size
        length = length + static_cast<int>(Network::HeaderSize);
    }
    else {
        // set uncompressed size to DefaultId since compression is not used
        memset(buffer.data() + 9, Network::DefaultId, sizeof(int));

        // add data to buffer
        memcpy(buffer.data() + Network::HeaderSize, data, length - Network::HeaderSize);
    }

    std::memcpy(buffer.data() + 5, &messageLength, sizeof(int));

    return true;
}

void NetworkManager::setDataTransferCompression(bool state, int level) {
    _compress = state;
    _compressionLevel = level;
}

unsigned int NetworkManager::getActiveConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return _nActiveConnections;
}

unsigned int NetworkManager::getActiveSyncConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return _nActiveSyncConnections;
}

unsigned int NetworkManager::getActiveDataTransferConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return _nActiveDataTransferConnections;
}

int NetworkManager::getConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return static_cast<int>(_networkConnections.size());
}

int NetworkManager::getSyncConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return static_cast<int>(_syncConnections.size());
}

int NetworkManager::getDataTransferConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return static_cast<int>(_dataTransferConnections.size());
}

const Network& NetworkManager::getConnectionByIndex(unsigned int index) const {
    return *_networkConnections[index];
}

Network* NetworkManager::getSyncConnectionByIndex(unsigned int index) const {
    return _syncConnections[index];
}

const std::vector<std::string>& NetworkManager::getLocalAddresses() const {
    return _localAddresses;
}

void NetworkManager::updateConnectionStatus(Network* connection) {
    MessageHandler::instance()->printDebug(
        "NetworkManager: Updating status for connection %d", connection->getId()
    );

    unsigned int numberOfConnectionsCounter = 0;
    unsigned int nConnectedSyncNodesCounter = 0;
    unsigned int nConnectedDataTransferNodesCounter = 0;

    core::mutex::DataSync.lock();
    unsigned int totalNumberOfConnections =
        static_cast<unsigned int>(_networkConnections.size());
    unsigned int totalNumberOfSyncConnections =
        static_cast<unsigned int>(_syncConnections.size());
    unsigned int totalNumberOfTransferConnections =
        static_cast<unsigned int>(_dataTransferConnections.size());
    core::mutex::DataSync.unlock();

    // count connections
    for (const std::unique_ptr<Network>& conn : _networkConnections) {
        if (conn->isConnected()) {
            numberOfConnectionsCounter++;
            if (conn->getType() == Network::ConnectionType::SyncConnection) {
                nConnectedSyncNodesCounter++;
            }
            else if (conn->getType() == Network::ConnectionType::DataTransfer) {
                nConnectedDataTransferNodesCounter++;
            }
        }
    }

    MessageHandler::printInfo(
        "NetworkManager: Number of active connections %u of %u",
        numberOfConnectionsCounter, totalNumberOfConnections
    );
    MessageHandler::printDebug(
        "NetworkManager: Number of connected sync nodes %u of %u",
        nConnectedSyncNodesCounter, totalNumberOfSyncConnections
    );
    MessageHandler::printDebug(
        "NetworkManager: Number of connected data transfer nodes %u of %u",
        nConnectedDataTransferNodesCounter, totalNumberOfTransferConnections
    );

    core::mutex::DataSync.lock();
    _nActiveConnections = numberOfConnectionsCounter;
    _nActiveSyncConnections = nConnectedSyncNodesCounter;
    _nActiveDataTransferConnections = nConnectedDataTransferNodesCounter;


    // if client disconnects then it cannot run anymore
    if (_nActiveSyncConnections == 0 && !_isServer) {
        _isRunning = false;
    }
    core::mutex::DataSync.unlock();

    if (_isServer) {
        core::mutex::DataSync.lock();
        // local copy (thread safe)
        bool allNodesConnectedCopy =
            (nConnectedSyncNodesCounter == totalNumberOfSyncConnections) &&
            (nConnectedDataTransferNodesCounter == totalNumberOfTransferConnections);

        _allNodesConnected = allNodesConnectedCopy;
        core::mutex::DataSync.unlock();

        // send cluster connected message to nodes/slaves
        if (allNodesConnectedCopy) {
            for (unsigned int i = 0; i < _syncConnections.size(); i++) {
                if (!_syncConnections[i]->isConnected()) {
                    continue;
                }
                char data[Network::HeaderSize];
                std::fill(
                    std::begin(data),
                    std::end(data),
                    static_cast<char>(Network::DefaultId)
                );
                data[0] = Network::ConnectedId;

                _syncConnections[i]->sendData(&data, Network::HeaderSize);
            }
            for (unsigned int i = 0; i < _dataTransferConnections.size(); i++) {
                if (_dataTransferConnections[i]->isConnected()) {
                    char data[Network::HeaderSize];
                    std::fill(
                        std::begin(data),
                        std::end(data),
                        static_cast<char>(Network::DefaultId)
                    );
                    data[0] = Network::ConnectedId;
                    _dataTransferConnections[i]->sendData(&data, Network::HeaderSize);
                }
            }
        }

        // Check if any external connection
        if (connection->getType() == Network::ConnectionType::ExternalASCIIConnection) {
            const bool externalControlConnectionStatus = connection->isConnected();
            std::string_view msg = "Connected to SGCT!\r\n";
            connection->sendData(msg.data(), static_cast<int>(msg.size()));
            Engine::instance()->invokeUpdateCallbackForExternalControl(
                externalControlConnectionStatus
            );
        }
        else if (connection->getType() == Network::ConnectionType::ExternalRawConnection)
        {
            const bool externalControlConnectionStatus = connection->isConnected();
            Engine::instance()->invokeUpdateCallbackForExternalControl(
                externalControlConnectionStatus
            );
        }

        // wake up the connection handler thread on server
        // if node disconnects to enable reconnection
        connection->_startConnectionCond.notify_all();
    }

    if (connection->getType() == Network::ConnectionType::DataTransfer) {
        const bool dataTransferConnectionStatus = connection->isConnected();
        Engine::instance()->invokeUpdateCallbackForDataTransfer(
            dataTransferConnectionStatus,
            connection->getId()
        );
    }

    // signal done to caller
    cond.notify_all();
}

void NetworkManager::setAllNodesConnected() {
    std::unique_lock lock(core::mutex::DataSync);

    if (!_isServer) {
        unsigned int totalNumberOfTransferConnections = static_cast<unsigned int>(
            _dataTransferConnections.size()
        );
        _allNodesConnected = (_nActiveSyncConnections == 1) &&
            (_nActiveDataTransferConnections == totalNumberOfTransferConnections);
    }
}

void NetworkManager::close() {
    _isRunning = false;

    // release condition variables
    cond.notify_all();

    // signal to terminate
    for (std::unique_ptr<Network>& connection : _networkConnections) {
        connection->initShutdown();
    }

    // wait for all nodes callbacks to run
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    // wait for threads to die
    for (std::unique_ptr<Network>& connection : _networkConnections) {
        connection->closeNetwork(false);
    }

    _networkConnections.clear();
    _syncConnections.clear();
    _dataTransferConnections.clear();

#ifdef _WIN_PLATFORM
    WSACleanup();
#endif
    MessageHandler::printInfo("NetworkManager: Network API closed");
}

bool NetworkManager::addConnection(int port, const std::string& address,
                                   Network::ConnectionType connectionType)
{
    if (port == 0) {
        MessageHandler::printInfo(
            "NetworkManager: No port set for %s",
            Network::getTypeStr(connectionType).c_str()
        );
        return false;
    }

    if (address.empty()) {
        MessageHandler::printError(
            "NetworkManager: Error: No address set for %s",
            Network::getTypeStr(connectionType).c_str()
        );
        return false;
    }

    try {
        std::unique_ptr<Network> netPtr = std::make_unique<Network>();
        MessageHandler::printDebug(
            "NetworkManager: Initiating network connection %d at port %d",
            _networkConnections.size(), port
        );
        netPtr->setUpdateFunction([this](Network* c) { updateConnectionStatus(c); });
        netPtr->setConnectedFunction([this]() { setAllNodesConnected(); });

        // must be initialized after binding
        netPtr->init(port, address, _isServer, connectionType);
        _networkConnections.push_back(std::move(netPtr));
    }
    catch (const std::runtime_error& e) {
        MessageHandler::printError("NetworkManager: Network error: %s", e.what());
        return false;
    }

    // Update the previously existing shortcuts (maybe remove them altogether?)
    _syncConnections.clear();
    _dataTransferConnections.clear();
    _externalControlConnection = nullptr;

    for (std::unique_ptr<Network>& connection : _networkConnections) {
        switch (connection->getType()) {
            case Network::ConnectionType::SyncConnection:
                _syncConnections.push_back(connection.get());
                break;
            case Network::ConnectionType::DataTransfer:
                _dataTransferConnections.push_back(connection.get());
                break;
            default:
                _externalControlConnection = connection.get();
                break;
        }
    }

    return true;
}

void NetworkManager::initAPI() {
#ifdef WIN32
    WORD version = MAKEWORD(2, 2);

    WSADATA wsaData;
    int error = WSAStartup(version, &wsaData);

    if (error != 0 || LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        // incorrect WinSock version
        WSACleanup();
        throw std::runtime_error("Winsock 2.2 startup failed");
    }
#endif
}

void NetworkManager::getHostInfo() {
    // get name & local ips
    // retrieves the standard host name for the local computer
    char tmpStr[128];
    if (gethostname(tmpStr, sizeof(tmpStr)) == SOCKET_ERROR) {
#ifdef _WIN_PLATFORM
        WSACleanup();
#endif
        throw std::runtime_error("Failed to get host name");
    }

    _hostName = tmpStr;
    // add hostname and adress in lower case
    std::transform(
        _hostName.cbegin(),
        _hostName.cend(),
        _hostName.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    _localAddresses.push_back(_hostName);

    addrinfo hints;
    sockaddr_in* sockaddr_ipv4;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    // hints.ai_family = AF_UNSPEC; // either IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    addrinfo* info;
    int result = getaddrinfo(tmpStr, "http", &hints, &info);
    if (result != 0) {
        MessageHandler::printError(
            "NetworkManager: Failed to get address info (%d)", Network::getLastError()
        );
    }
    else {
        char addr_str[INET_ADDRSTRLEN];
        for (addrinfo* p = info; p != nullptr; p = p->ai_next) {
            sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
            inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), addr_str, INET_ADDRSTRLEN);
            if (p->ai_canonname) {
                _dnsNames.push_back(p->ai_canonname);
            }
            _localAddresses.push_back(addr_str);
        }
    }

    freeaddrinfo(info);

    for (std::string& dns : _dnsNames) {
        std::transform(
            dns.cbegin(),
            dns.cend(),
            dns.begin(),
            [](char c) { return static_cast<char>(::tolower(c)); }
        );
        _localAddresses.push_back(dns);
    }

    // add the loop-back
    _localAddresses.push_back("127.0.0.1");
    _localAddresses.push_back("localhost");
}

bool NetworkManager::matchAddress(const std::string& address) const {
    auto it = std::find(_localAddresses.cbegin(), _localAddresses.cend(), address);
    return it != _localAddresses.cend();
}

bool NetworkManager::isComputerServer() const {
    return _isServer;
}

bool NetworkManager::isRunning() const {
    std::unique_lock lock(core::mutex::DataSync);
    return _isRunning;
}

bool NetworkManager::areAllNodesConnected() const {
    std::unique_lock lock(core::mutex::DataSync);
    return _allNodesConnected;
}

void NetworkManager::retrieveNodeId() const {
    for (int i = 0; i < ClusterManager::instance()->getNumberOfNodes(); i++) {
        // check ip
        if (matchAddress(ClusterManager::instance()->getNode(i)->getAddress())) {
            ClusterManager::instance()->setThisNodeId(static_cast<int>(i));
            MessageHandler::printDebug(
                "NetworkManager: Running in cluster mode as node %d",
                ClusterManager::instance()->getThisNodeId()
            );
            break;
        }
    }
}

} // namespace sgct::core
