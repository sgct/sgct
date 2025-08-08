/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2025                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/network.h>

#ifdef WIN32
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#define SGCT_ERRNO WSAGetLastError()
#else // ^^^^ WIN32 // !WIN32 vvvv
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <cerrno>
#include <netdb.h>
#include <unistd.h>
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (~0)
#define NO_ERROR 0L
#define SGCT_ERRNO errno
#endif // WIN32

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/mutexes.h>
#include <sgct/networkmanager.h>
#include <sgct/profiling.h>
#include <sgct/shareddata.h>
#include <algorithm>
#include <cstring>

#define Err(code, msg) sgct::Error(sgct::Error::Component::Network, code, msg)

namespace {
    constexpr int MaxNumberOfAttempts = 10;

    constexpr int MaxNetworkSyncFrameNumber = 10000;


    int receiveData(SGCT_SOCKET lsocket, char* buffer, int length, int flags) {
        long iResult = 0;
        int attempts = 1;

        while (iResult < length) {
            const long tmpRes = recv(lsocket, buffer + iResult, length - iResult, flags);
            if (tmpRes > 0) {
                iResult += tmpRes;
            }
#ifdef WIN32
            else if (SGCT_ERRNO == WSAEINTR && attempts <= MaxNumberOfAttempts) {
#else // ^^^^ WIN32 // !WIN32 vvvv
            else if (SGCT_ERRNO == EINTR && attempts <= MaxNumberOfAttempts) {
#endif // WIN32
                sgct::Log::Warning(std::format(
                    "Receiving data after interrupted system error (attempt {})", attempts
                ));
                attempts++;
            }
            else {
                // capture error
                iResult = tmpRes;
                break;
            }
            }

        // POXIX requires `recv` to return ssize_t which is -> long. We are not going to get
        // enough data to fill this, so we should be fine
        return static_cast<int>(iResult);
    }

    void setOptions(SGCT_SOCKET socket, sgct::Network::ConnectionType connectionType) {
        constexpr int TrueFlag = 1;

        // insert no delay, disable nagle's algorithm
        const int delayRes = setsockopt(
            socket,       // socket affected
            IPPROTO_TCP,   // set option at TCP level
            TCP_NODELAY,   // name of option
            reinterpret_cast<const char*>(&TrueFlag), // the cast is historical cruft
            sizeof(int)    // length of option value
        );

        if (delayRes != NO_ERROR) {
            throw Err(
                5005,
                std::format("Failed to set network no-delay option: {}", SGCT_ERRNO)
            );
        }

        // set timeout
        constexpr int Timeout = 0; // infinite
        setsockopt(
            socket,
            SOL_SOCKET,
            SO_SNDTIMEO,
            reinterpret_cast<const char*>(&Timeout),
            sizeof(Timeout)
        );

        const int sockoptRes = setsockopt(
            socket,
            SOL_SOCKET,
            SO_REUSEADDR,
            reinterpret_cast<const char*>(&TrueFlag),
            sizeof(TrueFlag)
        );
        if (sockoptRes == SOCKET_ERROR) {
            throw Err(5006, std::format("Failed to set reuse address: {}", SGCT_ERRNO));
        }

        if (connectionType != sgct::Network::ConnectionType::SyncConnection) {
            // set on all connections types, cluster nodes sends data several times per
            // second so there is no need so send alive packages
            const int iResult = setsockopt(
                socket,
                SOL_SOCKET,
                SO_KEEPALIVE,
                reinterpret_cast<const char*>(&TrueFlag),
                sizeof(TrueFlag)
            );
            if (iResult == SOCKET_ERROR) {
                throw Err(5009, std::format("Failed to set keep alive: {}", SGCT_ERRNO));
            }
        }
    }


    std::string typeStr(sgct::Network::ConnectionType ct) {
        switch (ct) {
            case sgct::Network::ConnectionType::SyncConnection: return "sync";
            case sgct::Network::ConnectionType::DataTransfer:   return "data transfer";
            default:                       throw std::logic_error("Unhandled case label");
        }
    }

    bool isDisconnectPackage(const char* header) {
        constexpr std::array<const char, 8> rhs = {
            sgct::Network::DisconnectId, 24, '\r', '\n', 27, '\r', '\n', '\0'
        };
        return std::string_view(header, 8) == std::string_view(rhs.data(), 8);
    }
} // namespace

namespace sgct {

int Network::lastError() {
    return SGCT_ERRNO;
}

Network::Network(int port, const std::string& address, bool isServer, ConnectionType t)
    : _socket(INVALID_SOCKET)
    , _listenSocket(INVALID_SOCKET)
    , _connectionType(t)
    , _isServer(isServer)
    , _port(port)
{
    static int id = 0;
    _id = id;
    id++;

    if (_connectionType == ConnectionType::SyncConnection) {
        _bufferSize = static_cast<uint32_t>(SharedData::instance().bufferSize());
        _uncompressedBufferSize = _bufferSize;
    }

    addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    const char* a = _isServer ? nullptr : address.c_str();
    std::string portStr = std::to_string(_port);
    addrinfo* res = nullptr;
    const int addrRes = getaddrinfo(a, portStr.c_str(), &hints, &res);
    if (addrRes != 0) {
        throw Err(5000, "Failed to parse hints for connection");
    }

    if (_isServer) {
        // Create a SOCKET for the server to listen for client connections
        _listenSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (_listenSocket == INVALID_SOCKET) {
            freeaddrinfo(res);
            throw Err(5001, "Failed to listen init socket");
        }

        setOptions(_listenSocket, type());

        // Setup the TCP listening socket
        const int addrlen = static_cast<int>(res->ai_addrlen);
        const int bindResult = bind(_listenSocket, res->ai_addr, addrlen);
        if (bindResult == SOCKET_ERROR) {
            freeaddrinfo(res);
#ifdef WIN32
            closesocket(_listenSocket);
#else // ^^^^ WIN32 // !WIN32 vvvv
            close(_listenSocket);
#endif // WIN32
            throw Err(5002, "Bind socket call failed");
        }

        if (listen(_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            freeaddrinfo(res);
#ifdef WIN32
            closesocket(_listenSocket);
#else // ^^^^ WIN32 // !WIN32 vvvv
            close(_listenSocket);
#endif // WIN32
            throw Err(5003, "Listen call failed");
        }
    }
    else {
        // Client socket: Connect to server
        while (!_shouldTerminate) {
            Log::Info(std::format(
                "Attempting to connect to server (id: {}, ip: {}, type: {})",
                _id, address, typeStr(type())
            ));

            _socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (_socket == INVALID_SOCKET) {
                freeaddrinfo(res);
                throw Err(5004, "Failed to init client socket");
            }

            setOptions(_socket, type());

            const int r = connect(
                _socket,
                res->ai_addr,
                static_cast<int>(res->ai_addrlen)
            );
            if (r != SOCKET_ERROR) {
                break;
            }

            if (SGCT_ERRNO == 10061) {
                Log::Debug("Waiting for connection...");
            }
            else {
                Log::Debug(std::format("Connect error code: {}", SGCT_ERRNO));
            }
            std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for next attempt
        }
    }

    freeaddrinfo(res);
}

Network::~Network() {
    closeNetwork(false);
}

void Network::initialize() {
    _mainThread = std::make_unique<std::thread>([this]() { connectionHandler(); });
}

void Network::connectionHandler() {
    if (_isServer) {
        while (!_shouldTerminate) {
            if (!_isConnected) {
                if (_commThread) {
                    _commThread->join();
                    _commThread = nullptr;
                }

                // start a new connection enabling the client to reconnect
                _commThread = std::make_unique<std::thread>([this]() {
                    try {
                        communicationHandler();
                    }
                    catch (const std::runtime_error& e) {
                        Log::Error(e.what());
                    }
                });
            }

            // wait for signal until next iteration in loop
            if (!_shouldTerminate) {
                std::unique_lock lk(_connectionMutex);
                _startConnectionCond.wait(lk);
            }
        }
    }
    else {
        _commThread = std::make_unique<std::thread>([this]() {
            try {
                communicationHandler();
            }
            catch (const std::runtime_error& e) {
                Log::Error(e.what());
            }
        });
    }

    Log::Info(std::format("Exiting connection handler for connection {}", _id));
}

int Network::port() const {
    return _port;
}

std::condition_variable& Network::startConnectionConditionVar() {
    return _startConnectionCond;
}

void Network::closeSocket(SGCT_SOCKET socket) {
    if (socket == INVALID_SOCKET) {
        return;
    }

    const std::unique_lock lock(_connectionMutex);

#ifdef WIN32
    shutdown(socket, SD_BOTH);
    closesocket(socket);
#else // ^^^^ WIN32 // !WIN32 vvvv
    shutdown(socket, SHUT_RDWR);
    close(socket);
#endif // WIN32
}

int Network::iterateFrameCounter() {
    _currentSendFrame = (_currentSendFrame + 1) % MaxNetworkSyncFrameNumber;
    _isUpdated = false;

    {
        const std::unique_lock lock(_connectionMutex);
        _timeStampSend = time();
    }

    return _currentSendFrame;
}

void Network::pushClientMessage() {
    // The servers' render function is locked until an ack message is received
    const int currentFrame = iterateFrameCounter();
    uint32_t localSyncHeaderSize = 0;

    std::array<char, HeaderSize> data = {};
    data[0] = Network::DataId;
    std::memcpy(data.data() + 1, &currentFrame, sizeof(currentFrame));
    std::memcpy(data.data() + 5, &localSyncHeaderSize, sizeof(localSyncHeaderSize));
    std::memset(data.data() + 9, DefaultId, 4);
    sendData(data.data(), HeaderSize);
}

int Network::sendFrameCurrent() const {
    return _currentSendFrame;
}

int Network::recvFrameCurrent() const {
    return _currentRecvFrame;
}

int Network::recvFramePrevious() const {
    return _previousRecvFrame;
}

double Network::loopTime() const {
    return _timeStampTotal;
}

bool Network::isUpdated() const {
    bool state = false;
    if (_isServer) {
        state = ClusterManager::instance().firmFrameLockSyncStatus() ?
            // master sends first -> so on reply they should be equal
            (_currentRecvFrame == _currentSendFrame) :
            // don't check if loose sync
            true;
    }
    else {
        state = ClusterManager::instance().firmFrameLockSyncStatus() ?
            // clients receive first and then send so the prev should be equal to the send
            (_previousRecvFrame == _currentSendFrame) :
            // if loose sync just check if updated
            _isUpdated.load();
    }

    return (state && _isConnected);
}

void Network::setDecodeFunction(std::function<void(const char*, int)> fn) {
    decoderCallback = std::move(fn);
}

void Network::setPackageDecodeFunction(std::function<void(void*, int, int, int)> fn) {
    _packageDecoderCallback = std::move(fn);
}

void Network::setUpdateFunction(std::function<void(Network&)> fn) {
    _updateCallback = std::move(fn);
}

void Network::setConnectedFunction(std::function<void(void)> fn) {
    _connectedCallback = std::move(fn);
}

void Network::setAcknowledgeFunction(std::function<void(int, int)> fn) {
    _acknowledgeCallback = std::move(fn);
}

void Network::setConnectedStatus(bool state) {
    const std::unique_lock lock(_connectionMutex);
    _isConnected = state;
}

bool Network::isConnected() const {
    return _isConnected;
}

Network::ConnectionType Network::type() const {
    const std::unique_lock lock(_connectionMutex);
    return _connectionType;
}

int Network::id() const {
    return _id;
}

bool Network::isServer() const {
    return _isServer;
}

void Network::setRecvFrame(int i) {
    _previousRecvFrame.store(_currentRecvFrame.load());
    _currentRecvFrame = i;
    _isUpdated = true;
    _timeStampTotal = time() - _timeStampSend;
}

void Network::updateBuffer(std::vector<char>& buffer, uint32_t reqSize,
                           uint32_t& currSize)
{
    // only grow
    if (reqSize <= currSize) {
        return;
    }

    const std::unique_lock lock(_connectionMutex);
    buffer.resize(reqSize);
    currSize = reqSize;
}

int Network::readSyncMessage(char* header, int32_t& syncFrame, uint32_t& dataSize,
                             uint32_t& uncompressedDataSize)
{
    int iResult = receiveData(_socket, header, static_cast<int>(HeaderSize), 0);

    if (iResult == static_cast<int>(HeaderSize)) {
        _headerId = header[0];
        if (_headerId == DataId) {
            std::memcpy(&syncFrame, header + 1, sizeof(syncFrame));
            std::memcpy(&dataSize, header + 5, sizeof(dataSize));
            std::memcpy(&uncompressedDataSize, header + 9, sizeof(uncompressedDataSize));

            setRecvFrame(syncFrame);
            if (syncFrame < 0) {
                throw Err(
                    5010,
                    std::format(
                        "Error in sync frame {} for connection {}", syncFrame, _id
                    )
                );
            }

            // resize buffer if needed
            updateBuffer(_recvBuffer, dataSize, _bufferSize);
            updateBuffer(
                _uncompressBuffer,
                uncompressedDataSize,
                _uncompressedBufferSize
            );
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
        if (_headerId == DataId) {
            // parse the package _id
            std::memcpy(&packageId, header + 1, sizeof(packageId));
            std::memcpy(&dataSize, header + 5, sizeof(dataSize));
            std::memcpy(&uncompressedDataSize, header + 9, sizeof(uncompressedDataSize));

            // resize buffer if needed
            updateBuffer(_recvBuffer, dataSize, _bufferSize);
            updateBuffer(
                _uncompressBuffer,
                uncompressedDataSize,
                _uncompressedBufferSize
            );
        }
        else if (_headerId == Ack && _acknowledgeCallback != nullptr) {
            std::memcpy(&packageId, header + 1, sizeof(packageId));
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
    long iResult = recv(_socket, _recvBuffer.data(), _bufferSize, 0);

    // if read fails try for x attempts
    int attempts = 1;
#ifdef WIN32
    while (iResult <= 0 && SGCT_ERRNO == WSAEINTR && attempts <= MaxNumberOfAttempts) {
#else // ^^^^ WIN32 // !WIN32 vvvv
    while (iResult <= 0 && SGCT_ERRNO == EINTR && attempts <= MaxNumberOfAttempts) {
#endif // WIN32
        iResult = recv(_socket, _recvBuffer.data(), _bufferSize, 0);
        Log::Info(std::format(
            "Receiving data after interrupted system error (attempt {})", attempts
        ));
        attempts++;
    }

    return static_cast<int>(iResult);
}

void Network::communicationHandler() {
    if (_shouldTerminate) {
        return;
    }

    // listen for client if server
    if (_isServer) {
        Log::Info(
            std::format("Waiting for client {} to connect on port {}", _id, _port)
        );

        _socket = accept(_listenSocket, nullptr, nullptr);

#ifdef WIN32
        while (!_shouldTerminate && _socket == INVALID_SOCKET && SGCT_ERRNO == WSAEINTR) {
#else // ^^^^ WIN32 // !WIN32 vvvv
        while (!_shouldTerminate && _socket == INVALID_SOCKET && SGCT_ERRNO == EINTR) {
#endif // WIN32
            Log::Info(
                std::format("Re-accept after interrupted system on connection {}", _id)
            );
            _socket = accept(_listenSocket, nullptr, nullptr);
        }

        if (_socket == INVALID_SOCKET) {
            Log::Error(
                std::format("Accept connection {} failed. Error: {}", _id, SGCT_ERRNO)
            );

            if (_updateCallback) {
                _updateCallback(*this);
            }
            return;
        }
    }

    setConnectedStatus(true);
    Log::Info(std::format("Connection {} established", _id));

    if (_updateCallback) {
        _updateCallback(*this);
    }

    // init buffers
    std::array<char, HeaderSize> RecvHeader = {};
    std::memset(RecvHeader.data(), DefaultId, HeaderSize);

    {
        const std::unique_lock lk(_connectionMutex);
        _recvBuffer.resize(_bufferSize);
        _uncompressBuffer.resize(_uncompressedBufferSize);
    }

    // Receive data until the server closes the connection
    int iResult = 0;
    do {
        // resize buffer request
        if (type() != ConnectionType::DataTransfer && _requestedSize > _bufferSize) {
            Log::Info(std::format(
                "Re-sizing buffer {} -> {}", _bufferSize, _requestedSize.load()
            ));
            updateBuffer(_recvBuffer, _requestedSize, _bufferSize);
        }
        int32_t packageId = -1;
        uint32_t dataSize = 0;
        uint32_t uncompressedDataSize = 0;

        _headerId = DefaultId;

        if (type() == ConnectionType::SyncConnection) {
            int32_t syncFrameNumber = -1;
            iResult = readSyncMessage(
                RecvHeader.data(),
                syncFrameNumber,
                dataSize,
                uncompressedDataSize
            );
        }
        else if (type() == ConnectionType::DataTransfer) {
            iResult = readDataTransferMessage(
                RecvHeader.data(),
                packageId,
                dataSize,
                uncompressedDataSize
            );
        }
        else {
            iResult = readExternalMessage();
        }

        // handle failed receive
        if (iResult == 0) {
            setConnectedStatus(false);
            Log::Info(std::format("TCP connection {} closed", _id));
        }
        else if (iResult < 0) {
            setConnectedStatus(false);
            throw Err(
                5013,
                std::format("TCP connection {} receive failed: {}", _id, SGCT_ERRNO)
            );
        }

        if (type() == ConnectionType::SyncConnection) {
            // handle sync disconnect
            if (isDisconnectPackage(RecvHeader.data())) {
                setConnectedStatus(false);

                // Terminate client only. The server only resets the connection,
                // allowing clients to connect.
                if (!_isServer) {
                    _shouldTerminate = true;
                }

                Log::Info(std::format("Client {} terminated connection", _id));
                break;
            }
            // handle sync communication
            if (_headerId == DataId && decoderCallback) {
                if (dataSize > 0) {
                    decoderCallback(_recvBuffer.data(), dataSize);
                }

                NetworkManager::cond.notify_all();
            }
            else if (_headerId == ConnectedId && _connectedCallback) {
                _connectedCallback();
                NetworkManager::cond.notify_all();
            }
        }
        // handle data transfer communication
        else if (type() == ConnectionType::DataTransfer) {
            // Disconnect if requested
            if (isDisconnectPackage(RecvHeader.data())) {
                setConnectedStatus(false);
                Log::Info(std::format("File connection {} terminated", _id));
            }
            //  Handle communication
            else {
                if (_headerId == DataId && _packageDecoderCallback && dataSize > 0) {
                    _packageDecoderCallback(_recvBuffer.data(), dataSize, packageId, _id);

                    // send acknowledge
                    uint32_t pLength = 0;
                    std::array<char, HeaderSize> sendBuffer = {};
                    sendBuffer[0] = Ack;
                    std::memcpy(sendBuffer.data() + 1, &packageId, sizeof(packageId));
                    std::memcpy(sendBuffer.data() + 5, &pLength, sizeof(pLength));
                    sendData(sendBuffer.data(), HeaderSize);

                    {
                        // Clear the buffers
                        const std::unique_lock lk(_connectionMutex);

                        _recvBuffer.clear();
                        _uncompressBuffer.clear();

                        _bufferSize = 0;
                        _uncompressedBufferSize = 0;
                    }
                }
                else if (_headerId == ConnectedId && _connectedCallback) {
                    _connectedCallback();
                    NetworkManager::cond.notify_all();
                }
            }
        }
    } while (iResult > 0 || _isConnected);

    _recvBuffer.clear();
    _uncompressBuffer.clear();

    // Close socket; contains mutex
    closeSocket(_socket);

    if (_updateCallback) {
        _updateCallback(*this);
    }

    Log::Info(std::format("Node {} disconnected", _id));
}

void Network::sendData(const void* data, int length) const {
    ZoneScoped;

    long sendSize = length;

    while (sendSize > 0) {
        const int offset = static_cast<int>(length - sendSize);
        const long sentLen = send(
            _socket,
            reinterpret_cast<const char*>(data) + offset,
            sendSize,
            0
        );
        if (sentLen == SOCKET_ERROR) {
            throw Err(5014, std::format("Send data failed: {}", SGCT_ERRNO));
        }
        sendSize -= sentLen;
    }
}

void Network::closeNetwork(bool forced) {
    ZoneScoped;

    decoderCallback = nullptr;
    _updateCallback = nullptr;
    _connectedCallback = nullptr;
    _acknowledgeCallback = nullptr;
    _packageDecoderCallback = nullptr;

    // release conditions
    NetworkManager::cond.notify_all();
    _startConnectionCond.notify_all();

    // blocking sockets -> cannot wait for thread so just kill it brutally

    if (_commThread && !forced) {
        _commThread->join();
    }
    _commThread = nullptr;

    if (_mainThread && !forced) {
        _mainThread->join();
    }
    _mainThread = nullptr;

    Log::Info(std::format("Connection {} successfully terminated", _id));
}

void Network::initShutdown() {
    ZoneScoped;

    if (_isConnected) {
        constexpr std::array<char, 9> GameOver = {
            DisconnectId, 24, '\r', '\n', 27, '\r', '\n', '\0', DefaultId
        };
        sendData(GameOver.data(), HeaderSize);
    }

    Log::Info(std::format("Closing connection {}", _id));

    {
        ZoneScopedN("Decoder callback lock");
        const std::unique_lock lock(_connectionMutex);
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

} // namespace sgct
