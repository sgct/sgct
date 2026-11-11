/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/shareddata.h>

#include <sgct/log.h>
#include <sgct/mutexes.h>
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

    // Fill rest of header with Network::DefaultId
    std::memset(_headerSpace.data(), Network::DefaultId, Network::HeaderSize);
    _headerSpace[0] = static_cast<std::byte>(Network::DataId);
}

void SharedData::setEncodeFunction(std::function<std::vector<std::byte>()> function) {
    _encodeFn = std::move(function);
}

void SharedData::setDecodeFunction(
                              std::function<void(const std::vector<std::byte>&)> function)
{
    _decodeFn = std::move(function);
}

void SharedData::storeReceivedData(const char* receivedData, int receivedLength) {
    ZoneScoped;

    const std::byte* begin = reinterpret_cast<const std::byte*>(receivedData);
    const std::unique_lock lock(_receivedDataMutex);

    // Reusing a buffer of a previous frame avoids allocating memory in every frame
    std::vector<std::byte> buffer;
    if (!_unusedBuffers.empty()) {
        buffer = std::move(_unusedBuffers.back());
        _unusedBuffers.pop_back();
    }
    buffer.assign(begin, begin + receivedLength);
    _receivedData.push_back(std::move(buffer));
}

void SharedData::decodeReceivedData() {
    ZoneScoped;

    // The list is a member variable to keep its memory between the frames. It is only
    // used by this function and is always empty outside of it, unless a decode function
    // threw an exception
    _decodingData.clear();
    {
        const std::unique_lock lock(_receivedDataMutex);
        _decodingData.swap(_receivedData);
    }

    if (_decodeFn) {
        for (const std::vector<std::byte>& d : _decodingData) {
            _decodeFn(d);
        }
    }

    // Keep the buffers for the data of the next frames. The number of buffers is limited
    // as more of them are only needed if this node falls behind the server
    constexpr size_t MaxUnusedBuffers = 4;
    {
        const std::unique_lock lock(_receivedDataMutex);
        for (std::vector<std::byte>& d : _decodingData) {
            if (_unusedBuffers.size() >= MaxUnusedBuffers) {
                break;
            }
            _unusedBuffers.push_back(std::move(d));
        }
    }
    _decodingData.clear();
}

void SharedData::encode() {
    ZoneScoped;

    {
        const std::unique_lock lock(mutex::DataSync);
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
    const std::byte* ws = reinterpret_cast<const std::byte*>(value.data());

    buffer.insert(buffer.end(), p, p + sizeof(uint32_t));
    buffer.insert(buffer.end(), ws, ws + length * sizeof(wchar_t));
}

template <>
void deserializeObject(const std::vector<std::byte>& buffer, unsigned int& pos,
                       std::string& value)
{
    uint32_t size = 0;
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
    uint32_t size = 0;
    deserializeObject<uint32_t>(buffer, pos, size);

    value.resize(size);
    std::memcpy(
        value.data(),
        buffer.data() + pos,
        size * sizeof(std::wstring::value_type)
    );
    pos += size * sizeof(std::wstring::value_type);
}

} // namespace sgct
