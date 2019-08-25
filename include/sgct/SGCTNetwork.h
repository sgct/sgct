/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__NETWORK__H__
#define __SGCT__NETWORK__H__

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#if defined(__WIN32__) || defined(__MINGW32__) || defined(__MINGW64__)
    #define _WIN_PLATFORM
    using SGCT_SOCKET = size_t;
#else //linux & OS X
    using SGCT_SOCKET = int;
#endif

using _ssize_t = int;

namespace sgct_core {

/**
 * SGCTNetwork manages peer-to-peer tcp connections.
 */
class SGCTNetwork {
public:
    // ASCII device control chars = 17, 18, 19 & 20
    enum PackageHeaderId {
        DefaultId = 0,
        Ack = 6,
        DataId = 17,
        ConnectedId = 18,
        DisconnectId = 19,
        CompressedDataId = 21
    };

    enum class ConnectionTypes {
        SyncConnection = 0,
        ExternalASCIIConnection,
        ExternalRawConnection,
        DataTransfer
    };

    static const size_t HeaderSize = 13;

    SGCTNetwork();

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
    void init(std::string port, std::string address, bool isServer,
        ConnectionTypes serverType);
    void closeNetwork(bool forced);
    void initShutdown();

    void setDecodeFunction(std::function<void(const char*, int, int)> callback);
    void setPackageDecodeFunction(std::function<void(void*, int, int, int)> callback);
    void setUpdateFunction(std::function<void(SGCTNetwork *)> callback);
    void setConnectedFunction(std::function<void (void)> callback);
    void setAcknowledgeFunction(std::function<void(int, int)> callback);
    
    void setBufferSize(uint32_t newSize);
    void setConnectedStatus(bool state);
    void setOptions(SGCT_SOCKET* socketPtr);
    void closeSocket(SGCT_SOCKET lSocket);

    ConnectionTypes getType() const;
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
    const std::string& getPort() const;

    /// \return the address of this connection
    const std::string& getAddress() const;

    /// \return the connection type as string
    static std::string getTypeStr(ConnectionTypes ct);

    std::function<void(const char*, int, int)> mDecoderCallbackFn;
    std::function<void(void*, int, int, int)> mPackageDecoderCallbackFn;
    std::function<void(SGCTNetwork *)> mUpdateCallbackFn;
    std::function<void(void)> mConnectedCallbackFn;
    std::function<void(int, int)> mAcknowledgeCallbackFn;

    std::condition_variable mStartConnectionCond;

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

    SGCT_SOCKET mSocket;
    SGCT_SOCKET mListenSocket;

    ConnectionTypes mConnectionType = ConnectionTypes::SyncConnection;
    std::atomic_bool mServer;
    std::atomic_bool mConnected = false;
    std::atomic_bool mUpdated = false;
    std::atomic<int32_t> mSendFrameCurrent = 0;
    std::atomic<int32_t> mSendFramePrevious = 0;
    std::atomic<int32_t> mRecvFrameCurrent = 0;
    std::atomic<int32_t> mRecvFramePrevious = -1;
    std::atomic_bool mTerminate = false; //set to true upon exit

    mutable std::mutex mConnectionMutex;
    std::unique_ptr<std::thread> mCommThread;
    std::unique_ptr<std::thread> mMainThread;

    double mTimeStampSend = 0.0;
    double mTimeStampTotal = 0.0;
    int mId;
    uint32_t mBufferSize = 1024;
    uint32_t mUncompressedBufferSize = mBufferSize;
    std::atomic<uint32_t> mRequestedSize = mBufferSize;
    std::string mPort;
    std::string mAddress;

    std::vector<char> mRecvBuf;
    std::vector<char> mUncompressBuf;
    char mHeaderId;

    bool mUseNaglesAlgorithmInTransfer = false;
};

} // namespace sgct_core

#endif // __SGCT__NETWORK__H__
