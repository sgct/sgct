/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__SHAREDDATA__H__
#define __SGCT__SHAREDDATA__H__

#include <sgct/mutexes.h>
#include <sgct/network.h>
#include <sgct/shareddatatypes.h>
#include <array>
#include <string>
#include <vector>

namespace sgct {

/**
 * This class shares application data between nodes in a cluster where the master encodes
 * and transmits the data and the clients receives and decode the data. If a large number
 * of strings are used for the synchronization then the data can be compressed using the
 * setCompression function. The process of synchronization is serial which means that the
 * order of encoding must be the same as in decoding.
 */
class SharedData {
public:
    static SharedData& instance();
    static void destroy();

    /**
     * Compression levels 1-9.
     *   -1 = Default compression
     *    0 = No compression
     *    1 = Best speed
     *    9 = Best compression
     */
    void setCompression(bool state, int level = 1);

    /**
     * Get the compresson ratio:
     *   ratio = (compressed data size + Huffman tree)/(original data size)
     * If the ratio is larger than 1.0 then there is no use for using compression.
     */
    float getCompressionRatio() const;

    template<class T>
    void writeObj(const SharedObject<T>& sobj);
    void writeFloat(const SharedFloat& sf);
    void writeDouble(const SharedDouble& sd);
    
    void writeInt64(const SharedInt64& si);
    void writeInt32(const SharedInt32& si);
    void writeInt16(const SharedInt16& si);
    void writeInt8(const SharedInt8& si);
    
    void writeUInt64(const SharedUInt64& si);
    void writeUInt32(const SharedUInt32& si);
    void writeUInt16(const SharedUInt16& si);
    void writeUInt8(const SharedUInt8& si);
    
    void writeUChar(const SharedUChar& suc);
    void writeBool(const SharedBool& sb);
    void writeString(const SharedString& ss);
    void writeWString(const SharedWString& ss);
    template<class T>
    void writeVector(const SharedVector<T>& vector);

    template<class T>
    void readObj(SharedObject<T>& sobj);
    void readFloat(SharedFloat& f);
    void readDouble(SharedDouble& d);
    
    void readInt64(SharedInt64& si);
    void readInt32(SharedInt32& si);
    void readInt16(SharedInt16& si);
    void readInt8(SharedInt8& si);
    
    void readUInt64(SharedUInt64& si);
    void readUInt32(SharedUInt32& si);
    void readUInt16(SharedUInt16& si);
    void readUInt8(SharedUInt8& si);
    
    void readUChar(SharedUChar& suc);
    void readBool(SharedBool& sb);
    void readString(SharedString& ss);
    void readWString(SharedWString& ss);
    template<class T>
    void readVector(SharedVector<T>& vector);

    /**
     * Set the encode callback.
     * Sample of a encode function:
     * \code{.cpp}
void encodeFun()
{
    sgct::SharedData::instance().writeDouble(currentTime);
}
\endcode
    */
    void setEncodeFunction(std::function<void()>);

    /**
     * Set the decoder callback.
     * Sample of a decode function:
\code{.cpp}
void decodeFun()
{
    currentTime = sgct::SharedData::instance().readDouble();
}
\endcode
    */
    void setDecodeFunction(std::function<void()>);

    /// This fuction is called internally by SGCT and shouldn't be used by the user.
    void encode();

    /// This function is called internally by SGCT and shouldn't be used by the user.
    void decode(const char* receivedData, int receivedLength, int clientIndex);

    size_t getUserDataSize();
    unsigned char* getDataBlock();
    size_t getDataSize();
    size_t getBufferSize();

private:
    SharedData();

    // function pointers
    std::function<void()> _encodeFn;
    std::function<void()> _decodeFn;

    static SharedData* _instance;
    std::vector<unsigned char> _dataBlock;
    std::vector<unsigned char> _dataBlockToCompress;
    std::vector<unsigned char>* _currentStorage;
    std::vector<unsigned char> _compressedBuffer;
    std::array<unsigned char, core::Network::HeaderSize> _headerSpace;
    unsigned int _pos = 0;
    int _compressionLevel;
    float _compressionRatio = 1.f;
    bool _useCompression = false;
};



template <class T>
void SharedData::writeObj(const SharedObject<T>& sobj) {
    T val = sobj.getVal();
    
    std::unique_lock lk(core::mutex::DataSync);
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    _currentStorage->insert(_currentStorage->end(), p, p + sizeof(T));
}

template<class T>
void SharedData::readObj(SharedObject<T>& sobj) {
    core::mutex::DataSync.lock();
    T val = *reinterpret_cast<T*>(&_dataBlock[_pos]);
    _pos += sizeof(T);
    core::mutex::DataSync.unlock();

    sobj.setVal(val);
}

template<class T>
void SharedData::writeVector(const SharedVector<T>& vector) {
    std::vector<T> tmpVec = vector.getVal();

    uint32_t vectorSize = static_cast<uint32_t>(tmpVec.size());
    core::mutex::DataSync.lock();
    unsigned char* p = reinterpret_cast<unsigned char*>(&vectorSize);
    _currentStorage->insert(_currentStorage->end(), p, p + sizeof(uint32_t));
    core::mutex::DataSync.unlock();

    if (vectorSize > 0) {
        unsigned char* c = reinterpret_cast<unsigned char*>(tmpVec.data());
        uint32_t length = sizeof(T) * vectorSize;
        core::mutex::DataSync.lock();
        _currentStorage->insert(_currentStorage->end(), c, c + length);
        core::mutex::DataSync.unlock();
    }
}

template<class T>
void SharedData::readVector(SharedVector<T>& vector) {
    core::mutex::DataSync.lock();

    uint32_t size = *reinterpret_cast<uint32_t*>(&_dataBlock[_pos]);
    _pos += sizeof(uint32_t);

    core::mutex::DataSync.unlock();

    if (size == 0) {
        vector.clear();
        return;
    }

    uint32_t totalSize = size * sizeof(T);

    core::mutex::DataSync.lock();

    unsigned char* c = &_dataBlock[_pos];
    _pos += totalSize;

    core::mutex::DataSync.unlock();

    std::vector<T> tmpVec;
    tmpVec.insert(
        tmpVec.begin(),
        reinterpret_cast<T*>(c),
        reinterpret_cast<T*>(c) + size
    );

    vector.setVal(std::move(tmpVec));
}

} // namespace sgct

#endif // __SGCT__SHAREDDATA__H__
