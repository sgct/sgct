/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

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
    #define INVALID_SOCKET (SGCT_SOCKET)(~0)
    #define NO_ERROR 0L
    #define SGCT_ERRNO errno
#endif

#include <sgct/engine.h>
#include <sgct/clustermanager.h>
#include <sgct/messagehandler.h>
#include <sgct/networkmanager.h>
#include <sgct/mutexmanager.h>
#include <sgct/shareddata.h>

#include <zlib.h>
#include <algorithm>

namespace {
    constexpr const int MaxNumberOfAttempts = 10;
    constexpr const int SocketBufferSize = 4096;

    constexpr const int MAX_NET_SYNC_FRAME_NUMBER = 10000;

    int32_t parseInt32(char* str) {
        int32_t val = *(reinterpret_cast<int32_t*>(str));
        return val;
    }

    uint32_t parseUInt32(char* str) {
        uint32_t val = *(reinterpret_cast<uint32_t*>(str));
        return val;
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

    bool parseDisconnectPackage(char* headerPtr) {
        return (headerPtr[0] == sgct::core::Network::DisconnectId &&
                headerPtr[1] == 24 && headerPtr[2] == '\r' && headerPtr[3] == '\n' &&
                headerPtr[4] == 27 && headerPtr[5] == '\r' &&
                headerPtr[6] == '\n' && headerPtr[7] == '\0');
    }
} // namespace

namespace sgct::core {

Network::Network()
    : mSocket(INVALID_SOCKET)
    , mListenSocket(INVALID_SOCKET)
{
    static int id = 0;
    mId = id;
    id++;
}

Network::~Network() {
    closeNetwork(false);
}

void Network::init(std::string port, std::string address, bool isServer,
                   ConnectionType type)
{
    mServer = isServer;
    mConnectionType = type;
    if (mConnectionType == ConnectionType::SyncConnection) {
        mBufferSize = static_cast<uint32_t>(SharedData::instance()->getBufferSize());
        mUncompressedBufferSize = mBufferSize;
    }

    mPort = std::move(port);
    mAddress = std::move(address);

    addrinfo* res = nullptr;
    addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    int addrRes = getaddrinfo(
        mServer ? nullptr : mAddress.c_str(),
        mPort.c_str(),
        &hints,
        &res
    );
    if (addrRes != 0) {
        throw std::runtime_error("Failed to parse hints for connection");
    }

    if (mServer) {
        // Create a SOCKET for the server to listen for client connections
        mListenSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (mListenSocket == INVALID_SOCKET) {
            freeaddrinfo(res);
            throw std::runtime_error("Failed to listen init socket");
        }

        setOptions(&mListenSocket);

        // Setup the TCP listening socket
        int bindResult = bind(
            mListenSocket,
            res->ai_addr,
            static_cast<int>(res->ai_addrlen)
        );
        if (bindResult == SOCKET_ERROR) {
            freeaddrinfo(res);
#ifdef __WIN32__
            closesocket(mListenSocket);
#else
            close(mListenSocket);
#endif
            throw std::runtime_error("Bind socket failed");
        }

        if (listen(mListenSocket, SOMAXCONN) == SOCKET_ERROR) {
            freeaddrinfo(res);
#ifdef __WIN32__
            closesocket(mListenSocket);
#else
            close(mListenSocket);
#endif
            throw std::runtime_error("Listen failed");
        }
    }
    else {
        // Client socket
        // Connect to server.
        while (!mTerminate) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Attempting to connect to server (id: %d, ip: %s, type: %s)\n",
                mId, getAddress().c_str(), getTypeStr(getType()).c_str()
            );

            mSocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
            if (mSocket == INVALID_SOCKET) {
                freeaddrinfo(res);
                throw std::runtime_error("Failed to init client socket");
            }

            setOptions(&mSocket);

            int connectRes = connect(
                mSocket,
                res->ai_addr,
                static_cast<int>(res->ai_addrlen)
            );
            if (connectRes != SOCKET_ERROR) {
                break;
            }

            MessageHandler::instance()->print(
                MessageHandler::Level::Debug,
                "Connect error code: %d\n", SGCT_ERRNO
            );

            std::this_thread::sleep_for(std::chrono::seconds(1)); // wait for next attempt
        }
    }

