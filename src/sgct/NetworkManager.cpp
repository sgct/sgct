/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/NetworkManager.h>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <sgct/ClusterManager.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/SharedData.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/Statistics.h>
#include <algorithm>

#ifndef SGCT_DONT_USE_EXTERNAL
#include "../include/external/zlib.h"
#else
#include <zlib.h>
#endif

#if defined(_WIN_PLATFORM)//WinSock
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
    struct sockaddr_in srcaddr;

    memset(&srcaddr, 0, sizeof(sockaddr_in));
    memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));

    srcaddr.sin_family = af;
    if (WSAAddressToString((sockaddr*) &srcaddr, sizeof(sockaddr_in), 0, dst, (LPDWORD) &cnt) != 0) {
        DWORD rv = WSAGetLastError();
        printf("WSAAddressToString() : %d\n",rv);
        return NULL;
    }
    return dst;
}
#endif

//#define __SGCT_NETWORK_DEBUG__

namespace sgct_core {

NetworkManager* NetworkManager::instance() {
    return mInstance;
}

std::condition_variable NetworkManager::gCond;

NetworkManager* NetworkManager::mInstance = nullptr;

NetworkManager::NetworkManager(NetworkMode nm) {
    mInstance = this;

    mMode = nm;

    mCompressionLevel = Z_BEST_SPEED;

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "NetworkManager: Initiating network API...\n"
    );
    try {
        initAPI();
    }
    catch(const char* err) {
        throw err;
    }

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "NetworkManager: Getting host info...\n"
    );
    try {
        getHostInfo();
    }
    catch (const char* err) {
        throw err;
    }

    if (mMode == Remote) {
        mIsServer = matchAddress(ClusterManager::instance()->getMasterAddress());
    }
    else if (mMode == LocalServer) {
        mIsServer = true;
    }
    else {
        mIsServer = false;
    }

    if (mIsServer) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Info,
            "NetworkManager: This computer is the network server.\n"
        );
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Info,
            "NetworkManager: This computer is the network client.\n"
        );
    }
}

NetworkManager::~NetworkManager() {
    close();
}

