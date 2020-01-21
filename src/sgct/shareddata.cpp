/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/shareddata.h>

#include <sgct/log.h>
#include <sgct/profiling.h>
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

    // fill rest of header with Network::DefaultId
    std::memset(_headerSpace.data(), Network::DefaultId, Network::HeaderSize);
    _headerSpace[0] = Network::DataId;
}

void SharedData::setEncodeFunction(std::function<std::vector<unsigned char>()> function) {
    _encodeFn = std::move(function);
}

void SharedData::setDecodeFunction(
                          std::function<void(const std::vector<unsigned char>&)> function)
{
    _decodeFn = std::move(function);
}

void SharedData::decode(const char* receivedData, int receivedLength) {
    ZoneScoped

    {
        std::unique_lock lk(mutex::DataSync);

        // reset
        _pos = 0;
        _dataBlock.clear();

        if (receivedLength > static_cast<int>(_dataBlock.capacity())) {
            _dataBlock.reserve(receivedLength);
        }
        _dataBlock.insert(_dataBlock.end(), receivedData, receivedData + receivedLength);
    }

    if (_decodeFn) {
        std::vector<unsigned char> data;
        data.assign(receivedData, receivedData + receivedLength);
        _decodeFn(data);
    }
}

void SharedData::encode() {
    ZoneScoped

    {
        std::unique_lock lk(mutex::DataSync);
        _dataBlock.clear();
        _headerSpace[0] = Network::DataId;

        // reserve header space
        _dataBlock.insert(
            _dataBlock.begin(),
            _headerSpace.cbegin(),
            _headerSpace.cbegin() + Network::HeaderSize
        );
    }

    if (_encodeFn) {
        std::vector<unsigned char> data = _encodeFn();
        _dataBlock.insert(_dataBlock.end(), data.begin(), data.end());
    }
}

unsigned char* SharedData::dataBlock() {
    return _dataBlock.data();
}

int SharedData::dataSize() {
    return static_cast<int>(_dataBlock.size());
}

int SharedData::bufferSize() {
    return static_cast<int>(_dataBlock.capacity());
}

template <>
void serializeObject(std::vector<unsigned char>& buffer, const std::string& value) {
    uint32_t length = static_cast<uint32_t>(value.size());
    unsigned char* p = reinterpret_cast<unsigned char*>(&length);

    buffer.insert(buffer.end(), p, p + sizeof(uint32_t));
    buffer.insert(buffer.end(), value.data(), value.data() + length);
}

template <>
void serializeObject(std::vector<unsigned char>& buffer, const std::wstring& value) {
    uint32_t length = static_cast<uint32_t>(value.size());
    unsigned char* p = reinterpret_cast<unsigned char*>(&length);
    const unsigned char* ws = reinterpret_cast<const unsigned char*>(&value[0]);

    buffer.insert(buffer.end(), p, p + sizeof(uint32_t));
    buffer.insert(buffer.end(), ws, ws + length * sizeof(wchar_t));
}

template <>
void deserializeObject(const std::vector<unsigned char>& buffer, unsigned int& pos,
                       std::string& value)
{
    uint32_t size;
    deserializeObject<uint32_t>(buffer, pos, size);

    value = std::string(buffer.begin() + pos, buffer.begin() + pos + size);
    pos += size * sizeof(std::string::value_type);
}

template <>
void deserializeObject(const std::vector<unsigned char>& buffer, unsigned int& pos,
                       std::wstring& value)
{
    uint32_t size;
    deserializeObject<uint32_t>(buffer, pos, size);

    value = std::wstring(buffer.begin() + pos, buffer.begin() + pos + size);
    pos += size * sizeof(std::wstring::value_type);
}

} // namespace sgct