    freeaddrinfo(res);
    mMainThread = std::make_unique<std::thread>([this]() { connectionHandler(); });
}

void Network::connectionHandler() {
    if (mServer) {
        while (!isTerminated()) {
            if (!mConnected) {
                // first time the thread is NULL so the wait will not run
                if (mCommThread) {
                    mCommThread->join();
                    mCommThread = nullptr;
                }

                // start a new connection enabling the client to reconnect
                mCommThread = std::make_unique<std::thread>(
                    [this]() { communicationHandler(); }
                );
            }

            //wait for signal until next iteration in loop
            if (!isTerminated()) {
#ifdef __SGCT_MUTEX_DEBUG__
                    fprintf(stderr, "Locking mutex for connection %d\n", mId);
#endif

                std::unique_lock lk(mConnectionMutex);
                mStartConnectionCond.wait(lk);
#ifdef __SGCT_MUTEX_DEBUG__
                    fprintf(stderr, "Mutex for connection %d is unlocked\n", mId);
#endif
            }
        }
    }
    else {
        // if client
        mCommThread = std::make_unique<std::thread>([this]() { communicationHandler(); });
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Exiting connection handler for connection %d\n", mId
    );
}

const std::string& Network::getPort() const {
    return mPort;
}

const std::string& Network::getAddress() const {
    return mAddress;
}

std::string Network::getTypeStr(ConnectionType ct) {
    switch (ct) {
        case ConnectionType::SyncConnection:
        default:
            return "sync";
        case ConnectionType::ExternalASCIIConnection:
            return "external ASCII control";
        case ConnectionType::ExternalRawConnection:
            return "external binary control";
        case ConnectionType::DataTransfer:
            return "data transfer";
    }
}

void Network::setOptions(SGCT_SOCKET* socketPtr) {
    if (socketPtr == nullptr) {
        return;
    }
    int flag = 1;

    if (!(getType() == ConnectionType::DataTransfer && mUseNaglesAlgorithmInTransfer)) {
        // intset no delay, disable nagle's algorithm
        int iResult = setsockopt(
            *socketPtr,    // socket affected
            IPPROTO_TCP,   // set option at TCP level
            TCP_NODELAY,   // name of option
            reinterpret_cast<char*>(&flag), // the cast is historical cruft
            sizeof(int)    // length of option value
        );

        if (iResult != NO_ERROR) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Network: Failed to set no delay with error: %d\n"
                "This will reduce cluster performance", SGCT_ERRNO
            );
        }
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Network: Enabling Nagle's Algorithm for connection %d\n", mId
        );
    }

    // set timeout
    int timeout = 0; //infinite
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "Network: Failed to set reuse address with error: %d\n", SGCT_ERRNO
        );
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
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Network: Failed to set send buffer size to %d with error: %d\n",
                bufferSize, SGCT_ERRNO
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
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Network: Failed to set receive buffer size to %d with error: %d\n",
                bufferSize, SGCT_ERRNO
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
            MessageHandler::instance()->print(
                MessageHandler::Level::Warning,
                "Network: Failed to set keep alive with error: %d\n", SGCT_ERRNO
            );
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

#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Locking mutex for connection %d\n", mId);
#endif
    std::unique_lock lock(mConnectionMutex);

#ifdef __WIN32__
    shutdown(lSocket, SD_BOTH);
    closesocket(lSocket);
#else
    shutdown(lSocket, SHUT_RDWR);
    close(lSocket);
#endif

    lSocket = INVALID_SOCKET;
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Mutex for connection %d is unlocked\n", mId);
#endif
}

void Network::setBufferSize(uint32_t newSize) {
    mRequestedSize = newSize;
}

int Network::iterateFrameCounter() {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::Info,
        "Network::iterateFrameCounter\n"
    );