bool NetworkManager::init() {
    sgct_core::ClusterManager& cm = *ClusterManager::instance();
    std::string this_address;
    if (!cm.getThisNodePtr()->getAddress().empty()) {
        this_address = cm.getThisNodePtr()->getAddress();
    }
    else {
        // error
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NetworkManager: No address information for this node availible!\n"
        );
        return false;
    }

    std::string remote_address;
    if (mMode == Remote) {
        if (!cm.getMasterAddress().empty()) {
            remote_address = cm.getMasterAddress();
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "NetworkManager: No address information for master/host availible!\n"
            );
            return false;
        }
    }
    else {
        //local (not remote)
        remote_address = "127.0.0.1";
    }


    // if faking an address (running local) then add it to the search list
    if (mMode != Remote) {
        mLocalAddresses.push_back(cm.getThisNodePtr()->getAddress());
    }

    // ========================================
    // ADD CLUSTER FUNCTIONALITY
    // ========================================
    if (ClusterManager::instance()->getNumberOfNodes() > 1) {
        // sanity check if port is used somewhere else
        for (size_t i = 0; i < mNetworkConnections.size(); i++) {
            const std::string port = mNetworkConnections[i]->getPort();
            if (port == cm.getThisNodePtr()->getSyncPort() ||
                port == cm.getThisNodePtr()->getDataTransferPort() ||
                port == cm.getExternalControlPort())
            {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "NetworkManager: Port %s is already used by connection %u!\n",
                    cm.getThisNodePtr()->getSyncPort().c_str(),
                    i
                );
                return false;
            }
        }

        //if client
        if (!mIsServer) {
            bool addSyncPort = addConnection(
                cm.getThisNodePtr()->getSyncPort(),
                remote_address
            );
            if (addSyncPort) {
                //bind
                mNetworkConnections[mNetworkConnections.size() - 1]->setDecodeFunction(
                    [](const char* data, int length, int index) {
                        sgct::SharedData::instance()->decode(data, length, index);
                    }
                );
            }
            else {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "NetworkManager: Failed to add network connection to %s!\n",
                    cm.getMasterAddress().c_str()
                );
                return false;
            }

            // add data transfer connection
            bool addTransferPort = addConnection(
                cm.getThisNodePtr()->getDataTransferPort(),
                remote_address,
                SGCTNetwork::ConnectionTypes::DataTransfer
            );
            if (addTransferPort) {
                mNetworkConnections[mNetworkConnections.size() - 1]->setPackageDecodeFunction(
                    [](void* data, int length, int packageId, int clientId) {
                        sgct::Engine::instance()->invokeDecodeCallbackForDataTransfer(
                            data,
                            length,
                            packageId,
                            clientId
                        );
                    }
                );

                // acknowledge callback
                mNetworkConnections[mNetworkConnections.size() - 1]->setAcknowledgeFunction(
                    [](int packageId, int clientId) {
                        sgct::Engine::instance()->invokeAcknowledgeCallbackForDataTransfer(
                            packageId,
                            clientId
                        );
                    }
                );
            }
        }

        //add all connections from config file
        for (unsigned int i = 0; i < cm.getNumberOfNodes(); i++) {
            //dont add itself if server
            if (mIsServer && !matchAddress(cm.getNodePtr(i)->getAddress())) {
                bool addSyncPort = addConnection(
                    cm.getNodePtr(i)->getSyncPort(),
                    remote_address
                );
                if (!addSyncPort) {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "NetworkManager: Failed to add network connection to %s!\n",
                        cm.getNodePtr(i)->getAddress().c_str()
                    );
                    return false;
                }
                else {
                    //bind
                    mNetworkConnections[mNetworkConnections.size() - 1]->setDecodeFunction(
                        [](const char* data, int length, int index) {
                            sgct::MessageHandler::instance()->decode(data, length, index);
                        }
                    );
                }

                //add data transfer connection
                bool addTransferPort = addConnection(
                    cm.getNodePtr(i)->getDataTransferPort(),
                    remote_address,
                    SGCTNetwork::ConnectionTypes::DataTransfer
                );
                if (addTransferPort) {
                    mNetworkConnections[mNetworkConnections.size() - 1]->setPackageDecodeFunction(
                        [](void* data, int length, int packageId, int clientId) {
                            sgct::Engine::instance()->invokeDecodeCallbackForDataTransfer(
                                data,
                                length,
                                packageId,
                                clientId
                            );
                        }
                    );

                    // acknowledge callback
                    mNetworkConnections[mNetworkConnections.size() - 1]->setAcknowledgeFunction(
                        [](int packageId, int clientId) {
                            sgct::Engine::instance()->invokeAcknowledgeCallbackForDataTransfer(
                                packageId,
                                clientId
                            );

                        }
                    );
                }
            }
        }
    }

    //add connection for external communication
    if (mIsServer) {
        bool addExternalPort = addConnection(
            cm.getExternalControlPort(),
            "127.0.0.1",
            cm.getUseASCIIForExternalControl() ?
                SGCTNetwork::ConnectionTypes::ExternalASCIIConnection :
                SGCTNetwork::ConnectionTypes::ExternalRawConnection
        );
        if (addExternalPort) {
            mNetworkConnections[mNetworkConnections.size() - 1]->setDecodeFunction(
                [](const char* data, int length, int client) {
                    sgct::Engine::instance()->invokeDecodeCallbackForExternalControl(
                        data,
                        length,
                        client
                    );
                }
            );
        }
    }

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "NetworkManager: Cluster sync is set to %s\n",
        cm.getFirmFrameLockSyncStatus() ? "firm/strict" : "loose"
    );

    return true;
}

