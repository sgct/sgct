/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/networkmanager.h>

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#include <windows.h>
#endif

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/messagehandler.h>
#include <sgct/mutexes.h>
#include <sgct/shareddata.h>
#include <zlib.h>
#include <algorithm>
#include <cstring>
#include <numeric>

#ifdef WIN32
    #include <ws2tcpip.h>
#else // Use BSD sockets
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

// missing function on mingw
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

#define Error(code, msg) Error(Error::Component::Network, code, msg)

namespace sgct::core {

std::condition_variable NetworkManager::cond;

NetworkManager* NetworkManager::_instance = nullptr;

NetworkManager& NetworkManager::instance() {
    return *_instance;
}

void NetworkManager::create(NetworkMode nm) {
    if (_instance) {
        throw std::logic_error("NetworkManager has already been created");
    }
    _instance = new NetworkManager(nm);
}

void NetworkManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

NetworkManager::NetworkManager(NetworkMode nm) 
    : _compressionLevel(Z_BEST_SPEED)
    , _mode(nm)
{
    MessageHandler::printDebug("Initiating network API");
#ifdef WIN32
    WORD version = MAKEWORD(2, 2);

    WSADATA wsaData;
    int error = WSAStartup(version, &wsaData);

    if (error != 0 || LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) {
        // incorrect WinSock version
        WSACleanup();
        throw Error(5020, "Winsock 2.2 startup failed");
    }
#endif

    MessageHandler::printDebug("Getting host info");
    getHostInfo();

    if (_mode == NetworkMode::Remote) {
        _isServer = matchesAddress(ClusterManager::instance().getMasterAddress());
    }
    else if (_mode == NetworkMode::LocalServer) {
        _isServer = true;
    }
    else {
        _isServer = false;
    }

    if (_isServer) {
        MessageHandler::printInfo("This computer is the network server");
    }
    else {
        MessageHandler::printInfo("This computer is the network client");
    }
}

NetworkManager::~NetworkManager() {
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

#ifdef WIN32
    WSACleanup();
#endif
    MessageHandler::printInfo("Network API closed");
}

void NetworkManager::init() {
    ClusterManager& cm = ClusterManager::instance();
    if (cm.getThisNode().getAddress().empty()) {
        throw Error(5021, "No address information for this node available");
    }

    std::string remoteAddress;
    if (_mode == NetworkMode::Remote) {
        if (cm.getMasterAddress().empty()) {
            throw Error(5022, "No address information for master available");
        }
        remoteAddress = cm.getMasterAddress();
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
    if (ClusterManager::instance().getNumberOfNodes() > 1) {
        // sanity check if port is used somewhere else
        for (size_t i = 0; i < _networkConnections.size(); i++) {
            const int port = _networkConnections[i]->getPort();
            if (port == cm.getThisNode().getSyncPort() ||
                port == cm.getThisNode().getDataTransferPort() ||
                port == cm.getExternalControlPort())
            {
                const std::string p = std::to_string(cm.getThisNode().getSyncPort());
                throw Error(5023,
                    "Port " + p + " is already used by connection " + std::to_string(i)
                );
            }
        }

        // if client
        if (!_isServer) {
            addConnection(cm.getThisNode().getSyncPort(), remoteAddress);
            _networkConnections.back()->setDecodeFunction(
                [](const char* data, int length, int index) {
                    SharedData::instance().decode(data, length, index);
                }
            );

            // add data transfer connection
            if (cm.getThisNode().getDataTransferPort() > 0 && !remoteAddress.empty()) {
                addConnection(
                    cm.getThisNode().getDataTransferPort(),
                    remoteAddress,
                    Network::ConnectionType::DataTransfer
                );
                _networkConnections.back()->setPackageDecodeFunction(
                    [](void* data, int length, int packageId, int clientId) {
                        Engine::instance().invokeDecodeCallbackForDataTransfer(
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
                        Engine::instance().invokeAcknowledgeCallbackForDataTransfer(
                            packageId,
                            clientId
                        );
                    }
                );
            }
        }

        // add all connections from config file
        for (int i = 0; i < cm.getNumberOfNodes(); i++) {
            const Node& n = cm.getNode(i);

            // don't add itself if server
            if (_isServer && !matchesAddress(n.getAddress())) {
                addConnection(n.getSyncPort(), remoteAddress);

                _networkConnections.back()->setDecodeFunction(
                    [](const char* data, int length, int index) {
                        MessageHandler::instance().decode(
                           std::vector<char>(data, data + length),
                           index
                        );
                    }
                );

                // add data transfer connection
                addConnection(
                    n.getDataTransferPort(),
                    remoteAddress,
                    Network::ConnectionType::DataTransfer
                );
                _networkConnections.back()->setPackageDecodeFunction(
                    [](void* data, int length, int packageId, int clientId) {
                        Engine::instance().invokeDecodeCallbackForDataTransfer(
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
                        Engine::instance().invokeAcknowledgeCallbackForDataTransfer(
                            packageId,
                            clientId
                        );
                    }
                );
            }
        }
    }

    // add connection for external communication
    if (_isServer) {
        addConnection(
            cm.getExternalControlPort(),
            "127.0.0.1",
            Network::ConnectionType::ExternalConnection
        );
        _networkConnections.back()->setDecodeFunction(
            [](const char* data, int len, int id) {
                Engine::instance().invokeDecodeCallbackForExternalControl(data, len, id);
            }
        );
    }

    MessageHandler::printDebug(
        "Cluster sync: %s", cm.getFirmFrameLockSyncStatus() ? "firm/strict" : "loose"
    );
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

            const double currentTime = connection->getLoopTime();
            maxTime = std::max(currentTime, maxTime);
            minTime = std::min(currentTime, minTime);

            const int currentSize =
                static_cast<int>(SharedData::instance().getDataSize()) -
                Network::HeaderSize;

            // iterate counter
            const int currentFrame = connection->iterateFrameCounter();

            unsigned char* dataBlock = SharedData::instance().getDataBlock();
            std::memcpy(dataBlock + 1, &currentFrame, sizeof(int));
            std::memcpy(dataBlock + 5, &currentSize, sizeof(int));

            connection->sendData(
                SharedData::instance().getDataBlock(),
                static_cast<int>(SharedData::instance().getDataSize())
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
    const std::ptrdiff_t counter = std::count_if(
        _syncConnections.cbegin(),
        _syncConnections.cend(),
        [](Network* n) { return n->isUpdated(); }
    );
    return (static_cast<unsigned int>(counter) == _nActiveSyncConnections);
}

Network* NetworkManager::getExternalControlConnection() {
    return _externalControlConnection;
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

    if (_compress) {
        length = static_cast<int>(compressBound(static_cast<uLong>(length)));
    }
    length += static_cast<int>(Network::HeaderSize);

    buffer.resize(length);

    buffer[0] = _compress ? Network::CompressedDataId : Network::DataId;
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
            const std::string e = [](int error) {
                switch (error) {
                    case Z_BUF_ERROR: return "Dest. buffer not large enough";
                    case Z_MEM_ERROR: return "Insufficient memory";
                    case Z_STREAM_ERROR: return "Incorrect compression level";
                    default: return "Unknown error";
                }
            }(err);
            throw Error(5024, "Failed to compress data: " + e);
        }

        // send original size
        std::memcpy(buffer.data() + 9, &length, sizeof(int));

        // re-calculate the true send size
        length = static_cast<int>(compressedSize) + static_cast<int>(Network::HeaderSize);
    }
    else {
        // set uncompressed size to DefaultId since compression is not used
        std::memset(buffer.data() + 9, Network::DefaultId, sizeof(int));

        // add data to buffer
        std::memcpy(
            buffer.data() + Network::HeaderSize,
            data,
            length - Network::HeaderSize
        );
    }

    std::memcpy(buffer.data() + 5, &messageLength, sizeof(int));
}

void NetworkManager::setDataTransferCompression(bool state, int level) {
    _compress = state;
    _compressionLevel = level;
}

unsigned int NetworkManager::getActiveConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return _nActiveConnections;
}

int NetworkManager::getConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return static_cast<int>(_networkConnections.size());
}

int NetworkManager::getSyncConnectionsCount() const {
    std::unique_lock lock(core::mutex::DataSync);
    return static_cast<int>(_syncConnections.size());
}

const Network& NetworkManager::getConnectionByIndex(int index) const {
    return *_networkConnections[index];
}

Network* NetworkManager::getSyncConnectionByIndex(int index) const {
    return _syncConnections[index];
}

void NetworkManager::updateConnectionStatus(Network* connection) {
    MessageHandler::printDebug("Updating status for connection %d", connection->getId());

    unsigned int nConnections = 0;
    unsigned int nConnectedSyncNodes = 0;
    unsigned int nConnectedDataTransferNodes = 0;

    core::mutex::DataSync.lock();
    unsigned int totalNConnections =
        static_cast<unsigned int>(_networkConnections.size());
    unsigned int totalNSyncConnections =
        static_cast<unsigned int>(_syncConnections.size());
    unsigned int totalNTransferConnections =
        static_cast<unsigned int>(_dataTransferConnections.size());
    core::mutex::DataSync.unlock();

    // count connections
    for (const std::unique_ptr<Network>& conn : _networkConnections) {
        if (conn->isConnected()) {
            nConnections++;
            if (conn->getType() == Network::ConnectionType::SyncConnection) {
                nConnectedSyncNodes++;
            }
            else if (conn->getType() == Network::ConnectionType::DataTransfer) {
                nConnectedDataTransferNodes++;
            }
        }
    }

    MessageHandler::printInfo(
        "Number of active connections %u of %u", nConnections, totalNConnections
    );
    MessageHandler::printDebug(
        "Number of connected sync nodes %u of %u",
        nConnectedSyncNodes, totalNSyncConnections
    );
    MessageHandler::printDebug(
        "Number of connected data transfer nodes %u of %u",
        nConnectedDataTransferNodes, totalNTransferConnections
    );

    core::mutex::DataSync.lock();
    _nActiveConnections = nConnections;
    _nActiveSyncConnections = nConnectedSyncNodes;
    _nActiveDataTransferConnections = nConnectedDataTransferNodes;


    // if client disconnects then it cannot run anymore
    if (_nActiveSyncConnections == 0 && !_isServer) {
        _isRunning = false;
    }
    core::mutex::DataSync.unlock();

    if (_isServer) {
        core::mutex::DataSync.lock();
        // local copy (thread safe)
        bool allNodesConnectedCopy = (nConnectedSyncNodes== totalNSyncConnections) &&
                                (nConnectedDataTransferNodes== totalNTransferConnections);
        _allNodesConnected = allNodesConnectedCopy;
        core::mutex::DataSync.unlock();

        // send cluster connected message to clients
        if (allNodesConnectedCopy) {
            for (Network* syncConnection : _syncConnections) {
                if (!syncConnection->isConnected()) {
                    continue;
                }
                char data[Network::HeaderSize];
                std::fill(
                    std::begin(data),
                    std::end(data),
                    static_cast<char>(Network::DefaultId)
                );
                data[0] = Network::ConnectedId;
                syncConnection->sendData(&data, Network::HeaderSize);
            }
            for (Network* dataConnection : _dataTransferConnections) {
                if (dataConnection->isConnected()) {
                    char data[Network::HeaderSize];
                    std::fill(
                        std::begin(data),
                        std::end(data),
                        static_cast<char>(Network::DefaultId)
                    );
                    data[0] = Network::ConnectedId;
                    dataConnection->sendData(&data, Network::HeaderSize);
                }
            }
        }

        // Check if any external connection
        if (connection->getType() == Network::ConnectionType::ExternalConnection) {
            const bool status = connection->isConnected();
            std::string msg = "Connected to SGCT!\r\n";
            connection->sendData(msg.c_str(), static_cast<int>(msg.size()));
            Engine::instance().invokeUpdateCallbackForExternalControl(status);
        }

        // wake up the connection handler thread on server
        connection->getStartConnectionConditionVar().notify_all();
    }

    if (connection->getType() == Network::ConnectionType::DataTransfer) {
        const bool status = connection->isConnected();
        const int id = connection->getId();
        Engine::instance().invokeUpdateCallbackForDataTransfer(status, id);
    }

    // signal done to caller
    cond.notify_all();
}

void NetworkManager::setAllNodesConnected() {
    std::unique_lock lock(core::mutex::DataSync);

    if (!_isServer) {
        unsigned int nConn = static_cast<unsigned int>(_dataTransferConnections.size());
        _allNodesConnected = (_nActiveSyncConnections == 1) &&
                             (_nActiveDataTransferConnections == nConn);
    }
}

void NetworkManager::addConnection(int port, const std::string& address,
                                   Network::ConnectionType connectionType)
{
    if (port == 0) {
        throw Error(5025, "No port provided for connection to " + address);
    }

    if (address.empty()) {
        throw Error(5026, "Empty address for connection to " + std::to_string(port));
    }

    auto net = std::make_unique<Network>(port, address, _isServer, connectionType);
    MessageHandler::printDebug(
        "Initiating network connection %d at port %d", _networkConnections.size(), port
    );
    net->setUpdateFunction([this](Network* c) { updateConnectionStatus(c); });
    net->setConnectedFunction([this]() { setAllNodesConnected(); });

    // must be initialized after binding
    net->init();
    _networkConnections.push_back(std::move(net));

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
}

void NetworkManager::getHostInfo() {
    // get name & local IPs. retrieves the standard host name for the local computer
    char tmpStr[128];
    const int res = gethostname(tmpStr, sizeof(tmpStr));
    if (res == SOCKET_ERROR) {
#ifdef WIN32
        WSACleanup();
#endif
        throw Error(5027, "Failed to get local host name");
    }

    std::string hostName = tmpStr;
    // add hostname and adress in lower case
    std::transform(
        hostName.cbegin(),
        hostName.cend(),
        hostName.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );
    _localAddresses.push_back(hostName);

    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    // hints.ai_family = AF_UNSPEC; // either IPV4 or IPV6
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    addrinfo* info;
    int result = getaddrinfo(tmpStr, "http", &hints, &info);
    if (result != 0) {
        throw Error(5028,
            "Failed to get address info: " + std::to_string(Network::getLastError())
        );
    }
    std::vector<std::string> dnsNames;
    char addr_str[INET_ADDRSTRLEN];
    for (addrinfo* p = info; p != nullptr; p = p->ai_next) {
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
        _localAddresses.push_back(dns);
    }

    // add the loop-back
    _localAddresses.emplace_back("127.0.0.1");
    _localAddresses.emplace_back("localhost");
}

bool NetworkManager::matchesAddress(const std::string& address) const {
    const auto it = std::find(_localAddresses.cbegin(), _localAddresses.cend(), address);
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

} // namespace sgct::core
