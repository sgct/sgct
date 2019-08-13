/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__SHARED_DATA_TYPES__H__
#define __SGCT__SHARED_DATA_TYPES__H__

#include <mutex>
#include <string>
#include <vector>

namespace sgct {    

/*!
Mutex protected float for multi-thread data sharing
*/
class SharedFloat {
public:
    SharedFloat() = default;
    explicit SharedFloat(float val);
    SharedFloat(const SharedFloat& sf);

    float getVal();
    void setVal(float val);

    void operator=(const SharedFloat& sf);
    void operator=(float val);
    void operator+=(float val);
    void operator-=(float val);
    void operator*=(float val);
    void operator/=(float val);
    void operator++(int);
    void operator--(int);

    bool operator<(float val);
    bool operator<=(float val);
    bool operator>(float val);
    bool operator>=(float val);
    bool operator==(float val);
    bool operator!=(float val);

    float operator+(float val);
    float operator-(float val);
    float operator*(float val);
    float operator/(float val);

private:
    float mVal = 0.f;
    std::mutex mMutex;
};

/*!
Mutex protected double for multi-thread data sharing
*/
class SharedDouble {
public:
    SharedDouble() = default;
    explicit SharedDouble(double val );
    SharedDouble(const SharedDouble& sd );
        
    double getVal();
    void setVal(double val);
        
    void operator=(const SharedDouble& sd);
    void operator=(double val);
    void operator+=(double val);
    void operator-=(double val);
    void operator*=(double val);
    void operator/=(double val);
    void operator++(int);
    void operator--(int);
        
    bool operator<(double val);
    bool operator<=(double val);
    bool operator>(double val);
    bool operator>=(double val);
    bool operator==(double val);
    bool operator!=(double val);
        
    double operator+(double val);
    double operator-(double val);
    double operator*(double val);
    double operator/(double val);
        
private:
    double mVal = 0.0;
    std::mutex mMutex;
};

/*!
Mutex protected long for multi-thread data sharing
*/
class SharedInt64 {
public:
    SharedInt64() = default;
    explicit SharedInt64(int64_t val);
    SharedInt64(const SharedInt64& si);

    int64_t getVal();
    void setVal(int64_t val);

    void operator=(const SharedInt64& si);
    void operator=(int64_t val);
    void operator+=(int64_t val);
    void operator-=(int64_t val);
    void operator*=(int64_t val);
    void operator/=(int64_t val);
    void operator++(int);
    void operator--(int);

    bool operator<(int64_t val);
    bool operator<=(int64_t val);
    bool operator>(int64_t val);
    bool operator>=(int64_t val);
    bool operator==(int64_t val);
    bool operator!=(int64_t val);

    int64_t operator+(int64_t val);
    int64_t operator-(int64_t val);
    int64_t operator*(int64_t val);
    int64_t operator/(int64_t val);

private:
    int64_t mVal = 0;
    std::mutex mMutex;
};

/*!
Mutex protected int for multi-thread data sharing
*/
class SharedInt32 {
public:
    SharedInt32() = default;
    explicit SharedInt32(int32_t val);
    SharedInt32(const SharedInt32& si);

    int32_t getVal();
    void setVal(int32_t val);

    void operator=(const SharedInt32& si);
    void operator=(int32_t val);
    void operator+=(int32_t val);
    void operator-=(int32_t val);
    void operator*=(int32_t val);
    void operator/=(int32_t val);
    void operator++(int);
    void operator--(int);

    bool operator<(int32_t val);
    bool operator<=(int32_t val);
    bool operator>(int32_t val);
    bool operator>=(int32_t val);
    bool operator==(int32_t val);
    bool operator!=(int32_t val);

    int32_t operator+(int32_t val);
    int32_t operator-(int32_t val);
    int32_t operator*(int32_t val);
    int32_t operator/(int32_t val);

private:
    int32_t mVal = 0;
    std::mutex mMutex;
};

/*!
Mutex protected short/int16 for multi-thread data sharing
*/
class SharedInt16 {
public:
    SharedInt16() = default;
    explicit SharedInt16(int16_t val);
    SharedInt16(const SharedInt16& si);

    int16_t getVal();
    void setVal(int16_t val);

    void operator=(const SharedInt16& si);
    void operator=(int16_t val);
    void operator+=(int16_t val);
    void operator-=(int16_t val);
    void operator*=(int16_t val);
    void operator/=(int16_t val);
    void operator++(int);
    void operator--(int);

    bool operator<(int16_t val);
    bool operator<=(int16_t val);
    bool operator>(int16_t val);
    bool operator>=(int16_t val);
    bool operator==(int16_t val);
    bool operator!=(int16_t val);

