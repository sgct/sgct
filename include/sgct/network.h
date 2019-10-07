/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__NETWORK__H__
#define __SGCT__NETWORK__H__

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(WIN32)
    #define _WIN_PLATFORM
    using SGCT_SOCKET = size_t;
#else //linux & OS X
    using SGCT_SOCKET = int;
#endif

using _ssize_t = int;

namespace sgct::core {

/**
 * Network manages peer-to-peer tcp connections.
 */
class Network {
public:
    // ASCII device control chars = 17, 18, 19 & 20
    static constexpr const char DefaultId = 0;
    static constexpr const char Ack = 6;
    static constexpr const char DataId = 17;
    static constexpr const char ConnectedId = 18;
    static constexpr const char DisconnectId = 19;
    static constexpr const char CompressedDataId = 21;

    enum class ConnectionType {
        SyncConnection = 0,
        ExternalASCIIConnection,
        ExternalRawConnection,
        DataTransfer
    };

    static const size_t HeaderSize = 13;

    Network();
    ~Network();

    /**
     * Inits this network connection.
     *
     * \param port is the network port (TCP)
     * \param address is the hostname, IPv4 address or ip6 address
     * \param isServer indicates if this connection is a server or client
     * \param id is a unique id of this connection
     * \param connectionType is the type of connection
     * \param firmSync if set to true then firm framesync will be used for the whole
                       cluster
     */
    void init(int port, std::string address, bool isServer, ConnectionType serverType);
    void closeNetwork(bool forced);
    void initShutdown();

    void setDecodeFunction(std::function<void(const char*, int, int)> callback);
    void setPackageDecodeFunction(std::function<void(void*, int, int, int)> callback);
    void setUpdateFunction(std::function<void(Network *)> callback);
    void setConnectedFunction(std::function<void (void)> callback);
    void setAcknowledgeFunction(std::function<void(int, int)> callback);
    
    void setBufferSize(uint32_t newSize);
    void setConnectedStatus(bool state);
    void setOptions(SGCT_SOCKET* socketPtr);
    void closeSocket(SGCT_SOCKET lSocket);

    ConnectionType getType() const;
    int getId() const;
    bool isServer() const;
    bool isConnected() const;

    bool isTerminated() const;
    int getSendFrameCurrent() const;
    int getSendFramePrevious() const;
    int getRecvFrameCurrent() const;
    int getRecvFramePrevious() const;

    /// Get the time in seconds from send to receive of sync data.
    double getLoopTime() const;

    /**
     * This function compares the received frame number with the sent frame number.
     *
     * The server starts by sending a frame sync number to the client. The client receives
     * the sync frame number and sends it back after drawing when ready for buffer swap.
     * When the server recieves a frame sync number equal to the send frame number it
     * swaps buffers.
     *
     * \returns true if updates has been received
     */
    bool isUpdated() const;
    void setRecvFrame(int i);
    void sendData(const void* data, int length);

    /// @return last error code 
    static int getLastError();
    static _ssize_t receiveData(SGCT_SOCKET& lsocket, char* buffer, int length, int flags);

    /// Iterates the send frame number and returns the new frame number
    int iterateFrameCounter();

    /// The client sends ack message to server + console messages
    void pushClientMessage();
    void enableNaglesAlgorithmInDataTransfer();

    /// \return the port of this connection
    int getPort() const;

    /// \return the address of this connection
    const std::string& getAddress() const;

    /// \return the connection type as string
    static std::string getTypeStr(ConnectionType ct);

    std::function<void(const char*, int, int)> decoderCallback;
    std::function<void(void*, int, int, int)> _packageDecoderCallback;
    std::function<void(Network*)> _updateCallback;
    std::function<void(void)> _connectedCallback;
    std::function<void(int, int)> _acknowledgeCallback;

    std::condition_variable _startConnectionCond;

private:
    void updateBuffer(std::vector<char>& buffer, uint32_t reqSize, uint32_t& currSize);
    int readSyncMessage(char* header, int32_t& syncFrameNumber, uint32_t& dataSize,
        uint32_t& uncompressedDataSize);
    int readDataTransferMessage(char* header, int32_t& packageId, uint32_t& dataSize,
        uint32_t& uncompressedDataSize);
    int readExternalMessage();

    /// function to decode messages
    void communicationHandler();
    void connectionHandler();

    SGCT_SOCKET _socket;
    SGCT_SOCKET _listenSocket;

    ConnectionType _connectionType = ConnectionType::SyncConnection;
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
    double _timeStampTotal = 0.0;
    int _id;
    uint32_t _bufferSize = 1024;
    uint32_t _uncompressedBufferSize = _bufferSize;
    std::atomic<uint32_t> _requestedSize = _bufferSize;
    int _port;
    std::string _address;

    std::vector<char> _recvBuffer;
    std::vector<char> _uncompressBuffer;
    char _headerId;

    bool _useNaglesAlgorithmInTransfer = false;
};

} // namespace sgct::core

#endif // __SGCT__NETWORK__H__
