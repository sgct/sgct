/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__SHAREDDATATYPES__H__
#define __SGCT__SHAREDDATATYPES__H__

#include <mutex>
#include <string>
#include <vector>

namespace sgct {

/// Mutex protected template for multi-thread data sharing
template <class T>
class SharedObject {
public:
    SharedObject() = default;
    explicit SharedObject(T val) : _value(std::move(val)) {}

    T value() const {
        _mutex.lock();
        T tmpT = _value;
        _mutex.unlock();
        return tmpT;
    }

    void setValue(T val) {
        _mutex.lock();
        _value = std::move(val);
        _mutex.unlock();
    }

    void operator=(const SharedObject<T>& so) {
        _mutex.lock();
        _value = so._value;
        _mutex.unlock();
    }

    void operator=(T val) {
        _mutex.lock();
        _value = std::move(val);
        _mutex.unlock();
    }

private:
    T _value;
    mutable std::mutex _mutex;
};

using SharedFloat = SharedObject<float>;
using SharedDouble = SharedObject<double>;
using SharedInt64 = SharedObject<int64_t>;
using SharedUInt64 = SharedObject<uint64_t>;
using SharedInt32 = SharedObject<int32_t>;
using SharedUInt32 = SharedObject<uint32_t>;
using SharedInt16 = SharedObject<int16_t>;
using SharedUInt16 = SharedObject<uint16_t>;
using SharedInt8 = SharedObject<int8_t>;
using SharedUInt8 = SharedObject<uint8_t>;
using SharedUChar = SharedObject<unsigned char>;
using SharedBool = SharedObject<bool>;
using SharedString = SharedObject<std::string>;
using SharedWString = SharedObject<std::wstring>;

extern template class SharedObject<float>;
extern template class SharedObject<double>;
extern template class SharedObject<int64_t>;
extern template class SharedObject<uint64_t>;
extern template class SharedObject<int32_t>;
extern template class SharedObject<uint32_t>;
extern template class SharedObject<int16_t>;
extern template class SharedObject<uint16_t>;
extern template class SharedObject<int8_t>;
extern template class SharedObject<bool>;
extern template class SharedObject<std::string>;
extern template class SharedObject<std::wstring>;

/// Mutex protected std::vector template for multi-thread data sharing
template <class T>
class SharedVector {
public:
    SharedVector() = default;
    explicit SharedVector(size_t size) {
        mVector.reserve(size);
    }

    T valueAt(size_t index) const {
        std::unique_lock lock(_mutex);
        return mVector[index];
    }

    std::vector<T> value() const {
        std::unique_lock lock(_mutex);
        return mVector;
    }

    void setValueAt(size_t index, T val) {
        std::unique_lock lock(_mutex);
        mVector[index] = val;
    }

    void addValue(T val) {
        std::unique_lock lock(_mutex);
        mVector.push_back(val);
    }

    void setValue(std::vector<T> mCopy) {
        std::unique_lock lock(_mutex);
        mVector = std::move(mCopy);
    }

    void clear() {
        std::unique_lock lock(_mutex);
        mVector.clear();
    }

    size_t size() const {
        std::unique_lock lock(_mutex);
        return mVector.size();
    }

private:
    std::vector<T> mVector;
    mutable std::mutex _mutex;
};

} // namespace sgct

#endif // __SGCT__SHAREDDATATYPES__H__