#endif

    mSendFramePrevious.store(mSendFrameCurrent.load());

    if (mSendFrameCurrent < MAX_NET_SYNC_FRAME_NUMBER) {
        mSendFrameCurrent++;
    }
    else {
        mSendFrameCurrent = 0;
    }

    mUpdated = false;


#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Locking mutex for connection %d\n", mId);
#endif
    {
        std::unique_lock lock(mConnectionMutex);
        mTimeStampSend = Engine::getTime();
    }
#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Mutex for connection %d is unlocked\n", mId);
#endif

    return mSendFrameCurrent;
}

void Network::pushClientMessage() {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::Info, "Network::pushClientMessage\n"
    );
#endif

    // The servers' render function is locked until a message starting with the ack-byte
    // is received.
    int currentFrame = iterateFrameCounter();
    unsigned char* p = reinterpret_cast<unsigned char*>(&currentFrame);

    if (MessageHandler::instance()->getDataSize() > HeaderSize) {
        MutexManager::instance()->mDataSyncMutex.lock();

        // abock (2019-08-26):  Why is this using the buffer from the MessageHandler even
        // though it has nothing to do with the messaging?

        // Don't remove this pointer, somehow the send function doesn't
        // work during the first call without setting the pointer first!!!
        char* messageToSend = MessageHandler::instance()->getMessage();
        messageToSend[0] = Network::DataId;
        messageToSend[1] = p[0];
        messageToSend[2] = p[1];
        messageToSend[3] = p[2];
        messageToSend[4] = p[3];

        // crop if needed
        uint32_t size = mBufferSize;
        uint32_t messageSize = std::min(
            static_cast<uint32_t>(MessageHandler::instance()->getDataSize()),
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

        MutexManager::instance()->mDataSyncMutex.unlock();

        MessageHandler::instance()->clearBuffer();
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
    mUseNaglesAlgorithmInTransfer = true;
}

int Network::getSendFrameCurrent() const {
    return mSendFrameCurrent;
}

int Network::getSendFramePrevious() const {
    return mSendFramePrevious;
}

int Network::getRecvFrameCurrent() const {
    return mRecvFrameCurrent;
}

int Network::getRecvFramePrevious() const {
    return mRecvFramePrevious;
}

double Network::getLoopTime() const {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Info,
        "Network::getLoopTime\n"
    );
#endif
    std::unique_lock lock(mConnectionMutex);
    return mTimeStampTotal;
}

bool Network::isUpdated() const {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::Info,
        "Network::isUpdated\n"
    );
#endif

    bool state = false;
    if (mServer) {
        state = ClusterManager::instance()->getFirmFrameLockSyncStatus() ?
            // master sends first -> so on reply they should be equal
            (mRecvFrameCurrent == mSendFrameCurrent) :
            // don't check if loose sync
            true;
    }
    else {
        state = ClusterManager::instance()->getFirmFrameLockSyncStatus() ?
            // slaves receive first and then send so the prev should be equal to the send
            (mRecvFramePrevious == mSendFrameCurrent) :
            // if loose sync just check if updated
            mUpdated.load();
    }

    return (state && mConnected);
}

void Network::setDecodeFunction(std::function<void(const char*, int, int)> fn) {
    mDecoderCallbackFn = std::move(fn);
}

void Network::setPackageDecodeFunction(std::function<void(void*, int, int, int)> fn) {
    mPackageDecoderCallbackFn = std::move(fn);
}

void Network::setUpdateFunction(std::function<void(Network*)> fn) {
    mUpdateCallbackFn = std::move(fn);
}

void Network::setConnectedFunction(std::function<void(void)> fn) {
    mConnectedCallbackFn = std::move(fn);
}

void Network::setAcknowledgeFunction(std::function<void(int, int)> fn) {
    mAcknowledgeCallbackFn = std::move(fn);
}

void Network::setConnectedStatus(bool state) {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::Info,
        "Network::setConnectedStatus = %s at syncframe %d\n",
        state ? "true" : "false", getSendFrame()
    );
