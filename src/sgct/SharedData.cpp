/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SharedData.h>

#include <sgct/Engine.h>
#include <sgct/SGCTMutexManager.h>
#include <sgct/MessageHandler.h>
#include <zlib.h>
#include <string>

namespace sgct {

SharedData* SharedData::mInstance = nullptr;

SharedData* SharedData::instance() {
    if (mInstance == nullptr) {
        mInstance = new SharedData();
    }

    return mInstance;
}

void SharedData::destroy() {
    if (mInstance != nullptr) {
        delete mInstance;
        mInstance = nullptr;
    }
}

SharedData::SharedData() {
    constexpr const int DefaultSize = 1024;

    // use a compression buffer twice as large to fit huffman tree + data which can be
    // larger than original data in some cases. Normally a sixe x 1.1 should be enough.
    mCompressedBuffer.resize(DefaultSize * 2);

    dataBlock.reserve(DefaultSize);
    dataBlockToCompress.reserve(DefaultSize);

    mCompressionLevel = Z_BEST_SPEED;

    if (mUseCompression) {
        currentStorage = &dataBlockToCompress;
    }
    else {
        currentStorage = &dataBlock;
    }

    // fill rest of header with SGCTNetwork::DefaultId
    memset(
        headerSpace.data(),
        sgct_core::SGCTNetwork::DefaultId,
        sgct_core::SGCTNetwork::HeaderSize
    );

    headerSpace[0] = sgct_core::SGCTNetwork::DataId;
}

void SharedData::setCompression(bool state, int level) {
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    mUseCompression = state;
    mCompressionLevel = level;

    if (mUseCompression) {
        currentStorage = &dataBlockToCompress;
    }
    else {
        currentStorage = &dataBlock;
        mCompressionRatio = 1.f;
    }
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

float SharedData::getCompressionRatio() {
    return mCompressionRatio;
}

void SharedData::setEncodeFunction(void(*fnPtr)(void)) {
    mEncodeFn = fnPtr;
}

void SharedData::setDecodeFunction(void(*fnPtr)(void)) {
    mDecodeFn = fnPtr;
}

void SharedData::decode(const char* receivedData, int receivedlength, int) {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        MessageHandler::Level::NotifyAll,
        "SharedData::decode\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();

    // reset
    pos = 0;
    dataBlock.clear();

    if (receivedlength > static_cast<int>(dataBlock.capacity())) {
        dataBlock.reserve(receivedlength);
    }
    dataBlock.insert(dataBlock.end(), receivedData, receivedData + receivedlength);

    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

    if (mDecodeFn != nullptr) {
        mDecodeFn();
    }
}

void SharedData::encode() {
#ifdef __SGCT_NETWORK_DEBUG__
    MessageHandler::instance()->printDebug(
        sgct::MessageHandler::NOTIFY_ALL,
        "SharedData::encode\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();

    dataBlock.clear();
    if (mUseCompression) {
        dataBlockToCompress.clear();
        headerSpace[0] = sgct_core::SGCTNetwork::CompressedDataId;
    }
    else {
        headerSpace[0] = sgct_core::SGCTNetwork::DataId;
    }

    //reserve header space
    dataBlock.insert(
        dataBlock.begin(),
        headerSpace.begin(),
        headerSpace.begin() + sgct_core::SGCTNetwork::HeaderSize
    );

    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

    if (mEncodeFn != nullptr) {
        mEncodeFn();
    }

    if (mUseCompression && !dataBlockToCompress.empty()) {
        SGCTMutexManager::instance()->mDataSyncMutex.lock();

        // re-allocate if needed use a compression buffer twice as large to fit huffman
        // tree + data which can  larger than original data in some cases.
        // Normally a sixe x 1.1 should be enough.
        if (mCompressedBuffer.size() < (dataBlockToCompress.size() / 2)) {
            mCompressedBuffer.resize(dataBlockToCompress.size() * 2);
        }

        uLongf compressedSize = static_cast<uLongf>(mCompressedBuffer.size());
        uLongf dataSize = static_cast<uLongf>(dataBlockToCompress.size());
        int err = compress2(
            mCompressedBuffer.data(),
            &compressedSize,
            dataBlockToCompress.data(),
            dataSize,
            mCompressionLevel
        );

        if (err == Z_OK) {
            // add original size
            uint32_t uncompressedSize = static_cast<uint32_t>(dataBlockToCompress.size());
            unsigned char* p = reinterpret_cast<unsigned char*>(&uncompressedSize);

            dataBlock[9] = p[0];
            dataBlock[10] = p[1];
            dataBlock[11] = p[2];
            dataBlock[12] = p[3];
            
            mCompressionRatio = static_cast<float>(compressedSize) /
                                static_cast<float>(uncompressedSize);

            // add the compressed block
            dataBlock.insert(
                dataBlock.end(),
                mCompressedBuffer.begin(),
                mCompressedBuffer.begin() + compressedSize
            );
        }
        else {
            SGCTMutexManager::instance()->mDataSyncMutex.unlock();
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "SharedData: Failed to compress data (error %d)\n", err
            );
            return;
        }

        SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    }
}

std::size_t SharedData::getUserDataSize() {
    return dataBlock.size() - sgct_core::SGCTNetwork::HeaderSize;
}

unsigned char* SharedData::getDataBlock() {
    return dataBlock.data();
}

size_t SharedData::getDataSize() {
    return dataBlock.size();
}

size_t SharedData::getBufferSize() {
    return dataBlock.capacity();
}

void SharedData::writeFloat(const SharedFloat& sf) {
#ifdef __SGCT_NETWORK_DEBUG__    
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeFloat\nFloat = %f", sf->getVal()
    );
#endif

    float val = sf.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(float));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeDouble(const SharedDouble& sd) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeDouble\nDouble = %f\n", sd->getVal()
    );
