/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__NETWORK__H__
#define __SGCT__NETWORK__H__

#include <sgct/sgctexports.h>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef WIN32
    using SGCT_SOCKET = size_t;
#else // linux & OS X
    using SGCT_SOCKET = int;
#endif

namespace sgct {

/**
 * Network manages peer-to-peer tcp connections.
 */
class SGCT_EXPORT Network {
public:
    // ASCII device control chars = 17, 18, 19 & 20
    static constexpr char DefaultId = 0;
    static constexpr char Ack = 6;
    static constexpr char DataId = 17;
    static constexpr char ConnectedId = 18;
    static constexpr char DisconnectId = 19;

    enum class ConnectionType { SyncConnection, DataTransfer };

    static constexpr size_t HeaderSize = 13;

    /**
     * \param port The network port (TCP)
     * \param address The hostname, IPv4 address or ip6 address
     * \param isServer Indicates if this connection is a server or client
     * \param type The type of connection
     */
    Network(int port, std::string address, bool isServer, ConnectionType type);
    Network(const Network&) = delete;
    Network(Network&&) = delete;
    Network& operator=(const Network&) = delete;
    Network& operator=(Network&&) = delete;
    ~Network();

    void initialize();
    void closeNetwork(bool forced);
    void initShutdown();

    void setDecodeFunction(std::function<void(const char*, int)> fn);
    void setPackageDecodeFunction(std::function<void(void*, int, int, int)> fn);
    void setUpdateFunction(std::function<void(Network*)> fn);
    void setConnectedFunction(std::function<void (void)> fn);
    void setAcknowledgeFunction(std::function<void(int, int)> fn);

    void setConnectedStatus(bool state);
    void setOptions(SGCT_SOCKET* socketPtr);
    void closeSocket(SGCT_SOCKET lSocket);

    ConnectionType type() const;
    int id() const;
    bool isServer() const;
    bool isConnected() const;

    int sendFrameCurrent() const;
    int sendFramePrevious() const;
    int recvFrameCurrent() const;
    int recvFramePrevious() const;

    /**
     * Get the time in seconds from send to receive of sync data.
     */
    double loopTime() const;

    /**
     * This function compares the received frame number with the sent frame number. The
     * server starts by sending a frame sync number to the client. The client receives the
     * sync frame number and sends it back after drawing when ready for buffer swap. When
     * the server gets a frame sync number equal to the sent number it swaps buffers.
     *
     * \return `true` if updates has been received
     */
    bool isUpdated() const;
    void sendData(const void* data, int length);

    /**
     * \return The last error code
     */
    static int lastError();
    static int receiveData(SGCT_SOCKET& lsocket, char* buffer, int length, int flags);

    /**
     * Iterates the send frame number and returns the new frame number.
     */
    int iterateFrameCounter();

    /**
     * The client sends ack message to server + console messages.
     */
    void pushClientMessage();

    /**
     * \return The port of this connection
     */
    int port() const;

    std::condition_variable& startConnectionConditionVar();

private:
    void setRecvFrame(int i);
    void updateBuffer(std::vector<char>& buffer, uint32_t reqSize, uint32_t& currSize);
    int readSyncMessage(char* header, int32_t& syncFrame, uint32_t& dataSize,
        uint32_t& uncompressedDataSize);
    int readDataTransferMessage(char* header, int32_t& packageId, uint32_t& dataSize,
        uint32_t& uncompressedDataSize);
    int readExternalMessage();

    /// function to decode messages
    void communicationHandler();
    void connectionHandler();

    SGCT_SOCKET _socket;
    SGCT_SOCKET _listenSocket;

    const ConnectionType _connectionType = ConnectionType::SyncConnection;
    std::atomic_bool _isServer;
    std::atomic_bool _isConnected = false;
    std::atomic_bool _isUpdated = false;
    std::atomic<int32_t> _currentSendFrame = 0;
    std::atomic<int32_t> _previousSendFrame = 0;
    std::atomic<int32_t> _currentRecvFrame = 0;
    std::atomic<int32_t> _previousRecvFrame = -1;
    std::atomic_bool _shouldTerminate = false; // set to true upon exit

    mutable std::mutex _connectionMutex;
    std::unique_ptr<std::thread> _commThread;
    std::unique_ptr<std::thread> _mainThread;

    double _timeStampSend = 0.0;
    std::atomic<double> _timeStampTotal = 0.0;
    int _id;
    uint32_t _bufferSize = 1024;
    uint32_t _uncompressedBufferSize = _bufferSize;
    std::atomic<uint32_t> _requestedSize = _bufferSize;
    const int _port = -1;

    std::vector<char> _recvBuffer;
    std::vector<char> _uncompressBuffer;
    char _headerId = 0;

    std::condition_variable _startConnectionCond;

    std::function<void(const char*, int)> decoderCallback;
    std::function<void(void*, int, int, int)> _packageDecoderCallback;
    std::function<void(Network*)> _updateCallback;
    std::function<void(void)> _connectedCallback;
    std::function<void(int, int)> _acknowledgeCallback;
};

} // namespace sgct

#endif // __SGCT__NETWORK__H__