#endif

    std::unique_lock lock(mConnectionMutex);
    mConnected = state;
}

bool Network::isConnected() const {
    return mConnected;
}

Network::ConnectionType Network::getType() const {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::Info,
        "Network::getTypeOfServer\n"
    );
#endif

    std::unique_lock lock(mConnectionMutex);
    return mConnectionType;
}

int Network::getId() const {
    return mId;
}

bool Network::isServer() const {
    return mServer;
}

bool Network::isTerminated() const {
    return mTerminate;
}

void Network::setRecvFrame(int i) {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::Info, "Network::setRecvFrame\n"
    );
#endif
    
    mRecvFramePrevious.store(mRecvFrameCurrent.load());
    mRecvFrameCurrent = i;
    mUpdated = true;

    std::unique_lock lock(mConnectionMutex);
    mTimeStampTotal = Engine::getTime() - mTimeStampSend;
}

int Network::getLastError() {
    return SGCT_ERRNO;
}

_ssize_t Network::receiveData(SGCT_SOCKET& lsocket, char* buffer, int length, int flags) {
    _ssize_t iResult = 0;
    int attempts = 1;

    while (iResult < length) {
        _ssize_t tmpRes = recv(lsocket, buffer + iResult, length - iResult, flags);
#ifdef __SGCT_NETWORK_DEBUG__
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Received %d bytes of %d\n", tmpRes, length
        );
        for (int i = 0; i < tmpRes; i++) {
            MessageHandler::instance()->print("%u\t", buffer[i]);
        }
        MessageHandler::instance()->print("\n");
#endif

        if (tmpRes > 0) {
            iResult += tmpRes;
        }
#ifdef __WIN32__
        else if (SGCT_ERRNO == WSAEINTR && attempts <= MaxNumberOfAttempts)
#else
        else if (SGCT_ERRNO == EINTR && attempts <= MaxNumberOfAttempts)
#endif
        {
            MessageHandler::instance()->print(
                MessageHandler::Level::Warning,
                "Receiving data after interrupted system error (attempt %d)\n", attempts
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

    std::unique_lock lock(mConnectionMutex);
    buffer.resize(reqSize);
    currSize = reqSize;
}

int Network::readSyncMessage(char* header, int32_t& syncFrameNumber, uint32_t& dataSize,
                             uint32_t& uncompressedDataSize)
{
    int iResult = receiveData(mSocket, header, static_cast<int>(HeaderSize), 0);

    if (iResult == static_cast<int>(HeaderSize)) {
        mHeaderId = header[0];
#ifdef __SGCT_NETWORK_DEBUG__
        MessageHandler::instance()->printDebug(
            MessageHandler::Info, "Header id=%d\n", mHeaderId
        );
#endif
        if (mHeaderId == DataId || mHeaderId == CompressedDataId) {
            // parse the sync frame number
            syncFrameNumber = parseInt32(&header[1]);
            // parse the data size
            dataSize = parseUInt32(&header[5]);
            // parse the uncompressed size if compression is used
            uncompressedDataSize = parseUInt32(&header[9]);

            setRecvFrame(syncFrameNumber);
            if (syncFrameNumber < 0) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "Network: Error sync in sync frame: %d for connection %d\n",
                    syncFrameNumber, mId
                );
            }

#ifdef __SGCT_NETWORK_DEBUG__
            MessageHandler::instance()->printDebug(
                MessageHandler::Level::Info,
                "Network: Package info: Frame = %d, Size = %u for connection %d\n",
                syncFrameNumber, dataSize, mId
            );
#endif

            // resize buffer if needed
#ifdef __SGCT_MUTEX_DEBUG__
            fprintf(stderr, "Locking mutex for connection %d\n", mId);
#endif
            
            updateBuffer(mRecvBuf, dataSize, mBufferSize);
            updateBuffer(mUncompressBuf, uncompressedDataSize, mUncompressedBufferSize);
            
#ifdef __SGCT_MUTEX_DEBUG__
            fprintf(stderr, "Mutex for connection %d is unlocked\n", mId);
#endif
        }
    }

#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::Info,
        "Receiving data (buffer size: %d)\n", dataSize
    );