#endif

    double val = sd.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(double));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeInt64(const SharedInt64& si) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeInt64\nInt = %ld\n", si->getVal()
    );
#endif

    int64_t val = si.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char* >(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(int64_t));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeInt32(const SharedInt32& si) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeInt32\nInt = %d\n", si->getVal()
    );
#endif

    int32_t val = si.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(int32_t));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeInt16(const SharedInt16& si) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeInt16\nInt = %d\n", si->getVal()
    );
#endif

    int16_t val = si.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(int16_t));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeInt8(const SharedInt8& si) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeInt8\nInt = %d\n", si->getVal()
    );
#endif

    int8_t val = si.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(int8_t));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeUInt64(const SharedUInt64& si) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->pruintDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeUInt64\nUInt = %lu\n", si->getVal()
    );
#endif

    uint64_t val = si.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(uint64_t));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeUInt32(const SharedUInt32& si) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeUInt32\nUInt = %u\n", si->getVal()
    );
#endif

    uint32_t val = si.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(uint32_t));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeUInt16(const SharedUInt16& si) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->pruintDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeUInt16\nUInt = %u\n", si->getVal()
    );
#endif

    uint16_t val = si.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(uint16_t));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeUInt8(const SharedUInt8& si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->pruintDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeUInt8\nUInt = %u\n", si->getVal()
    );
#endif

    uint8_t val = si.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    currentStorage->insert(currentStorage->end(), p, p + sizeof(uint8_t));
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeUChar(const SharedUChar& suc) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeUChar\n"
    );
#endif

    unsigned char val = suc.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    currentStorage->push_back(val);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeBool(const SharedBool& sb) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeBool\n"
    );
#endif
    
    bool val = sb.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    if (val) {
        currentStorage->push_back(1);
    }
    else {
        currentStorage->push_back(0);
    }
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeString(const SharedString& ss) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeString\n"
    );
#endif
    
    std::string tmpStr = ss.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    uint32_t length = static_cast<uint32_t>(tmpStr.size());
    unsigned char* p = reinterpret_cast<unsigned char*>(&length);
    
    currentStorage->insert(currentStorage->end(), p, p + sizeof(uint32_t));
    currentStorage->insert(currentStorage->end(), tmpStr.data(), tmpStr.data() + length);
    
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::writeWString(const SharedWString& ss) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::writeWString\n"
    );
#endif

    std::wstring tmpStr = ss.getVal();
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    uint32_t length = static_cast<uint32_t>(tmpStr.size());
    unsigned char* p = reinterpret_cast<unsigned char*>(&length);
    unsigned char* ws = reinterpret_cast<unsigned char*>(&tmpStr[0]);

    currentStorage->insert(currentStorage->end(), p, p + sizeof(uint32_t));
    currentStorage->insert(currentStorage->end(), ws, ws + length * sizeof(wchar_t));

    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
}

