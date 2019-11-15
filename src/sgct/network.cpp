/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/network.h>

#ifdef WIN32
    #define WIN32_LEAN_AND_MEAN
    #define VC_EXTRALEAN
    #include <windows.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #define SGCT_ERRNO WSAGetLastError()
#else //Use BSD sockets
    #ifdef _XCODE
        #include <unistd.h>
    #endif
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <errno.h>
    #define SOCKET_ERROR (-1)
    #define INVALID_SOCKET static_cast<SGCT_SOCKET>(~0)
    #define NO_ERROR 0L
    #define SGCT_ERRNO errno
#endif

#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/clustermanager.h>
#include <sgct/messagehandler.h>
#include <sgct/networkmanager.h>
#include <sgct/mutexes.h>
#include <sgct/shareddata.h>
#include <zlib.h>
#include <algorithm>

#define Error(code, msg) Error(Error::Component::Network, code, msg)

namespace {
    constexpr const int MaxNumberOfAttempts = 10;
    constexpr const int SocketBufferSize = 4096;

    constexpr const int MaxNetworkSyncFrameNumber = 10000;

    int32_t parseInt32(const char* str) {
        return *(reinterpret_cast<const int32_t*>(str));
    }

    uint32_t parseUInt32(const char* str) {
        return *(reinterpret_cast<const uint32_t*>(str));
    }

    std::string getUncompressionErrorAsStr(int err) {
        switch (err) {
            case Z_BUF_ERROR:
                return "Dest. buffer not large enough";
            case Z_MEM_ERROR:
                return "Insufficient memory";
            case Z_DATA_ERROR:
                return "Corrupted data";
            default:
                return "Unknown error";
        }
    }

    bool parseDisconnectPackage(const char* headerPtr) {
        return (headerPtr[0] == sgct::core::Network::DisconnectId &&
                headerPtr[1] == 24 && headerPtr[2] == '\r' && headerPtr[3] == '\n' &&
                headerPtr[4] == 27 && headerPtr[5] == '\r' &&
                headerPtr[6] == '\n' && headerPtr[7] == '\0');
    }
} // namespace