#endif

    // Get the data/message
    if (dataSize > 0) {
        iResult = receiveData(mSocket, mRecvBuf.data(), dataSize, 0);
    }

    return iResult;
}

int Network::readDataTransferMessage(char* header, int32_t& packageId, uint32_t& dataSize,
                                     uint32_t& uncompressedDataSize)
{
    int iResult = receiveData(mSocket, header, static_cast<int>(HeaderSize), 0);

    if (iResult == static_cast<int>(HeaderSize)) {
        mHeaderId = header[0];
#ifdef __SGCT_NETWORK_DEBUG__
        MessageHandler::instance()->printDebug(
            MessageHandler::Level::Info, "Header id=%d\n", mHeaderId
        );
#endif
        if (mHeaderId == DataId || mHeaderId == CompressedDataId) {
            // parse the package id
            packageId = parseInt32(&header[1]);
            // parse the data size
            dataSize = parseUInt32(&header[5]);
            // parse the uncompressed size if compression is used
            uncompressedDataSize = parseUInt32(&header[9]);

            // resize buffer if needed
#ifdef __SGCT_MUTEX_DEBUG__
            fprintf(stderr, "Locking mutex for connection %d\n", mId);
#endif
            updateBuffer(mRecvBuf, dataSize, mBufferSize);
            updateBuffer(mUncompressBuf, uncompressedDataSize, mUncompressedBufferSize);

#ifdef __SGCT_MUTEX_DEBUG__
            fprintf(stderr, "Mutex for connection %d is unlocked\n", mId);
#endif
        }
        else if (mHeaderId == Ack && mAcknowledgeCallbackFn != nullptr) {
            //parse the package id
            packageId = parseInt32(&header[1]);
            mAcknowledgeCallbackFn(packageId, mId);
        }
    }

#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::Info,
        "Receiving data (buffer size: %d)\n", dataSize
    );
#endif
    // Get the data/message
    if (dataSize > 0 && packageId > -1) {
        iResult = receiveData(mSocket, mRecvBuf.data(), dataSize, 0);
#ifdef __SGCT_NETWORK_DEBUG__
        MessageHandler::instance()->printDebug(
            MessageHandler::Level::Info,
            "Data type: %d, %d bytes of %u\n", packageId, iResult, dataSize
        );
#endif
    }

    return iResult;
}

int Network::readExternalMessage() {
    // do a normal read
    int iResult = recv(mSocket, mRecvBuf.data(), mBufferSize, 0);

    // if read fails try for x attempts
    int attempts = 1;
#ifdef __WIN32__
    while (iResult <= 0 && SGCT_ERRNO == WSAEINTR && attempts <= MaxNumberOfAttempts)
#else
    while (iResult <= 0 && SGCT_ERRNO == EINTR && attempts <= MaxNumberOfAttempts)
#endif
    {
        iResult = recv(mSocket, mRecvBuf.data(), mBufferSize, 0);

        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Receiving data after interrupted system error (attempt %d)\n", attempts
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
    if (mServer) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "Waiting for client to connect to connection %d (port %s)\n",
            mId, getPort().c_str()
        );

        mSocket = accept(mListenSocket, nullptr, nullptr);

        int accErr = SGCT_ERRNO;
#ifdef __WIN32__
        while (!isTerminated() && mSocket == INVALID_SOCKET && accErr == WSAEINTR)
#else
        while ( !isTerminated() && mSocket == INVALID_SOCKET && accErr == EINTR)
#endif
        {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Re-accept after interrupted system on connection %d\n", mId
            );

            mSocket = accept(mListenSocket, nullptr, nullptr);
        }

        if (mSocket == INVALID_SOCKET) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Accept connection %d failed! Error: %d\n", mId, accErr
            );

            if (mUpdateCallbackFn) {
                mUpdateCallbackFn(this);
            }
            return;
        }
    }

    setConnectedStatus(true);
    MessageHandler::instance()->print(
        MessageHandler::Level::Info, "Connection %d established!\n", mId
    );

    if (mUpdateCallbackFn) {
        mUpdateCallbackFn(this);
    }

    // init buffers
    char RecvHeader[HeaderSize];
    memset(RecvHeader, DefaultId, HeaderSize);

    mConnectionMutex.lock();
    mRecvBuf.resize(mBufferSize);
    mUncompressBuf.resize(mUncompressedBufferSize);
    mConnectionMutex.unlock();
    
    std::string extBuffer; //for external comm

    // Receive data until the server closes the connection
    _ssize_t iResult = 0;
    do {
        // resize buffer request
        if (getType() != ConnectionType::DataTransfer && mRequestedSize > mBufferSize) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Info,
                "Re-sizing tcp buffer size from %d to %d",
                mBufferSize, mRequestedSize.load()
            );

            updateBuffer(mRecvBuf, mRequestedSize, mBufferSize);

            MessageHandler::instance()->print(MessageHandler::Level::Info, "Done.\n");
        }