    int16_t operator+(int16_t val);
    int16_t operator-(int16_t val);
    int16_t operator*(int16_t val);
    int16_t operator/(int16_t val);

private:
    int16_t mVal = 0;
    std::mutex mMutex;
};

/*!
Mutex protected int8 for multi-thread data sharing
*/
class SharedInt8 {
public:
    SharedInt8() = default;
    explicit SharedInt8(int8_t val);
    SharedInt8(const SharedInt8& si);

    int8_t getVal();
    void setVal(int8_t val);

    void operator=(const SharedInt8& si);
    void operator=(int8_t val);
    void operator+=(int8_t val);
    void operator-=(int8_t val);
    void operator*=(int8_t val);
    void operator/=(int8_t val);
    void operator++(int);
    void operator--(int);

    bool operator<(int8_t val);
    bool operator<=(int8_t val);
    bool operator>(int8_t val);
    bool operator>=(int8_t val);
    bool operator==(int8_t val);
    bool operator!=(int8_t val);

    int8_t operator+(int8_t val);
    int8_t operator-(int8_t val);
    int8_t operator*(int8_t val);
    int8_t operator/(int8_t val);

private:
    int8_t mVal = 0;
    std::mutex mMutex;
};

/*!
Mutex protected unsigned long for multi-thread data sharing
*/
class SharedUInt64 {
public:
    SharedUInt64() = default;
    explicit SharedUInt64(uint64_t val);
    SharedUInt64(const SharedUInt64& si);

    uint64_t getVal();
    void setVal(uint64_t val);

    void operator=(const SharedUInt64& si);
    void operator=(uint64_t val);
    void operator+=(uint64_t val);
    void operator-=(uint64_t val);
    void operator*=(uint64_t val);
    void operator/=(uint64_t val);
    void operator++(int);
    void operator--(int);

    bool operator<(uint64_t val);
    bool operator<=(uint64_t val);
    bool operator>(uint64_t val);
    bool operator>=(uint64_t val);
    bool operator==(uint64_t val);
    bool operator!=(uint64_t val);

    uint64_t operator+(uint64_t val);
    uint64_t operator-(uint64_t val);
    uint64_t operator*(uint64_t val);
    uint64_t operator/(uint64_t val);

private:
    uint64_t mVal = 0;
    std::mutex mMutex;
};

/*!
Mutex protected unsigned int for multi-thread data sharing
*/
class SharedUInt32 {
public:
    SharedUInt32() = default;
    explicit SharedUInt32(uint32_t val);
    SharedUInt32(const SharedUInt32& si);

    uint32_t getVal();
    void setVal(uint32_t val);

    void operator=(const SharedUInt32& si);
    void operator=(uint32_t val);
    void operator+=(uint32_t val);
    void operator-=(uint32_t val);
    void operator*=(uint32_t val);
    void operator/=(uint32_t val);
    void operator++(int);
    void operator--(int);

    bool operator<(uint32_t val);
    bool operator<=(uint32_t val);
    bool operator>(uint32_t val);
    bool operator>=(uint32_t val);
    bool operator==(uint32_t val);
    bool operator!=(uint32_t val);

    uint32_t operator+(uint32_t val);
    uint32_t operator-(uint32_t val);
    uint32_t operator*(uint32_t val);
    uint32_t operator/(uint32_t val);

private:
    uint32_t mVal = 0;
    std::mutex mMutex;
};

/*!
Mutex protected unsigned short/uint16 for multi-thread data sharing
*/
class SharedUInt16 {
public:
    SharedUInt16() = default;
    explicit SharedUInt16(uint16_t val);
    SharedUInt16(const SharedUInt16& si);

    uint16_t getVal();
    void setVal(uint16_t val);

    void operator=(const SharedUInt16& si);
    void operator=(uint16_t val);
    void operator+=(uint16_t val);
    void operator-=(uint16_t val);
    void operator*=(uint16_t val);
    void operator/=(uint16_t val);
    void operator++(int);
    void operator--(int);

    bool operator<(uint16_t val);
    bool operator<=(uint16_t val);
    bool operator>(uint16_t val);
    bool operator>=(uint16_t val);
    bool operator==(uint16_t val);
    bool operator!=(uint16_t val);

    uint16_t operator+(uint16_t val);
    uint16_t operator-(uint16_t val);
    uint16_t operator*(uint16_t val);
    uint16_t operator/(uint16_t val);

private:
    uint16_t mVal = 0;
    std::mutex mMutex;
};

/*!
Mutex protected unsigned uint8 for multi-thread data sharing
*/
class SharedUInt8 {
public:
    SharedUInt8() = default;
    explicit SharedUInt8(uint8_t val);
    SharedUInt8(const SharedUInt8& si);

