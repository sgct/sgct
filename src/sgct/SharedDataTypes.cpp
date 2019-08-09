/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SharedDataTypes.h>

#include <sgct/SGCTMutexManager.h>

namespace sgct {

SharedFloat::SharedFloat(float val) : mVal(val) {}

SharedFloat::SharedFloat(const SharedFloat& sf) : mVal(sf.mVal) {}

float SharedFloat::getVal() {
    mMutex.lock();
    float tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedFloat::setVal(float val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedFloat::operator=(const SharedFloat& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedFloat::operator=(float val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedFloat::operator+=(float val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedFloat::operator-=(float val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedFloat::operator*=(float val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedFloat::operator/=(float val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedFloat::operator++(int) {
    mMutex.lock();
    mVal += 1.f;
    mMutex.unlock();
}

void SharedFloat::operator--(int) {
    mMutex.lock();
    mVal -= 1.f;
    mMutex.unlock();
}

bool sgct::SharedFloat::operator<(float val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedFloat::operator<=(float val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedFloat::operator>(float val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedFloat::operator>=(float val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedFloat::operator==(float val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedFloat::operator!=(float val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

float SharedFloat::operator+(float val) {
    mMutex.lock();
    float tmpF = (mVal + val);
    mMutex.unlock();
    return tmpF;
}

float SharedFloat::operator-(float val) {
    mMutex.lock();
    float tmpF = (mVal - val);
    mMutex.unlock();
    return tmpF;
}

float SharedFloat::operator*(float val) {
    mMutex.lock();
    float tmpF = (mVal * val);
    mMutex.unlock();
    return tmpF;
}

float SharedFloat::operator/(float val) {
    mMutex.lock();
    float tmpF = (mVal / val);
    mMutex.unlock();
    return tmpF;
}

SharedDouble::SharedDouble(double val) : mVal(val) {}

SharedDouble::SharedDouble(const SharedDouble& sd) : mVal(sd.mVal) {}

double SharedDouble::getVal() {
    mMutex.lock();
    double tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedDouble::setVal(double val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedDouble::operator=(const SharedDouble& sd) {
    mMutex.lock();
    mVal = sd.mVal;
    mMutex.unlock();
}

void SharedDouble::operator=(double val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedDouble::operator+=(double val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedDouble::operator-=(double val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedDouble::operator*=(double val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedDouble::operator/=(double val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedDouble::operator++(int) {
    mMutex.lock();
    mVal += 1.0;
    mMutex.unlock();
}

void SharedDouble::operator--(int) {
    mMutex.lock();
    mVal -= 1.0;
    mMutex.unlock();
}

bool SharedDouble::operator<(double val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedDouble::operator<=(double val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedDouble::operator>(double val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedDouble::operator>=(double val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedDouble::operator==(double val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedDouble::operator!=(double val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

double SharedDouble::operator+(double val) {
    mMutex.lock();
    double tmpD = (mVal + val);
    mMutex.unlock();
    return tmpD;
}

double SharedDouble::operator-(double val) {
    mMutex.lock();
    double tmpD = (mVal - val);
    mMutex.unlock();
    return tmpD;
}

double SharedDouble::operator*(double val) {
    mMutex.lock();
    double tmpD = (mVal * val);
    mMutex.unlock();
    return tmpD;
}

double SharedDouble::operator/(double val) {
    mMutex.lock();
    double tmpD = (mVal / val);
    mMutex.unlock();
    return tmpD;
}

sgct::SharedInt64::SharedInt64(int64_t val) : mVal(val) {}

sgct::SharedInt64::SharedInt64(const SharedInt64& sf) : mVal(sf.mVal) {}

int64_t SharedInt64::getVal() {
    mMutex.lock();
    int64_t tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedInt64::setVal(int64_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedInt64::operator=(const SharedInt64& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedInt64::operator=(int64_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedInt64::operator+=(int64_t val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedInt64::operator-=(int64_t val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedInt64::operator*=(int64_t val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedInt64::operator/=(int64_t val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedInt64::operator++(int) {
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void SharedInt64::operator--(int) {
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool SharedInt64::operator<(int64_t val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt64::operator<=(int64_t val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt64::operator>(int64_t val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt64::operator>=(int64_t val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt64::operator==(int64_t val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt64::operator!=(int64_t val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

int64_t SharedInt64::operator+(int64_t val) {
    mMutex.lock();
    int64_t tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

int64_t SharedInt64::operator-(int64_t val) {
    mMutex.lock();
    int64_t tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

int64_t SharedInt64::operator*(int64_t val) {
    mMutex.lock();
    int64_t tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

int64_t SharedInt64::operator/(int64_t val) {
    mMutex.lock();
    int64_t tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

SharedInt32::SharedInt32(int32_t val) : mVal(val) {}

SharedInt32::SharedInt32(const SharedInt32& sf) : mVal(sf.mVal) {}

int32_t SharedInt32::getVal() {
    mMutex.lock();
    int32_t tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedInt32::setVal(int32_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedInt32::operator=(const SharedInt32& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedInt32::operator=(int32_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedInt32::operator+=(int32_t val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedInt32::operator-=(int32_t val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedInt32::operator*=(int32_t val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedInt32::operator/=(int32_t val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedInt32::operator++(int) {
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void SharedInt32::operator--(int) {
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool SharedInt32::operator<(int32_t val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt32::operator<=(int32_t val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt32::operator>(int32_t val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt32::operator>=(int32_t val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt32::operator==(int32_t val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt32::operator!=(int32_t val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

int32_t SharedInt32::operator+(int32_t val) {
    mMutex.lock();
    int32_t tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

int32_t SharedInt32::operator-(int32_t val) {
    mMutex.lock();
    int32_t tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

int32_t SharedInt32::operator*(int32_t val) {
    mMutex.lock();
    int32_t tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

int32_t SharedInt32::operator/(int32_t val) {
    mMutex.lock();
    int32_t tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

SharedInt16::SharedInt16(int16_t val) : mVal(val) {}

SharedInt16::SharedInt16(const SharedInt16& sf) : mVal(sf.mVal) {}

int16_t SharedInt16::getVal() {
    mMutex.lock();
    int16_t tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedInt16::setVal(int16_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedInt16::operator=(const SharedInt16& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedInt16::operator=(int16_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedInt16::operator+=(int16_t val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedInt16::operator-=(int16_t val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedInt16::operator*=(int16_t val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedInt16::operator/=(int16_t val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedInt16::operator++(int) {
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void SharedInt16::operator--(int) {
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool SharedInt16::operator<(int16_t val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt16::operator<=(int16_t val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt16::operator>(int16_t val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt16::operator>=(int16_t val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt16::operator==(int16_t val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt16::operator!=(int16_t val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

int16_t SharedInt16::operator+(int16_t val) {
    mMutex.lock();
    int16_t tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

int16_t SharedInt16::operator-(int16_t val) {
    mMutex.lock();
    int16_t tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

int16_t SharedInt16::operator*(int16_t val) {
    mMutex.lock();
    int16_t tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

int16_t SharedInt16::operator/(int16_t val) {
    mMutex.lock();
    int16_t tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

SharedInt8::SharedInt8(int8_t val) : mVal(val) {}

SharedInt8::SharedInt8(const SharedInt8& sf) : mVal(sf.mVal) {}

int8_t SharedInt8::getVal() {
    mMutex.lock();
    int8_t tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedInt8::setVal(int8_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedInt8::operator=(const SharedInt8& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedInt8::operator=(int8_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedInt8::operator+=(int8_t val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedInt8::operator-=(int8_t val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedInt8::operator*=(int8_t val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedInt8::operator/=(int8_t val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedInt8::operator++(int) {
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void SharedInt8::operator--(int) {
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool SharedInt8::operator<(int8_t val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt8::operator<=(int8_t val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt8::operator>(int8_t val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt8::operator>=(int8_t val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt8::operator==(int8_t val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedInt8::operator!=(int8_t val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

int8_t SharedInt8::operator+(int8_t val) {
    mMutex.lock();
    int8_t tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

int8_t SharedInt8::operator-(int8_t val) {
    mMutex.lock();
    int8_t tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

int8_t SharedInt8::operator*(int8_t val) {
    mMutex.lock();
    int8_t tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

int8_t SharedInt8::operator/(int8_t val) {
    mMutex.lock();
    int8_t tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

SharedUInt64::SharedUInt64(uint64_t val) : mVal(val) {}

SharedUInt64::SharedUInt64(const SharedUInt64& sf) : mVal(sf.mVal) {}

uint64_t SharedUInt64::getVal() {
    mMutex.lock();
    uint64_t tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedUInt64::setVal(uint64_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedUInt64::operator=(const SharedUInt64& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedUInt64::operator=(uint64_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedUInt64::operator+=(uint64_t val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedUInt64::operator-=(uint64_t val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedUInt64::operator*=(uint64_t val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedUInt64::operator/=(uint64_t val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedUInt64::operator++(int) {
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void SharedUInt64::operator--(int) {
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool SharedUInt64::operator<(uint64_t val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt64::operator<=(uint64_t val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt64::operator>(uint64_t val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt64::operator>=(uint64_t val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt64::operator==(uint64_t val) {
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt64::operator!=(uint64_t val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

uint64_t SharedUInt64::operator+(uint64_t val) {
    mMutex.lock();
    uint64_t tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

uint64_t SharedUInt64::operator-(uint64_t val) {
    mMutex.lock();
    uint64_t tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

uint64_t SharedUInt64::operator*(uint64_t val) {
    mMutex.lock();
    uint64_t tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

uint64_t SharedUInt64::operator/(uint64_t val) {
    mMutex.lock();
    uint64_t tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

SharedUInt32::SharedUInt32(uint32_t val) : mVal(val) {}

SharedUInt32::SharedUInt32(const SharedUInt32& sf) : mVal(sf.mVal) {}

uint32_t SharedUInt32::getVal() {
    mMutex.lock();
    uint32_t tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedUInt32::setVal(uint32_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedUInt32::operator=(const SharedUInt32& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedUInt32::operator=(uint32_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedUInt32::operator+=(uint32_t val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedUInt32::operator-=(uint32_t val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedUInt32::operator*=(uint32_t val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedUInt32::operator/=(uint32_t val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedUInt32::operator++(int) {
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void SharedUInt32::operator--(int) {
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool SharedUInt32::operator<(uint32_t val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt32::operator<=(uint32_t val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt32::operator>(uint32_t val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt32::operator>=(uint32_t val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt32::operator==(uint32_t val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt32::operator!=(uint32_t val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

uint32_t SharedUInt32::operator+(uint32_t val) {
    uint32_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

uint32_t SharedUInt32::operator-(uint32_t val) {
    uint32_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

uint32_t SharedUInt32::operator*(uint32_t val) {
    uint32_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

uint32_t SharedUInt32::operator/(uint32_t val) {
    uint32_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

SharedUInt16::SharedUInt16(uint16_t val) : mVal(val) {}

SharedUInt16::SharedUInt16(const SharedUInt16& sf) : mVal(sf.mVal) {}

uint16_t SharedUInt16::getVal() {
    mMutex.lock();
    uint16_t tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedUInt16::setVal(uint16_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedUInt16::operator=(const SharedUInt16& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedUInt16::operator=(uint16_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedUInt16::operator+=(uint16_t val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedUInt16::operator-=(uint16_t val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedUInt16::operator*=(uint16_t val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedUInt16::operator/=(uint16_t val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedUInt16::operator++(int) {
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void SharedUInt16::operator--(int) {
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool SharedUInt16::operator<(uint16_t val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt16::operator<=(uint16_t val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt16::operator>(uint16_t val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt16::operator>=(uint16_t val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt16::operator==(uint16_t val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt16::operator!=(uint16_t val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

uint16_t SharedUInt16::operator+(uint16_t val) {
    mMutex.lock();
    uint16_t tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

uint16_t SharedUInt16::operator-(uint16_t val) {
    mMutex.lock();
    uint16_t tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

uint16_t SharedUInt16::operator*(uint16_t val) {
    mMutex.lock();
    uint16_t tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

uint16_t SharedUInt16::operator/(uint16_t val) {
    mMutex.lock();
    uint16_t tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

SharedUInt8::SharedUInt8(uint8_t val) : mVal(val) {}

SharedUInt8::SharedUInt8(const SharedUInt8& sf) : mVal(sf.mVal) {}

uint8_t SharedUInt8::getVal() {
    mMutex.lock();
    uint8_t tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedUInt8::setVal(uint8_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedUInt8::operator=(const SharedUInt8& sf) {
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void SharedUInt8::operator=(uint8_t val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedUInt8::operator+=(uint8_t val) {
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void SharedUInt8::operator-=(uint8_t val) {
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void SharedUInt8::operator*=(uint8_t val) {
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void SharedUInt8::operator/=(uint8_t val) {
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void SharedUInt8::operator++(int) {
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void SharedUInt8::operator--(int) {
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool SharedUInt8::operator<(uint8_t val) {
    mMutex.lock();
    bool tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt8::operator<=(uint8_t val) {
    mMutex.lock();
    bool tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt8::operator>(uint8_t val) {
    mMutex.lock();
    bool tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt8::operator>=(uint8_t val) {
    mMutex.lock();
    bool tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt8::operator==(uint8_t val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedUInt8::operator!=(uint8_t val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

uint8_t SharedUInt8::operator+(uint8_t val) {
    mMutex.lock();
    uint8_t tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

uint8_t SharedUInt8::operator-(uint8_t val) {
    mMutex.lock();
    uint8_t tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

uint8_t SharedUInt8::operator*(uint8_t val) {
    mMutex.lock();
    uint8_t tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

uint8_t SharedUInt8::operator/(uint8_t val) {
    mMutex.lock();
    uint8_t tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

SharedUChar::SharedUChar(unsigned char val) : mVal(val) {}

unsigned char SharedUChar::getVal() {
    mMutex.lock();
    unsigned char tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedUChar::setVal(unsigned char val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

SharedBool::SharedBool(bool val) : mVal(val) {}

SharedBool::SharedBool(const SharedBool& sd) {
    mVal = sd.mVal;
}

bool SharedBool::getVal() {
    mMutex.lock();
    bool tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void SharedBool::setVal(bool val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedBool::toggle() {
    mMutex.lock();
    mVal = !mVal;
    mMutex.unlock();
}

void SharedBool::operator=(bool val) {
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void SharedBool::operator=(const SharedBool& sb) {
    mMutex.lock();
    mVal = sb.mVal;
    mMutex.unlock();
}

bool SharedBool::operator==(bool val) {
    mMutex.lock();
    bool tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool SharedBool::operator!=(bool val) {
    mMutex.lock();
    bool tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

SharedString::SharedString(std::string str) : mStr(std::move(str)) {}

SharedString::SharedString(const SharedString& ss) : mStr(ss.mStr) {}

std::string SharedString::getVal() {
    mMutex.lock();
    std::string tmpStr = mStr;
    mMutex.unlock();

    return tmpStr;
}

void SharedString::setVal(std::string str) {
    mMutex.lock();
    mStr = std::move(str);
    mMutex.unlock();
}

void SharedString::clear() {
    mMutex.lock();
    mStr.clear();
    mMutex.unlock();
}

void SharedString::operator=(const std::string& str) {
    mMutex.lock();
    mStr = str;
    mMutex.unlock();
}

void SharedString::operator=(const SharedString& ss) {
    mMutex.lock();
    mStr = ss.mStr;
    mMutex.unlock();
}

SharedWString::SharedWString(std::wstring str) : mStr(std::move(str)) {}

SharedWString::SharedWString(const SharedWString & ss) : mStr(ss.mStr) {}

std::wstring SharedWString::getVal() {
    mMutex.lock();
    std::wstring tmpStr = mStr;
    mMutex.unlock();

    return tmpStr;
}

void SharedWString::setVal(std::wstring str) {
    mMutex.lock();
    mStr = std::move(str);
    mMutex.unlock();
}

void SharedWString::clear() {
    mMutex.lock();
    mStr.clear();
    mMutex.unlock();
}

void SharedWString::operator=(const std::wstring& str) {
    mMutex.lock();
    mStr = str;
    mMutex.unlock();
}

void SharedWString::operator=(const SharedWString& ss) {
    mMutex.lock();
    mStr = ss.mStr;
    mMutex.unlock();
}

} // namespace sgct