#ifdef __SGCT_NETWORK_DEBUG__
        MessageHandler::instance()->printDebug(
            MessageHandler::Level::NotifyAll,
            "Receiving message header\n"
        );
#endif
        
        int32_t packageId = -1;
        int32_t syncFrameNumber = -1;
        uint32_t dataSize = 0;
        uint32_t uncompressedDataSize = 0;
        
        mHeaderId = DefaultId;

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
                    if (!mServer) {
                        mTerminate = true;
                    }

                    MessageHandler::instance()->print(
                        MessageHandler::Level::Info,
                        "Network: Client %d terminated connection\n", mId
                    );

                    break;
                }
                else {
                    // handle sync communication
                    if (mHeaderId == DataId && mDecoderCallbackFn != nullptr) {
                        // decode callback
                        if (dataSize > 0) {
                            mDecoderCallbackFn(mRecvBuf.data(), dataSize, mId);
                        }

                        NetworkManager::gCond.notify_all();
                    }
                    else if (mHeaderId == CompressedDataId && mDecoderCallbackFn) {
                        //decode callback
                        if (dataSize > 0) {
                            // parse the package id
                            uLongf uncompressedSize = static_cast<uLongf>(
                                uncompressedDataSize
                            );
    
                            int err = uncompress(
                                reinterpret_cast<Bytef*>(mUncompressBuf.data()),
                                &uncompressedSize,
                                reinterpret_cast<Bytef*>(mRecvBuf.data()),
                                static_cast<uLongf>(dataSize)
                            );
                            
                            if (err == Z_OK) {
                                // decode callback
                                mDecoderCallbackFn(
                                    mUncompressBuf.data(),
                                    static_cast<int>(uncompressedSize),
                                    mId
                                );
                            }
                            else {
                                MessageHandler::instance()->print(
                                    MessageHandler::Level::Error,
                                    "Network: Failed to uncompress data for connection "
                                    "%d. Error: %s\n",
                                    mId, getUncompressionErrorAsStr(err).c_str()
                                );
                            }
                        }
                        
                        NetworkManager::gCond.notify_all();
                    }
                    else if (mHeaderId == ConnectedId && mConnectedCallbackFn) {
#ifdef __SGCT_NETWORK_DEBUG__
                        MessageHandler::instance()->printDebug(
                            MessageHandler::Level::Info,
                            "Signaling slave is connected... "
                        );
#endif
                        mConnectedCallbackFn();
                        NetworkManager::gCond.notify_all();
#ifdef __SGCT_NETWORK_DEBUG__
                        MessageHandler::instance()->printDebug(
                            MessageHandler::Level::Info, "Done.\n"
                        );
#endif
                    }
                }
            }
            // handle external ascii communication
            else if (getType() == ConnectionType::ExternalASCIIConnection) {
#ifdef __SGCT_NETWORK_DEBUG__
                MessageHandler::instance()->printDebug(
                    MessageHandler::Level::Info,
                    "Parsing external TCP ASCII data... "
                );
#endif
                extBuffer += std::string(mRecvBuf.data()).substr(0, iResult);

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

                    if (mDecoderCallbackFn) {
                        mDecoderCallbackFn(
                            extMessage.c_str(),
                            static_cast<int>(extMessage.size()),
                            mId
                        );
                    }

                    // reply
                    std::string msg = "OK\r\n";
                    sendData(msg.c_str(), static_cast<int>(msg.size()));
                    found = extBuffer.find("\r\n");
                }
