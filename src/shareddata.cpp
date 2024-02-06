/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
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
    constexpr int DefaultSize = 1024;

    _dataBlock.reserve(DefaultSize);

    // fill rest of header with Network::DefaultId
    std::memset(_headerSpace.data(), Network::DefaultId, Network::HeaderSize);
    _headerSpace[0] = std::byte { Network::DataId };
}

void SharedData::setEncodeFunction(std::function<std::vector<std::byte>()> function) {
    _encodeFn = std::move(function);
}

void SharedData::setDecodeFunction(
                              std::function<void(const std::vector<std::byte>&)> function)
{
    _decodeFn = std::move(function);
}

void SharedData::decode(const char* receivedData, int receivedLength) {
    ZoneScoped;

    {
        std::unique_lock lk(mutex::DataSync);

        // reset
        _dataBlock.clear();

        if (receivedLength > static_cast<int>(_dataBlock.capacity())) {
            _dataBlock.reserve(receivedLength);
        }
        _dataBlock.insert(
            _dataBlock.end(),
            reinterpret_cast<const std::byte*>(receivedData),
            reinterpret_cast<const std::byte*>(receivedData) + receivedLength
        );
    }

    if (_decodeFn) {
        std::vector<std::byte> data;
        data.assign(
            reinterpret_cast<const std::byte*>(receivedData),
            reinterpret_cast<const std::byte*>(receivedData) + receivedLength
        );
        _decodeFn(data);
    }
}

void SharedData::encode() {
    ZoneScoped;

    {
        std::unique_lock lk(mutex::DataSync);
        _dataBlock.clear();

        _dataBlock.insert(
            _dataBlock.begin(),
            _headerSpace.cbegin(),
            _headerSpace.cbegin() + Network::HeaderSize
        );
    }

    if (_encodeFn) {
        std::vector<std::byte> data = _encodeFn();
        _dataBlock.insert(_dataBlock.end(), data.begin(), data.end());
    }
}

unsigned char* SharedData::dataBlock() {
    return reinterpret_cast<unsigned char*>(_dataBlock.data());
}

int SharedData::dataSize() {
    return static_cast<int>(_dataBlock.size());
}

int SharedData::bufferSize() {
    return static_cast<int>(_dataBlock.capacity());
}

template <>
void serializeObject(std::vector<std::byte>& buffer, std::string_view value) {
    uint32_t length = static_cast<uint32_t>(value.size());
    std::byte* p = reinterpret_cast<std::byte*>(&length);

    buffer.insert(buffer.end(), p, p + sizeof(uint32_t));
    buffer.insert(
        buffer.end(),
        reinterpret_cast<const std::byte*>(value.data()),
        reinterpret_cast<const std::byte*>(value.data() + length)
    );
}

void serializeObject(std::vector<std::byte>& buffer, const std::string& value) {
    uint32_t length = static_cast<uint32_t>(value.size());
    std::byte* p = reinterpret_cast<std::byte*>(&length);

    buffer.insert(buffer.end(), p, p + sizeof(uint32_t));
    buffer.insert(
        buffer.end(),
        reinterpret_cast<const std::byte*>(value.data()),
        reinterpret_cast<const std::byte*>(value.data() + length)
    );
}

void serializeObject(std::vector<std::byte>& buffer, const std::wstring& value) {
    uint32_t length = static_cast<uint32_t>(value.size());
    std::byte* p = reinterpret_cast<std::byte*>(&length);
    const std::byte* ws = reinterpret_cast<const std::byte*>(&value[0]);

    buffer.insert(buffer.end(), p, p + sizeof(uint32_t));
    buffer.insert(buffer.end(), ws, ws + length * sizeof(wchar_t));
}

template <>
void deserializeObject(const std::vector<std::byte>& buffer, unsigned int& pos,
                       std::string& value)
{
    uint32_t size;
    deserializeObject<uint32_t>(buffer, pos, size);

    value = std::string(
        reinterpret_cast<const char*>(buffer.data() + pos),
        reinterpret_cast<const char*>(buffer.data() + pos + size)
    );
    pos += size * sizeof(std::string::value_type);
}

template <>
void deserializeObject(const std::vector<std::byte>& buffer, unsigned int& pos,
                       std::wstring& value)
{
    uint32_t size;
    deserializeObject<uint32_t>(buffer, pos, size);

    value = std::wstring(
        reinterpret_cast<const char*>(buffer.data() + pos),
        reinterpret_cast<const char*>(buffer.data() + pos + size)
    );
    pos += size * sizeof(std::wstring::value_type);
}

} // namespace sgct
