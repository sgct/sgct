/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__NETWORK__H__
#define __SGCT__NETWORK__H__

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define MAX_NET_SYNC_FRAME_NUMBER 10000

#if defined(__WIN32__) || defined(__MINGW32__) || defined(__MINGW64__)
    #define _WIN_PLATFORM
    using SGCT_SOCKET = size_t;
#else //linux & OS X
    using SGCT_SOCKET = int;
#endif

using _ssize_t = int;

namespace sgct_core {

/*!
SGCTNetwork manages peer-to-peer tcp connections.
*/
class SGCTNetwork {
public:
    //ASCII device control chars = 17, 18, 19 & 20
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

    enum ReceivedIndex { Current = 0, Previous };

    SGCTNetwork();
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
    int getSendFrame(ReceivedIndex ri = Current) const;
    int getRecvFrame(ReceivedIndex ri) const;
    double getLoopTime();
    bool isUpdated() const;
    void setRecvFrame(int i);
    void sendData(const void* data, int length);
    void sendStr(const std::string& msg);
    static int getLastError();
    static _ssize_t receiveData(SGCT_SOCKET& lsocket, char* buffer, int length, int flags);
    static int32_t parseInt32(char* str);
    static uint32_t parseUInt32(char* str);
    int iterateFrameCounter();
    void pushClientMessage();
    void enableNaglesAlgorithmInDataTransfer();
    std::string getPort() const;
    std::string getAddress() const;
    std::string getTypeStr() const;
    static std::string getTypeStr(ConnectionTypes ct);

    std::function<void(const char*, int, int)> mDecoderCallbackFn;
    std::function<void(void*, int, int, int)> mPackageDecoderCallbackFn;
    std::function<void(SGCTNetwork *)> mUpdateCallbackFn;
    std::function<void(void)> mConnectedCallbackFn;
    std::function<void(int, int)> mAcknowledgeCallbackFn;

private:
    void updateBuffer(char** buffer, uint32_t requested_size, uint32_t& current_size);
    int readSyncMessage(char* header, int32_t& syncFrameNumber, uint32_t& dataSize,
        uint32_t& uncompressedDataSize);
    int readDataTransferMessage(char* header, int32_t& packageId, uint32_t& dataSize,
        uint32_t& uncompressedDataSize);
    int readExternalMessage();

    static void communicationHandlerStarter(void* arg);
    static void connectionHandlerStarter(void* arg);
    void communicationHandler();
    void connectionHandler();
    static bool parseDisconnectPackage(char* headerPtr);
    static std::string getUncompressionErrorAsStr(int err);

public:
    static const size_t mHeaderSize = 13;
    std::condition_variable mStartConnectionCond;

private:
    enum TimeStampIndex { Send = 0, Total };

    SGCT_SOCKET mSocket;
    SGCT_SOCKET mListenSocket;

    ConnectionTypes mConnectionType = ConnectionTypes::SyncConnection;
    std::atomic<bool> mServer;
    std::atomic<bool> mConnected = false;
    std::atomic<bool> mUpdated = false;
    std::atomic<int32_t> mSendFrame[2] = { 0, 0 };
    std::atomic<int32_t> mRecvFrame[2] = { 0, -1 };
    std::atomic<bool> mTerminate = false; //set to true upon exit

    mutable std::mutex mConnectionMutex;
    std::thread* mCommThread = nullptr;
    std::thread* mMainThread = nullptr;

    double mTimeStamp[2] = { 0.0, 0.0 };
    int mId;
    uint32_t mBufferSize = 1024;
    uint32_t mUncompressedBufferSize = mBufferSize;
    std::atomic<uint32_t> mRequestedSize = mBufferSize;
    std::string mPort;
    std::string mAddress;

    char* mRecvBuf = nullptr;
    char* mUncompressBuf = nullptr;
    char mHeaderId;

    bool mUseNaglesAlgorithmInDataTransfer = false;
};

} // namespace sgct_core

#endif // __SGCT__NETWORK__H__