/*!
    \param if this application is server/master in cluster then set to true
*/
void NetworkManager::sync(SyncMode sm, Statistics* statsPtr) {
    if (sm == SyncMode::SendDataToClients) {
        double maxTime = -999999.0;
        double minTime = 999999.0;

        for (unsigned int i = 0; i < mSyncConnections.size(); i++) {
            if (mSyncConnections[i]->isServer() && mSyncConnections[i]->isConnected()) {
                double currentTime = mSyncConnections[i]->getLoopTime();
                if (currentTime > maxTime) {
                    maxTime = currentTime;
                }
                if (currentTime < minTime) {
                    minTime = currentTime;
                }

                int currentSize =
                    static_cast<int>(sgct::SharedData::instance()->getDataSize()) -
                    SGCTNetwork::mHeaderSize;

                // iterate counter
                int currentFrame = mSyncConnections[i]->iterateFrameCounter();

                /* The server only writes the sync data and never reads, no need for mutex protection
                sgct::Engine::lockMutex(gMutex);*/
                unsigned char* currentFrameDataPtr = (unsigned char *)&currentFrame;
                unsigned char* currentSizeDataPtr = (unsigned char *)&currentSize;

                sgct::SharedData::instance()->getDataBlock()[1] = currentFrameDataPtr[0];
                sgct::SharedData::instance()->getDataBlock()[2] = currentFrameDataPtr[1];
                sgct::SharedData::instance()->getDataBlock()[3] = currentFrameDataPtr[2];
                sgct::SharedData::instance()->getDataBlock()[4] = currentFrameDataPtr[3];
                sgct::SharedData::instance()->getDataBlock()[5] = currentSizeDataPtr[0];
                sgct::SharedData::instance()->getDataBlock()[6] = currentSizeDataPtr[1];
                sgct::SharedData::instance()->getDataBlock()[7] = currentSizeDataPtr[2];
                sgct::SharedData::instance()->getDataBlock()[8] = currentSizeDataPtr[3];

                //sgct::MessageHandler::instance()->print("NetworkManager::sync size %u\n", currentSize);

                //send
                mSyncConnections[i]->sendData(
                    sgct::SharedData::instance()->getDataBlock(),
                    static_cast<int>(sgct::SharedData::instance()->getDataSize())
                );
                //sgct::Engine::unlockMutex(gMutex);
            }
        }//end for

        if (isComputerServer()) {
            statsPtr->setLoopTime(
                static_cast<float>(minTime),
                static_cast<float>(maxTime)
            );
        }
    }
    else if (sm == SyncMode::AcknowledgeData) {
        for (unsigned int i = 0; i < mSyncConnections.size(); i++) {
            //Client
            if (!mSyncConnections[i]->isServer() && mSyncConnections[i]->isConnected()) {
                // The servers's render function is locked until a message starting with
                // the ack-byte is received.

                //send message to server
                mSyncConnections[i]->pushClientMessage();
            }
        }
    }
}

/*!
    Compare if the last frame and current frames are different -> data update
    And if send frame == recieved frame
*/
bool NetworkManager::isSyncComplete() {
    unsigned int counter = 0;
    for (unsigned int i = 0; i < mSyncConnections.size(); i++) {
        if (mSyncConnections[i]->isUpdated()) {
            //has all data been received?
            counter++;
        }
    }

#ifdef __SGCT_NETWORK_DEBUG__
    sgct::MessageHandler::instance()->printDebug(
        sgct::MessageHandler::Level::Debug,
        "SGCTNetworkManager::isSyncComplete: counter %u of %u\n",
        counter, getSyncConnectionsCount()
    );
#endif

    return counter == getActiveSyncConnectionsCount();
}

SGCTNetwork* NetworkManager::getExternalControlPtr() {
    return mExternalControlConnection;
}

void NetworkManager::transferData(const void* data, int length, int packageId) {
    char* buffer = nullptr;
    int sendSize = length;

    if (prepareTransferData(data, &buffer, sendSize, packageId)) {
        //Send the data
        for (size_t i = 0; i < mDataTransferConnections.size(); i++) {
            if (mDataTransferConnections[i]->isConnected()) {
                mDataTransferConnections[i]->sendData(buffer, sendSize);
            }
        }
    }

    if (buffer) {
        //clean up
        delete[] buffer;
        buffer = nullptr;
    }
}

void NetworkManager::transferData(const void* data, int length, int packageId,
                                  size_t nodeIndex)
{
    if (nodeIndex < mDataTransferConnections.size() &&
        mDataTransferConnections[nodeIndex]->isConnected())
    {
        char* buffer = nullptr;
        int sendSize = length;

        if (prepareTransferData(data, &buffer, sendSize, packageId)) {
            mDataTransferConnections[nodeIndex]->sendData(buffer, sendSize);
        }

        if (buffer) {
            //clean up
            delete[] buffer;
            buffer = nullptr;
        }
    }
}

void NetworkManager::transferData(const void* data, int length, int packageId,
                                  SGCTNetwork* connection)
{
    if (connection->isConnected()) {
        char* buffer = nullptr;
        int sendSize = length;

        if (prepareTransferData(data, &buffer, sendSize, packageId)) {
            connection->sendData(buffer, sendSize);
        }

        if (buffer) {
            //clean up
            delete[] buffer;
            buffer = nullptr;
        }
    }
}

