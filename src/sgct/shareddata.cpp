/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#include <sgct/shareddata.h>

#include <sgct/logger.h>
#include <zlib.h>
#include <cstring>
#include <string>

namespace sgct {

SharedData* SharedData::_instance = nullptr;

SharedData& SharedData::instance() {
    if (!_instance) {
        _instance = new SharedData;
    }
    return *_instance;
}

void SharedData::destroy() {
    delete _instance;
    _instance = nullptr;
}

SharedData::SharedData() {
    constexpr const int DefaultSize = 1024;

    _dataBlock.reserve(DefaultSize);
    _dataBlockToCompress.reserve(DefaultSize);

    // fill rest of header with Network::DefaultId
    std::memset(_headerSpace.data(), core::Network::DefaultId, core::Network::HeaderSize);
    _headerSpace[0] = core::Network::DataId;
}

void SharedData::setEncodeFunction(std::function<void()> fn) {
    _encodeFn = std::move(fn);
}

void SharedData::setDecodeFunction(std::function<void()> fn) {
    _decodeFn = std::move(fn);
}

void SharedData::decode(const char* receivedData, int receivedLength, int) {
    {
        std::unique_lock lk(core::mutex::DataSync);

        // reset
        _pos = 0;
        _dataBlock.clear();

        if (receivedLength > static_cast<int>(_dataBlock.capacity())) {
            _dataBlock.reserve(receivedLength);
        }
        _dataBlock.insert(_dataBlock.end(), receivedData, receivedData + receivedLength);
    }

    if (_decodeFn) {
        _decodeFn();
    }
}

void SharedData::encode() {
    {
        std::unique_lock lk(core::mutex::DataSync);
        _dataBlock.clear();
        _headerSpace[0] = core::Network::DataId;

        // reserve header space
        _dataBlock.insert(
            _dataBlock.begin(),
            _headerSpace.begin(),
            _headerSpace.begin() + core::Network::HeaderSize
        );
    }

    if (_encodeFn) {
        _encodeFn();
    }
}

std::size_t SharedData::getUserDataSize() {
    return _dataBlock.size() - core::Network::HeaderSize;
}

unsigned char* SharedData::getDataBlock() {
    return _dataBlock.data();
}

size_t SharedData::getDataSize() {
    return _dataBlock.size();
}

size_t SharedData::getBufferSize() {
    return _dataBlock.capacity();
}

void SharedData::writeFloat(const SharedFloat& sf) {
    float val = sf.getVal();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(float));
}

void SharedData::writeDouble(const SharedDouble& sd) {
    double val = sd.getVal();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(double));
}

void SharedData::writeInt64(const SharedInt64& si) {
    int64_t val = si.getVal();
    unsigned char* p = reinterpret_cast<unsigned char* >(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(int64_t));
}

void SharedData::writeInt32(const SharedInt32& si) {
    int32_t val = si.getVal();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(int32_t));
}

void SharedData::writeInt16(const SharedInt16& si) {
    int16_t val = si.getVal();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(int16_t));
}

void SharedData::writeInt8(const SharedInt8& si) {
    int8_t val = si.getVal();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(int8_t));
}

void SharedData::writeUInt64(const SharedUInt64& si) {
    uint64_t val = si.getVal();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(uint64_t));
}

void SharedData::writeUInt32(const SharedUInt32& si) {
    uint32_t val = si.getVal();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(uint32_t));
}

void SharedData::writeUInt16(const SharedUInt16& si) {
    uint16_t val = si.getVal();
    unsigned char* p = reinterpret_cast<unsigned char*>(&val);
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(uint16_t));
}

void SharedData::writeUInt8(const SharedUInt8& si) {
    uint8_t val = si.getVal();
    unsigned char* p = &val;
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(uint8_t));
}

void SharedData::writeUChar(const SharedUChar& suc) {
    unsigned char val = suc.getVal();
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.push_back(val);
}

void SharedData::writeBool(const SharedBool& sb) {
    bool val = sb.getVal();
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.push_back(val ? 1 : 0);
}

void SharedData::writeString(const SharedString& ss) {
    std::string tmpStr = ss.getVal();
    uint32_t length = static_cast<uint32_t>(tmpStr.size());
    unsigned char* p = reinterpret_cast<unsigned char*>(&length);
    
    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(uint32_t));
    _dataBlock.insert(_dataBlock.end(), tmpStr.data(), tmpStr.data() + length);
}

void SharedData::writeWString(const SharedWString& ss) {
    std::wstring tmpStr = ss.getVal();
    uint32_t length = static_cast<uint32_t>(tmpStr.size());
    unsigned char* p = reinterpret_cast<unsigned char*>(&length);
    unsigned char* ws = reinterpret_cast<unsigned char*>(&tmpStr[0]);

    std::unique_lock lk(core::mutex::DataSync);
    _dataBlock.insert(_dataBlock.end(), p, p + sizeof(uint32_t));
    _dataBlock.insert(_dataBlock.end(), ws, ws + length * sizeof(wchar_t));
}