    uint8_t getVal();
    void setVal(uint8_t val);

    void operator=(const SharedUInt8& si);
    void operator=(uint8_t val);
    void operator+=(uint8_t val);
    void operator-=(uint8_t val);
    void operator*=(uint8_t val);
    void operator/=(uint8_t val);
    void operator++(int);
    void operator--(int);

    bool operator<(uint8_t val);
    bool operator<=(uint8_t val);
    bool operator>(uint8_t val);
    bool operator>=(uint8_t val);
    bool operator==(uint8_t val);
    bool operator!=(uint8_t val);

    uint8_t operator+(uint8_t val);
    uint8_t operator-(uint8_t val);
    uint8_t operator*(uint8_t val);
    uint8_t operator/(uint8_t val);

private:
    uint8_t mVal = 0;
    std::mutex mMutex;
};

//backwards compability
using SharedShort = SharedInt16;
using SharedInt = SharedInt32;

/*!
Mutex protected unsigned char for multi-thread data sharing
*/
class SharedUChar {
public:
    SharedUChar() = default;
    explicit SharedUChar(unsigned char val);
    unsigned char getVal();
    void setVal(unsigned char val);

private:
    unsigned char mVal = 0;
    std::mutex mMutex;
};

/*!
Mutex protected bool for multi-thread data sharing
*/
class SharedBool {
public:
    SharedBool() = default;
    explicit SharedBool(bool val);
    SharedBool(const SharedBool& sb);
        
    bool getVal();
    void setVal(bool val);
    void toggle();
        
    void operator=(const SharedBool& sb);
    void operator=(bool val);
    bool operator==(bool val);
    bool operator!=(bool val);

private:
    bool mVal = false;
    std::mutex mMutex;
};

/*!
Mutex protected std::string for multi-thread data sharing
*/
class SharedString {
public:
    SharedString() = default;
    explicit SharedString(std::string str);
    SharedString(const SharedString& ss);

    std::string getVal();
    void setVal(std::string str);
    void clear();

    void operator=(const std::string& str);
    void operator=(const SharedString& ss);

private:
    std::string mStr;
    std::mutex mMutex;
};

/*!
Mutex protected std::wstring for multi-thread data sharing
*/
class SharedWString {
public:
    SharedWString() = default;
    explicit SharedWString(std::wstring str);
    SharedWString(const SharedWString& ss);

    std::wstring getVal();
    void setVal(std::wstring str);
    void clear();

    void operator=(const std::wstring& str);
    void operator=(const SharedWString& ss);

private:
    std::wstring mStr;
    std::mutex mMutex;
};

/*!
Mutex protected template for multi-thread data sharing
*/
template <class T>
class SharedObject {
public:
    SharedObject() = default;
    explicit SharedObject(T val) : mVal(val) {}

    T getVal() {
        mMutex.lock();
        T tmpT = mVal;
        mMutex.unlock();
        return tmpT;
    }

    void setVal(T val) {
        mMutex.lock();
        mVal = std::move(val);
        mMutex.unlock();
    }

private:
    T mVal;
    std::mutex mMutex;
};

/*!
Mutex protected std::vector template for multi-thread data sharing
*/
template <class T>
class SharedVector {
public:
    SharedVector() = default;
    explicit SharedVector(size_t size) {
        mVector.reserve(size);
    }

    T getValAt(size_t index) {
        mMutex.lock();
        T tmpT = mVector[index];
        mMutex.unlock();
        return tmpT;
    }

    std::vector<T> getVal() {
        mMutex.lock();
        std::vector<T> mCopy = mVector;
        mMutex.unlock();
        return mCopy;
    }

    void setValAt(size_t index, T val) {
        mMutex.lock();
        mVector[index] = val;
        mMutex.unlock();
    }

    void addVal(T val) {
        mMutex.lock();
        mVector.push_back(val);
        mMutex.unlock();
    }

    void setVal(std::vector<T> mCopy) {
        mMutex.lock();
        mVector = std::move(mCopy);
        mMutex.unlock();
    }

    void clear() {
        mMutex.lock();
        mVector.clear();
        mMutex.unlock();
    }

    size_t getSize() {
        mMutex.lock();
        size_t size = mVector.size();
        mMutex.unlock();
        return size;
    }

private:
    std::vector<T> mVector;
    std::mutex mMutex;
};

} // namespace sgct

#endif // __SGCT__SHARED_DATA_TYPES__H__