bool NetworkManager::prepareTransferData(const void* data, char** bufferPtr, int& length,
                                         int packageId)
{
    int msg_len = length;

    if (mCompress) {
        length = compressBound(static_cast<uLong>(length));
    }
    length += static_cast<int>(SGCTNetwork::mHeaderSize);

    (*bufferPtr) = new (std::nothrow) char[length];
    if ((*bufferPtr) != nullptr) {
        char* packageIdPtr = reinterpret_cast<char*>(&packageId);

        (*bufferPtr)[0] = mCompress ? SGCTNetwork::CompressedDataId : SGCTNetwork::DataId;
        (*bufferPtr)[1] = packageIdPtr[0];
        (*bufferPtr)[2] = packageIdPtr[1];
        (*bufferPtr)[3] = packageIdPtr[2];
        (*bufferPtr)[4] = packageIdPtr[3];

        char* compDataPtr = (*bufferPtr) + SGCTNetwork::mHeaderSize;

        if (mCompress) {
            uLong compressedSize = static_cast<uLongf>(length - SGCTNetwork::mHeaderSize);
            int err = compress2(
                reinterpret_cast<Bytef*>(compDataPtr),
                &compressedSize,
                reinterpret_cast<const Bytef*>(data),
                static_cast<uLong>(length),
                mCompressionLevel
            );

            if (err != Z_OK) {
                std::string errStr;
                switch (err) {
                    case Z_BUF_ERROR:
                        errStr = "Dest. buffer not large enough.";
                        break;
                    case Z_MEM_ERROR:
                        errStr = "Insufficient memory.";
                        break;
                    case Z_STREAM_ERROR:
                        errStr = "Incorrect compression level.";
                        break;
                    default:
                        errStr = "Unknown error.";
                        break;
                }

                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "NetworkManager: Failed to compress data! Error: %s\n",
                    errStr.c_str()
                );
                return false;
            }

            //send original size
            char* uncompressedSizePtr = reinterpret_cast<char*>(&length);
            (*bufferPtr)[9] = uncompressedSizePtr[0];
            (*bufferPtr)[10] = uncompressedSizePtr[1];
            (*bufferPtr)[11] = uncompressedSizePtr[2];
            (*bufferPtr)[12] = uncompressedSizePtr[3];

            length = static_cast<int>(compressedSize);
            //re-calculate the true send size
            length = length + static_cast<int>(SGCTNetwork::mHeaderSize);
        }
        else {
            //set uncompressed size to DefaultId since compression is not used
            memset(
                (*bufferPtr) + 9,
                SGCTNetwork::DefaultId,
                4
            );

            //add data to buffer
            //memcpy(buffer + SGCTNetwork::mHeaderSize, data, length);
            //faster to copy chunks of 4k than the whole buffer
            int offset = 0;
            int stride = 4096;

            while (offset < msg_len) {
                if ((msg_len - offset) < stride) {
                    stride = msg_len - offset;
                }

                memcpy(
                    (*bufferPtr) + SGCTNetwork::mHeaderSize + offset,
                    reinterpret_cast<const char*>(data)+offset,
                    stride
                );
                offset += stride;
            }
        }

        char* sizePtr = reinterpret_cast<char*>(&msg_len);
        (*bufferPtr)[5] = sizePtr[0];
        (*bufferPtr)[6] = sizePtr[1];
        (*bufferPtr)[7] = sizePtr[2];
        (*bufferPtr)[8] = sizePtr[3];
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NetworkManager: Failed to allocate data for transfer (bytes %d)!\n",
            length
        );
        return false;
    }

    return true;
}

/*!
 Compression levels 1-9.
 -1 = Default compression
 0 = No compression
 1 = Best speed
 9 = Best compression
 */
void NetworkManager::setDataTransferCompression(bool state, int level) {
    mCompress = state;
    mCompressionLevel = level;
}

unsigned int NetworkManager::getActiveConnectionsCount() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned int retVal = mNumberOfActiveConnections;
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return retVal;
}

unsigned int NetworkManager::getActiveSyncConnectionsCount() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned int retVal = mNumberOfActiveSyncConnections;
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return retVal;
}

unsigned int NetworkManager::getActiveDataTransferConnectionsCount() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned int retVal = mNumberOfActiveDataTransferConnections;
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return retVal;
}