namespace sgct::core {

Network::Network()
    : _socket(INVALID_SOCKET)
    , _listenSocket(INVALID_SOCKET)
{
    static int id = 0;
    _id = id;
    id++;
}

Network::~Network() {
    closeNetwork(false);
}

void Network::init(int port, std::string address, bool isServer, ConnectionType type) {
    _isServer = isServer;
    _connectionType = type;
    if (_connectionType == ConnectionType::SyncConnection) {
        _bufferSize = static_cast<uint32_t>(SharedData::instance().getBufferSize());
        _uncompressedBufferSize = _bufferSize;
    }

    _port = port;
    _address = std::move(address);

    addrinfo* res = nullptr;
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    int addrRes = getaddrinfo(
        _isServer ? nullptr : _address.c_str(),
        std::to_string(_port).c_str(),
        &hints,
        &res
    );
    if (addrRes != 0) {
        throw Error(5000, "Failed to parse hints for connection");
    }

    if (_isServer) {
        // Create a SOCKET for the server to listen for client connections
        _listenSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (_listenSocket == INVALID_SOCKET) {
            freeaddrinfo(res);
            throw Error(5001, "Failed to listen init socket");
        }

        setOptions(&_listenSocket);

        // Setup the TCP listening socket
        int bindResult = bind(
            _listenSocket,
            res->ai_addr,
            static_cast<int>(res->ai_addrlen)
        );
        if (bindResult == SOCKET_ERROR) {
            freeaddrinfo(res);
#ifdef WIN32
            closesocket(_listenSocket);
#else
            close(_listenSocket);
#endif
            throw Error(5002, "Bind socket failed");
        }

        if (listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            freeaddrinfo(res);
#ifdef WIN32
            closesocket(_listenSocket);
#else
            close(_listenSocket);
#endif
            throw Error(5003, "Listen failed");
        }
    }
    else {
        // Client socket
        // Connect to server.
        while (!_shouldTerminate) {
            MessageHandler::printInfo(
                "Attempting to connect to server (id: %d, ip: %s, type: %s)",
                _id, getAddress().c_str(), getTypeStr(getType()).c_str()
            );

            _socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (_socket == INVALID_SOCKET) {
                freeaddrinfo(res);
                throw Error(5004, "Failed to init client socket");
            }

            setOptions(&_socket);

            int r = connect(_socket, res->ai_addr, static_cast<int>(res->ai_addrlen));
            if (r != SOCKET_ERROR) {
                break;
            }

            MessageHandler::printDebug("Connect error code: %d", SGCT_ERRNO);
            std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for next attempt
        }
    }

    freeaddrinfo(res);
    _mainThread = std::make_unique<std::thread>([this]() { connectionHandler(); });
}

void Network::connectionHandler() {
    if (_isServer) {
        while (!isTerminated()) {
            if (!_isConnected) {
                // first time the thread is NULL so the wait will not run
                if (_commThread) {
                    _commThread->join();
                    _commThread = nullptr;
                }

                // start a new connection enabling the client to reconnect
                _commThread = std::make_unique<std::thread>(
                    [this]() { communicationHandler(); }
                );
            }

            // wait for signal until next iteration in loop
            if (!isTerminated()) {
                std::unique_lock lk(_connectionMutex);
                _startConnectionCond.wait(lk);
            }
        }
    }
    else {
        // if client
        _commThread = std::make_unique<std::thread>([this]() { communicationHandler(); });
    }

    MessageHandler::printInfo("Exiting connection handler for connection %d", _id);
}

int Network::getPort() const {
    return _port;
}

const std::string& Network::getAddress() const {
    return _address;
}

std::string Network::getTypeStr(ConnectionType ct) {
    switch (ct) {
        case ConnectionType::SyncConnection:
        default:
            return "sync";
        case ConnectionType::ExternalConnection:
            return "external ASCII control";
        case ConnectionType::DataTransfer:
            return "data transfer";
    }
}

void Network::setOptions(SGCT_SOCKET* socketPtr) {
    if (socketPtr == nullptr) {
        return;
    }
    int flag = 1;

    if (!(getType() == ConnectionType::DataTransfer && _useNaglesAlgorithmInTransfer)) {
        // intset no delay, disable nagle's algorithm
        int iResult = setsockopt(
            *socketPtr,    // socket affected
            IPPROTO_TCP,   // set option at TCP level
            TCP_NODELAY,   // name of option
            reinterpret_cast<char*>(&flag), // the cast is historical cruft
            sizeof(int)    // length of option value
        );

        if (iResult != NO_ERROR) {
            MessageHandler::printError(
                "Failed to set network no-delay option with error: %d", SGCT_ERRNO
            );
        }
    }
    else {
        MessageHandler::printInfo("Enabling Nagle's Algorithm for connection %d", _id);
    }

    // set timeout
    int timeout = 0; // infinite
    setsockopt(
        *socketPtr,
        SOL_SOCKET,
        SO_SNDTIMEO,
        reinterpret_cast<char*>(&timeout),
        sizeof(timeout)
    );

    int sockoptRes = setsockopt(
        *socketPtr,
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<char*>(&flag),
        sizeof(int)
    );
    if (sockoptRes == SOCKET_ERROR) {
        MessageHandler::printWarning("Failed to set reuse address. %d", SGCT_ERRNO);
    }

    // The default buffer value is 8k (8192 bytes) which is good for external control
    // but might be a bit to big for sync data.
    if (getType() == core::Network::ConnectionType::SyncConnection) {
        int bufferSize = SocketBufferSize;
        int iResult = setsockopt(
            *socketPtr,
            SOL_SOCKET,
            SO_RCVBUF,
            reinterpret_cast<char*>(&bufferSize),
            sizeof(int)
        );
        if (iResult == SOCKET_ERROR) {
            MessageHandler::printError(
                "Failed to set send buffer size to %d. %d", bufferSize, SGCT_ERRNO
            );
        }
        iResult = setsockopt(
            *socketPtr,
            SOL_SOCKET,
            SO_SNDBUF,
            reinterpret_cast<char*>(&bufferSize),
            sizeof(int)
        );
        if (iResult == SOCKET_ERROR) {
            MessageHandler::printError(
                "Failed to set receive buffer size to %d. %d", bufferSize, SGCT_ERRNO
            );
        }
    }
    else {
        // set on all connections types, cluster nodes sends data several times per
        // second so there is no need so send alive packages
        int iResult = setsockopt(
            *socketPtr,
            SOL_SOCKET,
            SO_KEEPALIVE,
            reinterpret_cast<char*>(&flag),
            sizeof(int)
        );
        if (iResult == SOCKET_ERROR) {
            MessageHandler::printWarning("Failed to set keep alive. %d", SGCT_ERRNO);
        }
    }
}

void Network::closeSocket(SGCT_SOCKET lSocket) {
    if (lSocket == INVALID_SOCKET) {
        return;
    }

    // Windows shutdown options
    //   * SD_RECIEVE
    //   * SD_SEND
    //   * SD_BOTH

    // Linux & Mac shutdown options
    //   * SHUT_RD (Disables further receive operations)
    //   * SHUT_WR (Disables further send operations)
    //   * SHUT_RDWR (Disables further send and receive operations)

    std::unique_lock lock(_connectionMutex);

#ifdef WIN32
    shutdown(lSocket, SD_BOTH);
    closesocket(lSocket);
#else
    shutdown(lSocket, SHUT_RDWR);
    close(lSocket);
#endif
}

void Network::setBufferSize(uint32_t newSize) {
    _requestedSize = newSize;
}

int Network::iterateFrameCounter() {
    _previousSendFrame.store(_currentSendFrame.load());

    if (_currentSendFrame < MaxNetworkSyncFrameNumber) {
        _currentSendFrame++;
    }
    else {
        _currentSendFrame = 0;
    }

    _isUpdated = false;


    {
        std::unique_lock lock(_connectionMutex);
        _timeStampSend = Engine::getTime();
    }

    return _currentSendFrame;
}

void Network::pushClientMessage() {
    // The servers' render function is locked until a message starting with the ack-byte
    // is received.
    int currentFrame = iterateFrameCounter();
    unsigned char* p = reinterpret_cast<unsigned char*>(&currentFrame);

    if (MessageHandler::instance().getDataSize() > HeaderSize) {
        core::mutex::DataSync.lock();

        // abock (2019-08-26):  Why is this using the buffer from the MessageHandler even
        // though it has nothing to do with the messaging?

        // Don't remove this pointer, somehow the send function doesn't
        // work during the first call without setting the pointer first!!!
        char* messageToSend = MessageHandler::instance().getMessage();
        messageToSend[0] = Network::DataId;
        messageToSend[1] = p[0];
        messageToSend[2] = p[1];
        messageToSend[3] = p[2];
        messageToSend[4] = p[3];

        // crop if needed
        uint32_t size = _bufferSize;
        uint32_t messageSize = std::min(
            static_cast<uint32_t>(MessageHandler::instance().getDataSize()),
            size
        );

        uint32_t dataSize = messageSize - HeaderSize;
        unsigned char* messageSizePtr = reinterpret_cast<unsigned char*>(&dataSize);
        messageToSend[5] = messageSizePtr[0];
        messageToSend[6] = messageSizePtr[1];
        messageToSend[7] = messageSizePtr[2];
        messageToSend[8] = messageSizePtr[3];
        
        // fill rest of header with DefaultId
        memset(messageToSend + 9, DefaultId, 4);

        sendData(
            reinterpret_cast<void*>(messageToSend),
            static_cast<int>(messageSize)
        );

        core::mutex::DataSync.unlock();
        MessageHandler::instance().clearBuffer();
    }
    else {
        char tmpca[HeaderSize];
        tmpca[0] = Network::DataId;
        tmpca[1] = p[0];
        tmpca[2] = p[1];
        tmpca[3] = p[2];
        tmpca[4] = p[3];

        uint32_t localSyncHeaderSize = 0;
        unsigned char* currentMessageSizePtr = reinterpret_cast<unsigned char*>(
            &localSyncHeaderSize
        );
        tmpca[5] = currentMessageSizePtr[0];
        tmpca[6] = currentMessageSizePtr[1];
        tmpca[7] = currentMessageSizePtr[2];
        tmpca[8] = currentMessageSizePtr[3];
        
        // fill rest of header with DefaultId
        memset(tmpca + 9, DefaultId, 4);

        sendData(reinterpret_cast<void*>(tmpca), HeaderSize);
    }
}

void Network::enableNaglesAlgorithmInDataTransfer() {
    _useNaglesAlgorithmInTransfer = true;
}

int Network::getSendFrameCurrent() const {
    return _currentSendFrame;
}

int Network::getSendFramePrevious() const {
    return _previousSendFrame;
}

int Network::getRecvFrameCurrent() const {
    return _currentRecvFrame;
}

int Network::getRecvFramePrevious() const {
    return _previousRecvFrame;
}

double Network::getLoopTime() const {
    std::unique_lock lock(_connectionMutex);
    return _timeStampTotal;
}

bool Network::isUpdated() const {
    bool state = false;
    if (_isServer) {
        state = ClusterManager::instance().getFirmFrameLockSyncStatus() ?
            // master sends first -> so on reply they should be equal
            (_currentRecvFrame == _currentSendFrame) :
            // don't check if loose sync
            true;
    }
    else {
        state = ClusterManager::instance().getFirmFrameLockSyncStatus() ?
            // slaves receive first and then send so the prev should be equal to the send
            (_previousRecvFrame == _currentSendFrame) :
            // if loose sync just check if updated
            _isUpdated.load();
    }

    return (state && _isConnected);
}

void Network::setDecodeFunction(std::function<void(const char*, int, int)> fn) {
    decoderCallback = std::move(fn);
}

void Network::setPackageDecodeFunction(std::function<void(void*, int, int, int)> fn) {
    _packageDecoderCallback = std::move(fn);
}

void Network::setUpdateFunction(std::function<void(Network*)> fn) {
    _updateCallback = std::move(fn);
}

void Network::setConnectedFunction(std::function<void(void)> fn) {
    _connectedCallback = std::move(fn);
}

void Network::setAcknowledgeFunction(std::function<void(int, int)> fn) {
    _acknowledgeCallback = std::move(fn);
}

void Network::setConnectedStatus(bool state) {
    std::unique_lock lock(_connectionMutex);
    _isConnected = state;
}

bool Network::isConnected() const {
    return _isConnected;
}

Network::ConnectionType Network::getType() const {
    std::unique_lock lock(_connectionMutex);
    return _connectionType;
}

int Network::getId() const {
    return _id;
}

bool Network::isServer() const {
    return _isServer;
}

bool Network::isTerminated() const {
    return _shouldTerminate;
}

void Network::setRecvFrame(int i) {
    _previousRecvFrame.store(_currentRecvFrame.load());
    _currentRecvFrame = i;
    _isUpdated = true;

    std::unique_lock lock(_connectionMutex);
    _timeStampTotal = Engine::getTime() - _timeStampSend;
}

int Network::getLastError() {
    return SGCT_ERRNO;
}

int Network::receiveData(SGCT_SOCKET& lsocket, char* buffer, int length, int flags) {
    int iResult = 0;
    int attempts = 1;

    while (iResult < length) {
        int tmpRes = recv(lsocket, buffer + iResult, length - iResult, flags);
        if (tmpRes > 0) {
            iResult += tmpRes;
        }
#ifdef WIN32
        else if (SGCT_ERRNO == WSAEINTR && attempts <= MaxNumberOfAttempts)
#else
        else if (SGCT_ERRNO == EINTR && attempts <= MaxNumberOfAttempts)
#endif
        {
            MessageHandler::printWarning(
                "Receiving data after interrupted system error (attempt %d)", attempts
            );
            attempts++;
        }
        else {
            // capture error
            iResult = tmpRes;
            break;
        }
    }

    return iResult;
}


void Network::updateBuffer(std::vector<char>& buffer, uint32_t reqSize,
                           uint32_t& currSize)
{
    // grow only
    if (reqSize <= currSize) {
        return;
    }

    std::unique_lock lock(_connectionMutex);
    buffer.resize(reqSize);
    currSize = reqSize;
}

int Network::readSyncMessage(char* header, int32_t& syncFrameNumber, uint32_t& dataSize,
                             uint32_t& uncompressedDataSize)
{
    int iResult = receiveData(_socket, header, static_cast<int>(HeaderSize), 0);

    if (iResult == static_cast<int>(HeaderSize)) {
        _headerId = header[0];
        if (_headerId == DataId || _headerId == CompressedDataId) {
            // parse the sync frame number
            syncFrameNumber = parseInt32(&header[1]);
            // parse the data size
            dataSize = parseUInt32(&header[5]);
            // parse the uncompressed size if compression is used
            uncompressedDataSize = parseUInt32(&header[9]);

            setRecvFrame(syncFrameNumber);
            if (syncFrameNumber < 0) {
                MessageHandler::printError(
                    "Error sync in sync frame: %d for connection %d", syncFrameNumber, _id
                );
            }

            // resize buffer if needed
            updateBuffer(_recvBuffer, dataSize, _bufferSize);
            updateBuffer(_uncompressBuffer, uncompressedDataSize, _uncompressedBufferSize);
        }
    }

    // Get the data/message
    if (dataSize > 0) {
        iResult = receiveData(_socket, _recvBuffer.data(), dataSize, 0);
    }

    return iResult;
}

int Network::readDataTransferMessage(char* header, int32_t& packageId, uint32_t& dataSize,
                                     uint32_t& uncompressedDataSize)
{
    int iResult = receiveData(_socket, header, static_cast<int>(HeaderSize), 0);

    if (iResult == static_cast<int>(HeaderSize)) {
        _headerId = header[0];
        if (_headerId == DataId || _headerId == CompressedDataId) {
            // parse the package _id
            packageId = parseInt32(&header[1]);
            // parse the data size
            dataSize = parseUInt32(&header[5]);
            // parse the uncompressed size if compression is used
            uncompressedDataSize = parseUInt32(&header[9]);

            // resize buffer if needed
            updateBuffer(_recvBuffer, dataSize, _bufferSize);
            updateBuffer(_uncompressBuffer, uncompressedDataSize, _uncompressedBufferSize);
        }
        else if (_headerId == Ack && _acknowledgeCallback != nullptr) {
            //parse the package _id
            packageId = parseInt32(&header[1]);
            _acknowledgeCallback(packageId, _id);
        }
    }

    // Get the data/message
    if (dataSize > 0 && packageId > -1) {
        iResult = receiveData(_socket, _recvBuffer.data(), dataSize, 0);
    }

    return iResult;
}

int Network::readExternalMessage() {
    // do a normal read
    int iResult = recv(_socket, _recvBuffer.data(), _bufferSize, 0);

    // if read fails try for x attempts
    int attempts = 1;
#ifdef WIN32
    while (iResult <= 0 && SGCT_ERRNO == WSAEINTR && attempts <= MaxNumberOfAttempts)
#else
    while (iResult <= 0 && SGCT_ERRNO == EINTR && attempts <= MaxNumberOfAttempts)
#endif
    {
        iResult = recv(_socket, _recvBuffer.data(), _bufferSize, 0);

        MessageHandler::printInfo(
            "Receiving data after interrupted system error (attempt %d)", attempts
        );
        attempts++;
    }
    
    return iResult;
}

void Network::communicationHandler() {
    // exit if terminating
    if (isTerminated()) {
        return;
    }

    // listen for client if server
    if (_isServer) {
        MessageHandler::printInfo(
            "Waiting for client to connect to connection %d (port %d)", _id, getPort()
        );

        _socket = accept(_listenSocket, nullptr, nullptr);

        int accErr = SGCT_ERRNO;
#ifdef WIN32
        while (!isTerminated() && _socket == INVALID_SOCKET && accErr == WSAEINTR)
#else
        while ( !isTerminated() && _socket == INVALID_SOCKET && accErr == EINTR)
#endif
        {
            MessageHandler::printInfo(
                "Re-accept after interrupted system on connection %d", _id
            );

            _socket = accept(_listenSocket, nullptr, nullptr);
        }

        if (_socket == INVALID_SOCKET) {
            MessageHandler::printError(
                "Accept connection %d failed. Error: %d", _id, accErr
            );

            if (_updateCallback) {
                _updateCallback(this);
            }
            return;
        }
    }

    setConnectedStatus(true);
    MessageHandler::printInfo("Connection %d established", _id);

    if (_updateCallback) {
        _updateCallback(this);
    }

    // init buffers
    char RecvHeader[HeaderSize];
    memset(RecvHeader, DefaultId, HeaderSize);

    _connectionMutex.lock();
    _recvBuffer.resize(_bufferSize);
    _uncompressBuffer.resize(_uncompressedBufferSize);
    _connectionMutex.unlock();
    
    std::string extBuffer; // for external communication

    // Receive data until the server closes the connection
    int iResult = 0;
    do {
        // resize buffer request
        if (getType() != ConnectionType::DataTransfer && _requestedSize > _bufferSize) {
            MessageHandler::printInfo(
                "Re-sizing TCP buffer size from %d to %d",
                _bufferSize, _requestedSize.load()
            );

            updateBuffer(_recvBuffer, _requestedSize, _bufferSize);

            MessageHandler::printInfo("Done");
        }
        int32_t packageId = -1;
        int32_t syncFrameNumber = -1;
        uint32_t dataSize = 0;
        uint32_t uncompressedDataSize = 0;
        
        _headerId = DefaultId;

        if (getType() == ConnectionType::SyncConnection) {
            iResult = readSyncMessage(
                RecvHeader,
                syncFrameNumber,
                dataSize,
                uncompressedDataSize
            );
        }
        else if (getType() == ConnectionType::DataTransfer) {
            iResult = readDataTransferMessage(
                RecvHeader,
                packageId,
                dataSize,
                uncompressedDataSize
            );
        }
        else {
            iResult = readExternalMessage();
        }

        // if data was read successfully, then decode it
        if (iResult > 0) {
            if (getType() == ConnectionType::SyncConnection) {
                // handle sync disconnect
                if (parseDisconnectPackage(RecvHeader)) {
                    setConnectedStatus(false);

                    // Terminate client only. The server only resets the connection,
                    // allowing clients to connect.
                    if (!_isServer) {
                        _shouldTerminate = true;
                    }

                    MessageHandler::printInfo("Client %d terminated connection", _id);

                    break;
                }
                // handle sync communication
                if (_headerId == DataId && decoderCallback != nullptr) {
                    // decode callback
                    if (dataSize > 0) {
                        decoderCallback(_recvBuffer.data(), dataSize, _id);
                    }

                    NetworkManager::cond.notify_all();
                }
                else if (_headerId == CompressedDataId && decoderCallback) {
                    //decode callback
                    if (dataSize > 0) {
                        // parse the package _id
                        uLongf uncompSize = static_cast<uLongf>(uncompressedDataSize);
    
                        int err = uncompress(
                            reinterpret_cast<Bytef*>(_uncompressBuffer.data()),
                            &uncompSize,
                            reinterpret_cast<Bytef*>(_recvBuffer.data()),
                            static_cast<uLongf>(dataSize)
                        );
                            
                        if (err == Z_OK) {
                            // decode callback
                            decoderCallback(
                                _uncompressBuffer.data(),
                                static_cast<int>(uncompSize),
                                _id
                            );
                        }
                        else {
                            MessageHandler::printError(
                                "Failed to uncompress data for connection %d. Error: %s",
                                _id, getUncompressionErrorAsStr(err).c_str()
                            );
                        }
                    }
                        
                    NetworkManager::cond.notify_all();
                }
                else if (_headerId == ConnectedId && _connectedCallback) {
                    _connectedCallback();
                    NetworkManager::cond.notify_all();
                }
            }
            // handle external ascii communication
            else if (getType() == ConnectionType::ExternalConnection) {
                extBuffer += std::string(_recvBuffer.data()).substr(0, iResult);

                bool breakConnection = false;
                // look for cancel
                if (extBuffer.find(24) != std::string::npos) {
                    breakConnection = true;
                }
                // look for escape
                if (extBuffer.find(27) != std::string::npos) {
                    breakConnection = true;
                }
                // look for logout
                if (extBuffer.find("logout") != std::string::npos) {
                    breakConnection = true;
                }
                // look for close
                if (extBuffer.find("close") != std::string::npos) {
                    breakConnection = true;
                }
                // look for exit
                if (extBuffer.find("exit") != std::string::npos) {
                    breakConnection = true;
                }
                // look for quit
                if (extBuffer.find("quit") != std::string::npos) {
                    breakConnection = true;
                }

                if (breakConnection) {
                    setConnectedStatus(false);
                    break;
                }

                // separate messages by <CR><NL>
                size_t found = extBuffer.find("\r\n");
                while (found != std::string::npos) {
                    std::string extMessage = extBuffer.substr(0, found);
                    extBuffer = extBuffer.substr(found + 2); //jump over \r\n

                    if (decoderCallback) {
                        decoderCallback(
                            extMessage.c_str(),
                            static_cast<int>(extMessage.size()),
                            _id
                        );
                    }

                    // reply
                    std::string msg = "OK\r\n";
                    sendData(msg.c_str(), static_cast<int>(msg.size()));
                    found = extBuffer.find("\r\n");
                }
            }
            // handle data transfer communication
            else if (getType() == ConnectionType::DataTransfer) {
                // Disconnect if requested
                if (parseDisconnectPackage(RecvHeader)) {
                    setConnectedStatus(false);
                    MessageHandler::printInfo(
                        "File transfer %d terminated connection", _id
                    );
                }
                //  Handle communication
                else {
                    if ((_headerId == DataId || _headerId == CompressedDataId) &&
                        _packageDecoderCallback && dataSize > 0)
                    {
                        bool recvOk = false;
                        
                        if (_headerId == DataId) {
                            // uncompressed
                            // decode callback
                            _packageDecoderCallback(
                                _recvBuffer.data(),
                                dataSize,
                                packageId,
                                _id
                            );
                            recvOk = true;
                        }
                        else {
                            // compressed
                            // parse the package _id
                            uLongf uncompressedSize = static_cast<uLongf>(
                                uncompressedDataSize
                            );
                            
                            int err = uncompress(
                                reinterpret_cast<Bytef*>(_uncompressBuffer.data()),
                                &uncompressedSize,
                                reinterpret_cast<Bytef*>(_recvBuffer.data()),
                                static_cast<uLongf>(dataSize)
                            );
                            
                            if (err == Z_OK) {
                                // decode callback
                                _packageDecoderCallback(
                                    _uncompressBuffer.data(),
                                    static_cast<int>(uncompressedSize),
                                    packageId,
                                    _id
                                );
                                recvOk = true;
                            }
                            else {
                                MessageHandler::printError(
                                    "Failed to uncompress data for connection %d. %s",
                                    _id, getUncompressionErrorAsStr(err).c_str()
                                );
                            }
                        }
                        
                        if (recvOk) {
                            //send acknowledge
                            char sendBuff[HeaderSize];
                            uint32_t pLength = 0;
                            char* packageIdPtr = reinterpret_cast<char*>(&packageId);
                            char* sizeDataPtr = reinterpret_cast<char*>(&pLength);
                            
                            sendBuff[0] = Ack;
                            sendBuff[1] = packageIdPtr[0];
                            sendBuff[2] = packageIdPtr[1];
                            sendBuff[3] = packageIdPtr[2];
                            sendBuff[4] = packageIdPtr[3];
                            sendBuff[5] = sizeDataPtr[0];
                            sendBuff[6] = sizeDataPtr[1];
                            sendBuff[7] = sizeDataPtr[2];
                            sendBuff[8] = sizeDataPtr[3];

                            sendData(sendBuff, HeaderSize);
                        }

                        // Clear the buffer
                        _connectionMutex.lock();

                        // clean up
                        _recvBuffer.clear();
                        _uncompressBuffer.clear();
                        
                        _bufferSize = 0;
                        _uncompressedBufferSize = 0;
                        _connectionMutex.unlock();
                    }
                    else if (_headerId == ConnectedId && _connectedCallback) {
                        _connectedCallback();
                        NetworkManager::cond.notify_all();
                    }
                }
            }
        }
        // handle failed receive
        else if (iResult == 0) {
            setConnectedStatus(false);
            MessageHandler::printError(
                "TCP Connection %d closed (error: %d)", _id, SGCT_ERRNO
            );
        }
        else {
        // if negative
            setConnectedStatus(false);
            MessageHandler::printError(
                "TCP connection %d recv failed: %d", _id, SGCT_ERRNO
            );
        }
    } while (iResult > 0 || _isConnected);

    // cleanup
    _recvBuffer.clear();
    _uncompressBuffer.clear();

    // Close socket; contains mutex
    closeSocket(_socket);

    if (_updateCallback) {
        _updateCallback(this);
    }

    MessageHandler::printInfo("Node %d disconnected", _id);
}

void Network::sendData(const void* data, int length) {
    int sentLen;
    int sendSize = length;

    while (sendSize > 0) {
        int offset = length - sendSize;
        sentLen = send(
            _socket,
            reinterpret_cast<const char*>(data) + offset,
            sendSize,
            0
        );
        if (sentLen == SOCKET_ERROR) {
            MessageHandler::printError("Send data failed");
            break;
        }
        sendSize -= sentLen;
    }
}

void Network::closeNetwork(bool forced) {
    decoderCallback = nullptr;
    _updateCallback = nullptr;
    _connectedCallback = nullptr;
    _acknowledgeCallback = nullptr;
    _packageDecoderCallback = nullptr;

    // release conditions
    NetworkManager::cond.notify_all();
    _startConnectionCond.notify_all();

    // blocking sockets -> cannot wait for thread so just kill it brutally

    if (_commThread) {
        if (!forced) {
            _commThread->join();
        }
        _commThread = nullptr;
    }

    if (_mainThread) {
        if (!forced) {
            _mainThread->join();
        }
        _mainThread = nullptr;
    }

    MessageHandler::printInfo("Connection %d successfully terminated", _id);
}

void Network::initShutdown() {
    if (_isConnected) {
        char gameOver[9];
        gameOver[0] = DisconnectId;
        gameOver[1] = 24; // ASCII for cancel
        gameOver[2] = '\r';
        gameOver[3] = '\n';
        gameOver[4] = 27; // ASCII for Esc
        gameOver[5] = '\r';
        gameOver[6] = '\n';
        gameOver[7] = '\0';
        gameOver[8] = DefaultId;
        sendData(gameOver, HeaderSize);
    }

    MessageHandler::printInfo("Closing connection %d", _id);

    {
        std::unique_lock lock(_connectionMutex);
        decoderCallback = nullptr;
    }

    _isConnected = false;
    _shouldTerminate = true;

    // wake up the connection handler thread (in order to finish)
    if (_isServer) {
        _startConnectionCond.notify_all();
    }

    closeSocket(_socket);
    closeSocket(_listenSocket);
}

} // namespace sgct::core