void SharedData::readFloat(SharedFloat& sf) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readFloat\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    
    float val = *reinterpret_cast<float*>(&dataBlock[pos]);
    pos += sizeof(float);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "Float = %f\n", val
    );
#endif

    sf.setVal(val);
}

void SharedData::readDouble(SharedDouble& sd) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readDouble\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    double val = *reinterpret_cast<double*>(&dataBlock[pos]);
    pos += sizeof(double);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "Double = %lf\n", val
    );
#endif

    sd.setVal(val);
}

void SharedData::readInt64(SharedInt64& si) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readInt64\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    int64_t val = *reinterpret_cast<int64_t*>(&dataBlock[pos]);
    pos += sizeof(int64_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "Int64 = %ld\n", val
    );
#endif

    si.setVal(val);
}

void SharedData::readInt32(SharedInt32& si) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readInt32\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    int32_t val = *reinterpret_cast<int32_t*>(&dataBlock[pos]);
    pos += sizeof(int32_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "Int32 = %d\n", val
    );
#endif

    si.setVal(val);
}

void SharedData::readInt16(SharedInt16& si) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readInt16\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    int16_t val = *reinterpret_cast<int16_t*>(&dataBlock[pos]);
    pos += sizeof(int16_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "Int16 = %d\n", val
    );
#endif

    si.setVal(val);
}

void SharedData::readInt8(SharedInt8& si) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readInt8\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    int8_t val = *reinterpret_cast<int8_t*>(&dataBlock[pos]);
    pos += sizeof(int8_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "Int8 = %d\n", val
    );
#endif

    si.setVal(val);
}

void SharedData::readUInt64(SharedUInt64& si) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readUInt64\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    uint64_t val = *reinterpret_cast<uint64_t*>(&dataBlock[pos]);
    pos += sizeof(uint64_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "UInt64 = %lu\n", val
    );
#endif

    si.setVal(val);
}

void SharedData::readUInt32(SharedUInt32& si) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readUInt32\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    uint32_t val = *reinterpret_cast<uint32_t*>(&dataBlock[pos]);
    pos += sizeof(uint32_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "UInt32 = %u\n", val
    );
#endif

    si.setVal(val);
}

void SharedData::readUInt16(SharedUInt16& si) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readUInt16\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    uint16_t val = *reinterpret_cast<uint16_t*>(&dataBlock[pos]);
    pos += sizeof(uint16_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "UInt16 = %u\n", val
    );
#endif

    si.setVal(val);
}

void SharedData::readUInt8(SharedUInt8& si) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readUInt8\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    uint8_t val = *reinterpret_cast<uint8_t*>(&dataBlock[pos]);
    pos += sizeof(uint8_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "UInt8 = %u\n", val
    );
#endif

    si.setVal(val);
}

void SharedData::readUChar(SharedUChar& suc) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readUChar\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    unsigned char c = dataBlock[pos];
    pos += sizeof(unsigned char);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "UChar = %d\n", c
    );
#endif
    suc.setVal(c);
}

void SharedData::readBool(SharedBool& sb) {
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readBool\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    bool b = dataBlock[pos] == 1;
    pos += 1;
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(MessageHandler::NOTIFY_INFO, "Bool = %d\n", b);
#endif
    sb.setVal(b);
}

void SharedData::readString(SharedString& ss) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readString\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();
    
    uint32_t length = *reinterpret_cast<uint32_t*>(&dataBlock[pos]);
    pos += sizeof(uint32_t);

    if (length == 0) {
        ss.setVal("");
        return;
    }

    std::string stringData(dataBlock.begin() + pos, dataBlock.begin() + pos + length);
    pos += length;
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();
    
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "String = '%s'\n", stringData
    );
#endif

    ss.setVal(std::move(stringData));
}

void SharedData::readWString(SharedWString& ss) {
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(
        MessageHandler::NOTIFY_INFO,
        "SharedData::readWString\n"
    );
#endif
    SGCTMutexManager::instance()->mDataSyncMutex.lock();

    uint32_t length = *(reinterpret_cast<uint32_t*>(&dataBlock[pos]));
    pos += sizeof(uint32_t);

    if (length == 0) {
        ss.setVal(std::wstring());
        return;
    }

    std::wstring stringData(dataBlock.begin() + pos, dataBlock.begin() + pos + length);
    pos += length * sizeof(wchar_t);
    SGCTMutexManager::instance()->mDataSyncMutex.unlock();

    ss.setVal(std::move(stringData));
}

} // namespace sgct