unsigned int NetworkManager::getConnectionsCount() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned int retVal = static_cast<unsigned int>(mNetworkConnections.size());
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return retVal;
}

unsigned int NetworkManager::getSyncConnectionsCount() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned int retVal = static_cast<unsigned int>(mSyncConnections.size());
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return retVal;
}

unsigned int NetworkManager::getDataTransferConnectionsCount() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned int retVal = static_cast<unsigned int>(mDataTransferConnections.size());
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return retVal;
}

SGCTNetwork* NetworkManager::getConnectionByIndex(unsigned int index) const {
    return mNetworkConnections[index];
}

SGCTNetwork* NetworkManager::getSyncConnectionByIndex(unsigned int index) const {
    return mSyncConnections[index];
}

std::vector<std::string> NetworkManager::getLocalAddresses() {
    return mLocalAddresses;
}

void NetworkManager::updateConnectionStatus(SGCTNetwork* connection) {
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "NetworkManager: Updating status for connection %d\n",
        connection->getId()
    );

    unsigned int numberOfConnectionsCounter = 0;
    unsigned int numberOfConnectedSyncNodesCounter = 0;
    unsigned int numberOfConnectedDataTransferNodesCounter = 0;

    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned int totalNumberOfConnections =
        static_cast<unsigned int>(mNetworkConnections.size());
    unsigned int totalNumberOfSyncConnections =
        static_cast<unsigned int>(mSyncConnections.size());
    unsigned int totalNumberOfTransferConnections =
        static_cast<unsigned int>(mDataTransferConnections.size());
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();

    //count connections
    for (unsigned int i = 0; i < mNetworkConnections.size(); i++) {
        if (mNetworkConnections[i] != nullptr && mNetworkConnections[i]->isConnected()) {
            numberOfConnectionsCounter++;
            if (mNetworkConnections[i]->getType() == SGCTNetwork::ConnectionTypes::SyncConnection) {
                numberOfConnectedSyncNodesCounter++;
            }
            else if (mNetworkConnections[i]->getType() == SGCTNetwork::ConnectionTypes::DataTransfer) {
                numberOfConnectedDataTransferNodesCounter++;
            }
        }
    }

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "NetworkManager: Number of active connections %u of %u\n",
        numberOfConnectionsCounter, totalNumberOfConnections
    );
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "NetworkManager: Number of connected sync nodes %u of %u\n",
        numberOfConnectedSyncNodesCounter, totalNumberOfSyncConnections
    );
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "NetworkManager: Number of connected data transfer nodes %u of %u\n",
        numberOfConnectedDataTransferNodesCounter,totalNumberOfTransferConnections
    );

    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    mNumberOfActiveConnections = numberOfConnectionsCounter;
    mNumberOfActiveSyncConnections = numberOfConnectedSyncNodesCounter;
    mNumberOfActiveDataTransferConnections = numberOfConnectedDataTransferNodesCounter;

    //create a local copy to use so we don't need mutex on several locations
    bool isServer = mIsServer;

    //if client disconnects then it cannot run anymore
    if (mNumberOfActiveSyncConnections == 0 && !isServer) {
        mIsRunning = false;
    }
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();

    if (isServer) {
        bool allNodesConnectedCopy;

        sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
        //local copy (thread safe)
        allNodesConnectedCopy =
            (numberOfConnectedSyncNodesCounter == totalNumberOfSyncConnections) &&
            (numberOfConnectedDataTransferNodesCounter == totalNumberOfTransferConnections);

        mAllNodesConnected = allNodesConnectedCopy;
        sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();

        //send cluster connected message to nodes/slaves
        if (allNodesConnectedCopy) {
            for (unsigned int i = 0; i < mSyncConnections.size(); i++) {
                if (mSyncConnections[i]->isConnected()) {
                    char tmpc[SGCTNetwork::mHeaderSize];
                    tmpc[0] = SGCTNetwork::ConnectedId;
                    for (unsigned int j = 1; j < SGCTNetwork::mHeaderSize; j++) {
                        tmpc[j] = SGCTNetwork::DefaultId;
                    }

                    mSyncConnections[i]->sendData(&tmpc, SGCTNetwork::mHeaderSize);
                }
            }
            for (unsigned int i = 0; i < mDataTransferConnections.size(); i++) {
                if (mDataTransferConnections[i]->isConnected()) {
                    char tmpc[SGCTNetwork::mHeaderSize];
                    tmpc[0] = SGCTNetwork::ConnectedId;
                    for (unsigned int j = 1; j < SGCTNetwork::mHeaderSize; j++)
                        tmpc[j] = SGCTNetwork::DefaultId;

                    mDataTransferConnections[i]->sendData(&tmpc, SGCTNetwork::mHeaderSize);
                }
            }
        }

        /*
            Check if any external connection
        */
        if (connection->getType() ==
                sgct_core::SGCTNetwork::ConnectionTypes::ExternalASCIIConnection)
        {
            bool externalControlConnectionStatus = connection->isConnected();
            std::string msg = "Connected to SGCT!\r\n";
            connection->sendData(msg.c_str(), static_cast<int>(msg.size()));
            sgct::Engine::instance()->invokeUpdateCallbackForExternalControl(
                externalControlConnectionStatus
            );
        }
        else if (connection->getType() ==
                sgct_core::SGCTNetwork::ConnectionTypes::ExternalRawConnection)
        {
            bool externalControlConnectionStatus = connection->isConnected();
            sgct::Engine::instance()->invokeUpdateCallbackForExternalControl(
                externalControlConnectionStatus
            );
        }

        //wake up the connection handler thread on server
        //if node disconnects to enable reconnection
        connection->mStartConnectionCond.notify_all();
    }


    if (connection->getType() ==
            sgct_core::SGCTNetwork::ConnectionTypes::DataTransfer)
    {
        bool dataTransferConnectionStatus = connection->isConnected();
        sgct::Engine::instance()->invokeUpdateCallbackForDataTransfer(
            dataTransferConnectionStatus,
            connection->getId()
        );
    }

    //signal done to caller
    gCond.notify_all();
}