#ifdef __SGCT_NETWORK_DEBUG__
                MessageHandler::instance()->printDebug(
                    MessageHandler::Level::Info,
                    "Done\n"
                );
#endif
            }
            // handle external raw/binary communication
            else if (getType() == ConnectionType::ExternalRawConnection) {
#ifdef __SGCT_NETWORK_DEBUG__
                MessageHandler::instance()->printDebug(
                    MessageHandler::Level::Info,
                    "Parsing external TCP raw data... "
                );
#endif
                if (mDecoderCallbackFn) {
                    mDecoderCallbackFn(mRecvBuf.data(), iResult, mId);
                }

#ifdef __SGCT_NETWORK_DEBUG__
                MessageHandler::instance()->printDebug(
                    MessageHandler::Level::Info,
                    "Done\n"
                );
#endif
            }
            // handle data transfer communication
            else if (getType() == ConnectionType::DataTransfer) {
                // Disconnect if requested
                if (parseDisconnectPackage(RecvHeader)) {
                    setConnectedStatus(false);
                    MessageHandler::instance()->print(
                        MessageHandler::Level::Info,
                        "Network: File transfer %d terminated connection\n", mId
                    );
                }
                //  Handle communication
                else {
                    if ((mHeaderId == DataId || mHeaderId == CompressedDataId) &&
                        mPackageDecoderCallbackFn && dataSize > 0)
                    {
                        bool recvOk = false;
                        
                        if (mHeaderId == DataId) {
                            // uncompressed
                            // decode callback
                            mPackageDecoderCallbackFn(
                                mRecvBuf.data(),
                                dataSize,
                                packageId,
                                mId
                            );
                            recvOk = true;
                        }
                        else {
                            // compressed
                            // parse the package id
                            uLongf uncompressedSize = static_cast<uLongf>(
                                uncompressedDataSize
                            );
                            
                            int err = uncompress(
                                reinterpret_cast<Bytef*>(mUncompressBuf.data()),
                                &uncompressedSize,
                                reinterpret_cast<Bytef*>(mRecvBuf.data()),
                                static_cast<uLongf>(dataSize)
                            );
                            
                            if (err == Z_OK) {
                                // decode callback
                                mPackageDecoderCallbackFn(
                                    mUncompressBuf.data(),
                                    static_cast<int>(uncompressedSize),
                                    packageId,
                                    mId
                                );
                                recvOk = true;
                            }
                            else {
                                MessageHandler::instance()->print(
                                    MessageHandler::Level::Error,
                                    "Network: Failed to uncompress data for connection "
                                    "%d! Error: %s\n",
                                    mId, getUncompressionErrorAsStr(err).c_str()
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
                        mConnectionMutex.lock();

                        // clean up
                        mRecvBuf.clear();
                        mUncompressBuf.clear();
                        
                        mBufferSize = 0;
                        mUncompressedBufferSize = 0;
                        mConnectionMutex.unlock();
                    }
                    else if (mHeaderId == ConnectedId && mConnectedCallbackFn) {
#ifdef __SGCT_NETWORK_DEBUG__
                        MessageHandler::instance()->printDebug(
                            MessageHandler::Level::Info,
                            "Signaling slave is connected... "
                        );
#endif
                        mConnectedCallbackFn();
                        NetworkManager::gCond.notify_all();
                        
#ifdef __SGCT_NETWORK_DEBUG__
                        MessageHandler::instance()->printDebug(
                            MessageHandler::Level::Info,
                            "Done.\n"
                        );
#endif
                    }
                }
            }
        }
        // handle failed receive
        else if (iResult == 0) {
#ifdef __SGCT_NETWORK_DEBUG__
            MessageHandler::instance()->printDebug(
                MessageHandler::Level::Info, "Setting connection status to false..."
            );
#endif
            setConnectedStatus(false);
#ifdef __SGCT_NETWORK_DEBUG__
            MessageHandler::instance()->printDebug(
                MessageHandler::Level::Info, "Done.\n"
            );
#endif

            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "TCP Connection %d closed (error: %d)\n", mId, SGCT_ERRNO
            );
        }
        else {
        // if negative
#ifdef __SGCT_NETWORK_DEBUG__
            MessageHandler::instance()->printDebug(
                MessageHandler::Level::Info,
                "Setting connection status to false... "
            );
#endif
            setConnectedStatus(false);
#ifdef __SGCT_NETWORK_DEBUG__
            MessageHandler::instance()->printDebug(
                MessageHandler::Level::Info,
                "Done.\n"
            );
#endif

            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "TCP connection %d recv failed: %d\n", mId, SGCT_ERRNO
            );
        }
    } while (iResult > 0 || mConnected);

    // cleanup
    mRecvBuf.clear();
    mUncompressBuf.clear();

    // Close socket; contains mutex
    closeSocket(mSocket);

    if (mUpdateCallbackFn) {
        mUpdateCallbackFn(this);
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Node %d disconnected\n", mId
    );
}

