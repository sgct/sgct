/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2023                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SHAREDDATA__H__
#define __SGCT__SHAREDDATA__H__

#include <sgct/sgctexports.h>
#include <sgct/mutexes.h>
#include <sgct/network.h>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace sgct {

/**
 * This class shares application data between nodes in a cluster where the master encodes
 * and transmits the data and the clients receives and decode the data.
 */
class SGCT_EXPORT SharedData {
public:
    static SharedData& instance();
    static void destroy();

    void setEncodeFunction(std::function<std::vector<std::byte>()> function);
    void setDecodeFunction(std::function<void(const std::vector<std::byte>&)> function);

    /**
     * This fuction is called internally by SGCT and shouldn't be used by the user.
     */
    void encode();

    /**
     * This function is called internally by SGCT and shouldn't be used by the user.
     */
    void decode(const char* receivedData, int receivedLength);

    unsigned char* dataBlock();
    int dataSize();
    int bufferSize();

private:
    SharedData();

    std::function<std::vector<std::byte>()> _encodeFn;
    std::function<void(const std::vector<std::byte>&)> _decodeFn;

    static SharedData* _instance;
    std::vector<std::byte> _dataBlock;
    std::array<std::byte, Network::HeaderSize> _headerSpace;
};

template <typename T>
void serializeObject(std::vector<std::byte>& buffer, T value) {
    static_assert(
        std::is_standard_layout_v<T> && std::is_trivial_v<T>,
        "Type has to be a plain-old data type"
    );

    buffer.insert(
        buffer.end(),
        reinterpret_cast<const std::byte*>(&value),
        reinterpret_cast<const std::byte*>(&value) + sizeof(T)
    );
}

template <typename T>
void serializeObject(std::vector<std::byte>& buffer, const std::vector<T>& value) {
    static_assert(
        std::is_standard_layout_v<T> && std::is_trivial_v<T>,
        "Type has to be a plain-old data type"
    );

    const uint32_t size = static_cast<uint32_t>(value.size());
    serializeObject(buffer, size);

    if (size > 0) {
        buffer.insert(
            buffer.end(),
            reinterpret_cast<const std::byte*>(value.data()),
            reinterpret_cast<const std::byte*>(value.data()) + size * sizeof(T)
        );
    }
}

template <>
SGCT_EXPORT void serializeObject(std::vector<std::byte>& buffer, std::string_view value);

SGCT_EXPORT void serializeObject(std::vector<std::byte>& buffer, const std::string& value);
SGCT_EXPORT void serializeObject(std::vector<std::byte>& buffer, const std::wstring& value);

template <typename T>
void deserializeObject(const std::vector<std::byte>& buffer, unsigned int& pos,
                       T& value)
{
    static_assert(
        std::is_standard_layout_v<T> && std::is_trivial_v<T>,
        "Type has to be a plain-old data type"
    );

    value = *reinterpret_cast<const T*>(&buffer[pos]);
    pos += sizeof(T);
}

template <typename T>
void deserializeObject(const std::vector<std::byte>& buffer, unsigned int& pos,
                       std::vector<T>& value)
{
    value.clear();

    uint32_t size;
    deserializeObject<uint32_t>(buffer, pos, size);

    if (size == 0) {
        return;
    }

    value.clear();
    value.assign(
        reinterpret_cast<const T*>(buffer.data() + pos),
        reinterpret_cast<const T*>(buffer.data() + pos + size * sizeof(T))
    );
    pos += size * sizeof(T);
}

template <>
SGCT_EXPORT void deserializeObject(const std::vector<std::byte>& buffer, unsigned int& pos,
    std::string& value);

template <>
SGCT_EXPORT void deserializeObject(const std::vector<std::byte>& buffer, unsigned int& pos,
    std::wstring& value);

} // namespace sgct

#endif // __SGCT__SHAREDDATA__H__