void NetworkManager::setAllNodesConnected() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();

    if (!mIsServer) {
        unsigned int totalNumberOfTransferConnections =
            static_cast<unsigned int>( mDataTransferConnections.size());
        mAllNodesConnected = (mNumberOfActiveSyncConnections == 1) &&
            (mNumberOfActiveDataTransferConnections == totalNumberOfTransferConnections);
    }
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void NetworkManager::close() {
    mIsRunning = false;

    //release condition variables
    gCond.notify_all();

    //signal to terminate
    for (unsigned int i = 0; i < mNetworkConnections.size(); i++) {
        if (mNetworkConnections[i] != nullptr) {
            mNetworkConnections[i]->initShutdown();
        }
    }

    //wait for all nodes callbacks to run
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    //wait for threads to die
    for (unsigned int i = 0; i < mNetworkConnections.size(); i++) {
        if (mNetworkConnections[i] != nullptr) {
            mNetworkConnections[i]->closeNetwork(false);
            delete mNetworkConnections[i];
        }
    }

    mNetworkConnections.clear();
    mSyncConnections.clear();
    mDataTransferConnections.clear();

#if defined(_WIN_PLATFORM)
    WSACleanup();
#else
    //No cleanup needed
#endif
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "NetworkManager: Network API closed!\n"
    );
}

bool NetworkManager::addConnection(const std::string& port, const std::string& address,
                                   SGCTNetwork::ConnectionTypes connectionType)
{
    if (port.empty()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Info,
            "NetworkManager: No port set for %s!\n",
            SGCTNetwork::getTypeStr(connectionType).c_str()
        );
        return false;
    }

    if (address.empty()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NetworkManager: Error: No address set for %s!\n",
            SGCTNetwork::getTypeStr(connectionType).c_str()
        );
        return false;
    }

    SGCTNetwork* netPtr = new SGCTNetwork();

    try {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "NetworkManager: Initiating network connection %d at port %s.\n",
            mNetworkConnections.size(), port.c_str()
        );

        //bind callback
        netPtr->setUpdateFunction([this](SGCTNetwork* c) { updateConnectionStatus(c); });

        //bind callback
        netPtr->setConnectedFunction([this]() { setAllNodesConnected(); });

        if (connectionType == SGCTNetwork::ConnectionTypes::SyncConnection) {
            mSyncConnections.push_back(netPtr);
        }
        else if (connectionType == SGCTNetwork::ConnectionTypes::DataTransfer) {
            mDataTransferConnections.push_back(netPtr);
        }
        else {
            mExternalControlConnection = netPtr;
        }
        mNetworkConnections.push_back(netPtr);


        //must be inited after binding
        netPtr->init(port, address, mIsServer, connectionType);
    }
    catch (const std::runtime_error& e) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NetworkManager: Network error: %s\n",
            e.what()
        );
        return false;
    }
    catch (const char* err) {
        // abock (2019-08-17):  I don't think catch block is necessary anymore as we now
        //                      throw the correct type from SGCTNetwork, but I don't dare
        //                      to remove it yet
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NetworkManager: Network error: %s\n",
            err
        );
        return false;
    }

    return true;
}