void Network::sendData(const void* data, int length) {
#ifdef __SGCT_NETWORK_DEBUG__
    for (int i = 0; i < length; i++) {
        fprintf(stderr, "%u ", (reinterpret_cast<const char*>(data)[i]));
    }
    fprintf(stderr, "\n");
#endif
    _ssize_t sentLen;
    int sendSize = length;

    while (sendSize > 0) {
        int offset = length - sendSize;
        sentLen = send(
            mSocket,
            reinterpret_cast<const char*>(data) + offset,
            sendSize,
            0
        );
        if (sentLen == SOCKET_ERROR) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "Send data failed\n"
            );
            break;
        }
        else {
            sendSize -= sentLen;
        }
    }
}

void Network::closeNetwork(bool forced) {
    mDecoderCallbackFn = nullptr;
    mUpdateCallbackFn = nullptr;
    mConnectedCallbackFn = nullptr;
    mAcknowledgeCallbackFn = nullptr;
    mPackageDecoderCallbackFn = nullptr;

    // release conditions
    NetworkManager::gCond.notify_all();
    mStartConnectionCond.notify_all();

    // blocking sockets -> cannot wait for thread so just kill it brutally

    if (mCommThread) {
        if (!forced) {
            mCommThread->join();
        }
        mCommThread = nullptr;
    }

    if (mMainThread) {
        if (!forced) {
            mMainThread->join();
        }
        mMainThread = nullptr;
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Connection %d successfully terminated\n", mId
    );
}

void Network::initShutdown() {
    if (mConnected) {
        char gameOver[9];
        gameOver[0] = DisconnectId;
        gameOver[1] = 24; //ASCII for cancel
        gameOver[2] = '\r';
        gameOver[3] = '\n';
        gameOver[4] = 27; //ASCII for Esc
        gameOver[5] = '\r';
        gameOver[6] = '\n';
        gameOver[7] = '\0';
        gameOver[8] = DefaultId;
        sendData(gameOver, HeaderSize);
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "Closing connection %d\n", mId
    );

#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Locking mutex for connection %d\n", mId);
#endif

    {
        std::unique_lock lock(mConnectionMutex);
        mDecoderCallbackFn = nullptr;
    }

#ifdef __SGCT_MUTEX_DEBUG__
    fprintf(stderr, "Mutex for connection %d is unlocked\n", mId);
#endif

    mConnected = false;
    mTerminate = true;

    // wake up the connection handler thread (in order to finish)
    if (mServer) {
        mStartConnectionCond.notify_all();
    }

    closeSocket(mSocket);
    closeSocket(mListenSocket);
}

} // namespace sgct::core