void SharedData::readFloat(SharedFloat& sf) {
    core::mutex::DataSync.lock();
    float val = *reinterpret_cast<float*>(&_dataBlock[_pos]);
    _pos += sizeof(float);
    core::mutex::DataSync.unlock();

    sf.setVal(val);
}

void SharedData::readDouble(SharedDouble& sd) {
    core::mutex::DataSync.lock();
    double val = *reinterpret_cast<double*>(&_dataBlock[_pos]);
    _pos += sizeof(double);
    core::mutex::DataSync.unlock();

    sd.setVal(val);
}

void SharedData::readInt64(SharedInt64& si) {
    core::mutex::DataSync.lock();
    int64_t val = *reinterpret_cast<int64_t*>(&_dataBlock[_pos]);
    _pos += sizeof(int64_t);
    core::mutex::DataSync.unlock();

    si.setVal(val);
}

void SharedData::readInt32(SharedInt32& si) {
    core::mutex::DataSync.lock();
    int32_t val = *reinterpret_cast<int32_t*>(&_dataBlock[_pos]);
    _pos += sizeof(int32_t);
    core::mutex::DataSync.unlock();

    si.setVal(val);
}

void SharedData::readInt16(SharedInt16& si) {
    core::mutex::DataSync.lock();
    int16_t val = *reinterpret_cast<int16_t*>(&_dataBlock[_pos]);
    _pos += sizeof(int16_t);
    core::mutex::DataSync.unlock();

    si.setVal(val);
}

void SharedData::readInt8(SharedInt8& si) {
    core::mutex::DataSync.lock();
    int8_t val = *reinterpret_cast<int8_t*>(&_dataBlock[_pos]);
    _pos += sizeof(int8_t);
    core::mutex::DataSync.unlock();

    si.setVal(val);
}

void SharedData::readUInt64(SharedUInt64& si) {
    core::mutex::DataSync.lock();
    uint64_t val = *reinterpret_cast<uint64_t*>(&_dataBlock[_pos]);
    _pos += sizeof(uint64_t);
    core::mutex::DataSync.unlock();

    si.setVal(val);
}

void SharedData::readUInt32(SharedUInt32& si) {
    core::mutex::DataSync.lock();
    uint32_t val = *reinterpret_cast<uint32_t*>(&_dataBlock[_pos]);
    _pos += sizeof(uint32_t);
    core::mutex::DataSync.unlock();

    si.setVal(val);
}

void SharedData::readUInt16(SharedUInt16& si) {
    core::mutex::DataSync.lock();
    uint16_t val = *reinterpret_cast<uint16_t*>(&_dataBlock[_pos]);
    _pos += sizeof(uint16_t);
    core::mutex::DataSync.unlock();

    si.setVal(val);
}

void SharedData::readUInt8(SharedUInt8& si) {
    core::mutex::DataSync.lock();
    uint8_t val = _dataBlock[_pos];
    _pos += sizeof(uint8_t);
    core::mutex::DataSync.unlock();

    si.setVal(val);
}

void SharedData::readUChar(SharedUChar& suc) {
    core::mutex::DataSync.lock();
    unsigned char c = _dataBlock[_pos];
    _pos += sizeof(unsigned char);
    core::mutex::DataSync.unlock();

    suc.setVal(c);
}

void SharedData::readBool(SharedBool& sb) {
    core::mutex::DataSync.lock();
    bool b = _dataBlock[_pos] == 1;
    _pos += 1;
    core::mutex::DataSync.unlock();

    sb.setVal(b);
}

void SharedData::readString(SharedString& ss) {
    core::mutex::DataSync.lock();
    
    uint32_t length = *reinterpret_cast<uint32_t*>(&_dataBlock[_pos]);
    _pos += sizeof(uint32_t);

    if (length == 0) {
        ss.setVal("");
        return;
    }

    std::string stringData(_dataBlock.begin() + _pos, _dataBlock.begin() + _pos + length);
    _pos += length;
    core::mutex::DataSync.unlock();
    
    ss.setVal(std::move(stringData));
}

void SharedData::readWString(SharedWString& ss) {
    core::mutex::DataSync.lock();

    uint32_t length = *(reinterpret_cast<uint32_t*>(&_dataBlock[_pos]));
    _pos += sizeof(uint32_t);

    if (length == 0) {
        ss.setVal(std::wstring());
        return;
    }

    std::wstring stringData(_dataBlock.begin() + _pos, _dataBlock.begin() + _pos + length);
    _pos += length * sizeof(wchar_t);
    core::mutex::DataSync.unlock();

    ss.setVal(std::move(stringData));
}

} // namespace sgct
