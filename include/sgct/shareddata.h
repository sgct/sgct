/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SHAREDDATA__H__
#define __SGCT__SHAREDDATA__H__

#include <sgct/mutexes.h>
#include <sgct/network.h>
#include <array>
#include <string>
#include <vector>

namespace sgct {

/**
 * This class shares application data between nodes in a cluster where the master encodes
 * and transmits the data and the clients receives and decode the data.
 */
class SharedData {
public:
    static SharedData& instance();
    static void destroy();

    void setEncodeFunction(std::function<std::vector<unsigned char>()> function);
    void setDecodeFunction(
        std::function<void(const std::vector<unsigned char>&)> function);

    /// This fuction is called internally by SGCT and shouldn't be used by the user.
    void encode();

    /// This function is called internally by SGCT and shouldn't be used by the user.
    void decode(const char* receivedData, int receivedLength);

    unsigned char* dataBlock();
    int dataSize();
    int bufferSize();

private:
    SharedData();

    // function pointers
    std::function<std::vector<unsigned char>()> _encodeFn;
    std::function<void(const std::vector<unsigned char>&)> _decodeFn;

    static SharedData* _instance;
    std::vector<unsigned char> _dataBlock;
    std::array<unsigned char, Network::HeaderSize> _headerSpace;
    unsigned int _pos = 0;
};

template <typename T>
void serializeObject(std::vector<unsigned char>& buffer, const T& value) {
    static_assert(std::is_pod_v<T>, "Type has to be a plain-old data type");
   
    buffer.insert(
        buffer.end(),
        reinterpret_cast<const unsigned char*>(&value),
        reinterpret_cast<const unsigned char*>(&value) + sizeof(T)
    );
}

template <typename T>
void serializeObject(std::vector<unsigned char>& buffer, const std::vector<T>& value) {
    static_assert(std::is_pod_v<T>, "Type has to be a plain-old data type");

    const uint32_t size = static_cast<uint32_t>(value.size());
    serializeObject(buffer, size);

    if (size > 0) {
        buffer.insert(
            buffer.end(),
            reinterpret_cast<const unsigned char*>(value.data()),
            reinterpret_cast<const unsigned char*>(value.data()) + size * sizeof(T)
        );
    }
}

template <>
void serializeObject(std::vector<unsigned char>& buffer, const std::string& value);


template <>
void serializeObject(std::vector<unsigned char>& buffer, const std::wstring& value);


template <typename T>
void deserializeObject(const std::vector<unsigned char>& buffer, unsigned int& pos,
                       T& value)
{
    static_assert(std::is_pod_v<T>, "Type has to be a plain-old data type");

    value = *reinterpret_cast<const T*>(&buffer[pos]);
    pos += sizeof(T);
}

template <typename T>
void deserializeObject(const std::vector<unsigned char>& buffer, unsigned int& pos,
                       std::vector<T>& value)
{
    value.clear();

    uint32_t size;
    deserializeObject<uint32_t>(buffer, pos, size);

    if (size == 0) {
        return;
    }

    value.clear();
    value.assign(&buffer[pos], &buffer[pos] + size * sizeof(T));
}

template <>
void deserializeObject(const std::vector<unsigned char>& buffer,
    unsigned int& pos, std::string& value);

template <>
void deserializeObject(const std::vector<unsigned char>& buffer,
    unsigned int& pos, std::wstring& value);

} // namespace sgct

#endif // __SGCT__SHAREDDATA__H__