void NetworkManager::initAPI() {
#if defined(_WIN_PLATFORM)
    WSADATA wsaData;
    WORD version;
    int error;

    version = MAKEWORD(2, 2);

    error = WSAStartup(version, &wsaData);

    if (error != 0 || LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2 ) {
        /* incorrect WinSock version */
        WSACleanup();
        throw "Winsock 2.2 startup failed!";
    }
#else
    //No init needed
#endif

}

void sgct_core::NetworkManager::getHostInfo() {
    //get name & local ips
    //retrieves the standard host name for the local computer
    char tmpStr[128];
    if (gethostname(tmpStr, sizeof(tmpStr)) == SOCKET_ERROR) {
#if defined(_WIN_PLATFORM)
        WSACleanup();
#else
        //No cleanup needed
#endif
        throw "Failed to get host name!";
    }

    mHostName = tmpStr;
    //add hostname and adress in lower case
    std::transform(mHostName.begin(), mHostName.end(), mHostName.begin(), ::tolower);
    mLocalAddresses.push_back(mHostName);

    addrinfo hints;
    sockaddr_in* sockaddr_ipv4;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    //hints.ai_family = AF_UNSPEC; /*either IPV4 or IPV6*/
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_CANONNAME;

    addrinfo* info;
    int result = getaddrinfo(tmpStr, "http", &hints, &info);
    if (result != 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "NetworkManager: Failed to get address info (error %d)!\n",
            SGCTNetwork::getLastError()
        );
    }
    else {
        char addr_str[INET_ADDRSTRLEN];
        for (addrinfo* p = info; p != nullptr; p = p->ai_next) {
            sockaddr_ipv4 = reinterpret_cast<sockaddr_in*>(p->ai_addr);
            inet_ntop(AF_INET, &(sockaddr_ipv4->sin_addr), addr_str, INET_ADDRSTRLEN);
            if (p->ai_canonname) {
                mDNSNames.push_back(std::string(p->ai_canonname));
            }
            mLocalAddresses.push_back(std::string(addr_str));
        }
    }

    freeaddrinfo(info);

    for (size_t i = 0; i < mDNSNames.size(); i++) {
        std::transform(
            mDNSNames[i].begin(),
            mDNSNames[i].end(),
            mDNSNames[i].begin(),
            ::tolower
        );
        mLocalAddresses.push_back(mDNSNames[i]);
    }

    //add the loop-back
    mLocalAddresses.push_back("127.0.0.1");
    mLocalAddresses.push_back("localhost");
}

bool NetworkManager::matchAddress(const std::string& address) {
    for (unsigned int i = 0; i<mLocalAddresses.size(); i++)
        if (strcmp(address.c_str(), mLocalAddresses[i].c_str()) == 0)
            return true;
    //No match
    return false;
}

bool NetworkManager::isComputerServer() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    bool tmpB = mIsServer;
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return tmpB;
}

bool NetworkManager::isRunning() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    bool tmpB = mIsRunning;
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return tmpB;
}

bool NetworkManager::areAllNodesConnected() {
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.lock();
    bool tmpB = mAllNodesConnected;
    sgct::SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    return tmpB;
}

/*!
    Retrieve the node id if this node is part of the cluster configuration
*/
void NetworkManager::retrieveNodeId() {
    for (size_t i = 0; i < ClusterManager::instance()->getNumberOfNodes(); i++) {
        //check ip
        if ( matchAddress(ClusterManager::instance()->getNodePtr(i)->getAddress())) {
            ClusterManager::instance()->setThisNodeId(static_cast<int>(i));
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "NetworkManager: Running in cluster mode as node %d\n",
                ClusterManager::instance()->getThisNodeId()
            );
            break;
        }
    }
}

} // namespace sgct_core
