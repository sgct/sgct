/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/SharedDataTypes.h>
#include <sgct/SGCTMutexManager.h>

sgct::SharedFloat::SharedFloat()
{
    mVal = 0.0f;
}

sgct::SharedFloat::SharedFloat(float val)
{
    mVal = val;
}

sgct::SharedFloat::SharedFloat(const SharedFloat & sf)
{
    mVal = sf.mVal;
}

float sgct::SharedFloat::getVal()
{
    float tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedFloat::setVal(float val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedFloat::operator=(const SharedFloat & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedFloat::operator=(const float & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedFloat::operator+=(const float & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedFloat::operator-=(const float & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedFloat::operator*=(const float & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedFloat::operator/=(const float & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedFloat::operator++(int)
{
    mMutex.lock();
    mVal += 1.0f;
    mMutex.unlock();
}

void sgct::SharedFloat::operator--(int)
{
    mMutex.lock();
    mVal -= 1.0f;
    mMutex.unlock();
}

bool sgct::SharedFloat::operator<(const float & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedFloat::operator<=(const float & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedFloat::operator>(const float & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedFloat::operator>=(const float & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedFloat::operator==(const float & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedFloat::operator!=(const float & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

float sgct::SharedFloat::operator+(const float & val)
{
    float tmpF;
    mMutex.lock();
    tmpF = (mVal + val);
    mMutex.unlock();
    return tmpF;
}

float sgct::SharedFloat::operator-(const float & val)
{
    float tmpF;
    mMutex.lock();
    tmpF = (mVal - val);
    mMutex.unlock();
    return tmpF;
}

float sgct::SharedFloat::operator*(const float & val)
{
    float tmpF;
    mMutex.lock();
    tmpF = (mVal * val);
    mMutex.unlock();
    return tmpF;
}

float sgct::SharedFloat::operator/(const float & val)
{
    float tmpF;
    mMutex.lock();
    tmpF = (mVal / val);
    mMutex.unlock();
    return tmpF;
}

sgct::SharedDouble::SharedDouble()
{
    mVal = 0.0;
}

sgct::SharedDouble::SharedDouble(double val)
{
    mVal = val;
}

sgct::SharedDouble::SharedDouble( const SharedDouble & sd )
{
    mVal = sd.mVal;
}

double sgct::SharedDouble::getVal()
{
    double tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedDouble::setVal(double val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedDouble::operator=(const SharedDouble & sd)
{
    mMutex.lock();
    mVal = sd.mVal;
    mMutex.unlock();
}

void sgct::SharedDouble::operator=( const double & val )
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedDouble::operator+=( const double & val )
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedDouble::operator-=( const double & val )
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedDouble::operator*=( const double & val )
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedDouble::operator/=( const double & val )
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedDouble::operator++(int)
{
    mMutex.lock();
    mVal += 1.0;
    mMutex.unlock();
}

void sgct::SharedDouble::operator--(int)
{
    mMutex.lock();
    mVal -= 1.0;
    mMutex.unlock();
}

bool sgct::SharedDouble::operator<( const double & val )
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedDouble::operator<=( const double & val )
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedDouble::operator>( const double & val )
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedDouble::operator>=( const double & val )
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedDouble::operator==( const double & val )
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedDouble::operator!=( const double & val )
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

double sgct::SharedDouble::operator+( const double & val )
{
    double tmpD;
    mMutex.lock();
    tmpD = (mVal + val);
    mMutex.unlock();
    return tmpD;
}

double sgct::SharedDouble::operator-( const double & val )
{
    double tmpD;
    mMutex.lock();
    tmpD = (mVal - val);
    mMutex.unlock();
    return tmpD;
}

double sgct::SharedDouble::operator*( const double & val )
{
    double tmpD;
    mMutex.lock();
    tmpD = (mVal * val);
    mMutex.unlock();
    return tmpD;
}

double sgct::SharedDouble::operator/( const double & val )
{
    double tmpD;
    mMutex.lock();
    tmpD = (mVal / val);
    mMutex.unlock();
    return tmpD;
}

sgct::SharedInt64::SharedInt64()
{
    mVal = 0;
}

sgct::SharedInt64::SharedInt64(int64_t val)
{
    mVal = val;
}

sgct::SharedInt64::SharedInt64(const SharedInt64 & sf)
{
    mVal = sf.mVal;
}

int64_t sgct::SharedInt64::getVal()
{
    int64_t tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedInt64::setVal(int64_t val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedInt64::operator=(const SharedInt64 & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedInt64::operator=(const int64_t & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedInt64::operator+=(const int64_t & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedInt64::operator-=(const int64_t & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedInt64::operator*=(const int64_t & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedInt64::operator/=(const int64_t & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedInt64::operator++(int)
{
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void sgct::SharedInt64::operator--(int)
{
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool sgct::SharedInt64::operator<(const int64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt64::operator<=(const int64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt64::operator>(const int64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt64::operator>=(const int64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt64::operator==(const int64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt64::operator!=(const int64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

int64_t sgct::SharedInt64::operator+(const int64_t & val)
{
    int64_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

int64_t sgct::SharedInt64::operator-(const int64_t & val)
{
    int64_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

int64_t sgct::SharedInt64::operator*(const int64_t & val)
{
    int64_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

int64_t sgct::SharedInt64::operator/(const int64_t & val)
{
    int64_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

sgct::SharedInt32::SharedInt32()
{
    mVal = 0;
}

sgct::SharedInt32::SharedInt32(int32_t val)
{
    mVal = val;
}

sgct::SharedInt32::SharedInt32(const SharedInt32 & sf)
{
    mVal = sf.mVal;
}

int32_t sgct::SharedInt32::getVal()
{
    int32_t tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedInt32::setVal(int32_t val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedInt32::operator=(const SharedInt32 & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedInt32::operator=(const int32_t & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedInt32::operator+=(const int32_t & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedInt32::operator-=(const int32_t & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedInt32::operator*=(const int32_t & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedInt32::operator/=(const int32_t & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedInt32::operator++(int)
{
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void sgct::SharedInt32::operator--(int)
{
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool sgct::SharedInt32::operator<(const int32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt32::operator<=(const int32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt32::operator>(const int32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt32::operator>=(const int32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt32::operator==(const int32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt32::operator!=(const int32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

int32_t sgct::SharedInt32::operator+(const int32_t & val)
{
    int32_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

int32_t sgct::SharedInt32::operator-(const int32_t & val)
{
    int32_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

int32_t sgct::SharedInt32::operator*(const int32_t & val)
{
    int32_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

int32_t sgct::SharedInt32::operator/(const int32_t & val)
{
    int32_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

sgct::SharedInt16::SharedInt16()
{
    mVal = 0;
}

sgct::SharedInt16::SharedInt16(int16_t val)
{
    mVal = val;
}

sgct::SharedInt16::SharedInt16(const SharedInt16 & sf)
{
    mVal = sf.mVal;
}

int16_t sgct::SharedInt16::getVal()
{
    int16_t tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedInt16::setVal(int16_t val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedInt16::operator=(const SharedInt16 & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedInt16::operator=(const int16_t & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedInt16::operator+=(const int16_t & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedInt16::operator-=(const int16_t & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedInt16::operator*=(const int16_t & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedInt16::operator/=(const int16_t & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedInt16::operator++(int)
{
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void sgct::SharedInt16::operator--(int)
{
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool sgct::SharedInt16::operator<(const int16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt16::operator<=(const int16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt16::operator>(const int16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt16::operator>=(const int16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt16::operator==(const int16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt16::operator!=(const int16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

int16_t sgct::SharedInt16::operator+(const int16_t & val)
{
    int16_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

int16_t sgct::SharedInt16::operator-(const int16_t & val)
{
    int16_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

int16_t sgct::SharedInt16::operator*(const int16_t & val)
{
    int16_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

int16_t sgct::SharedInt16::operator/(const int16_t & val)
{
    int16_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

sgct::SharedInt8::SharedInt8()
{
    mVal = 0;
}

sgct::SharedInt8::SharedInt8(int8_t val)
{
    mVal = val;
}

sgct::SharedInt8::SharedInt8(const SharedInt8 & sf)
{
    mVal = sf.mVal;
}

int8_t sgct::SharedInt8::getVal()
{
    int8_t tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedInt8::setVal(int8_t val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedInt8::operator=(const SharedInt8 & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedInt8::operator=(const int8_t & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedInt8::operator+=(const int8_t & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedInt8::operator-=(const int8_t & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedInt8::operator*=(const int8_t & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedInt8::operator/=(const int8_t & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedInt8::operator++(int)
{
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void sgct::SharedInt8::operator--(int)
{
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool sgct::SharedInt8::operator<(const int8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt8::operator<=(const int8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt8::operator>(const int8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt8::operator>=(const int8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt8::operator==(const int8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedInt8::operator!=(const int8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

int8_t sgct::SharedInt8::operator+(const int8_t & val)
{
    int8_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

int8_t sgct::SharedInt8::operator-(const int8_t & val)
{
    int8_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

int8_t sgct::SharedInt8::operator*(const int8_t & val)
{
    int8_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

int8_t sgct::SharedInt8::operator/(const int8_t & val)
{
    int8_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

sgct::SharedUInt64::SharedUInt64()
{
    mVal = 0;
}

sgct::SharedUInt64::SharedUInt64(uint64_t val)
{
    mVal = val;
}

sgct::SharedUInt64::SharedUInt64(const SharedUInt64 & sf)
{
    mVal = sf.mVal;
}

uint64_t sgct::SharedUInt64::getVal()
{
    uint64_t tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedUInt64::setVal(uint64_t val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedUInt64::operator=(const SharedUInt64 & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedUInt64::operator=(const uint64_t & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedUInt64::operator+=(const uint64_t & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedUInt64::operator-=(const uint64_t & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedUInt64::operator*=(const uint64_t & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedUInt64::operator/=(const uint64_t & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedUInt64::operator++(int)
{
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void sgct::SharedUInt64::operator--(int)
{
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool sgct::SharedUInt64::operator<(const uint64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt64::operator<=(const uint64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt64::operator>(const uint64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt64::operator>=(const uint64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt64::operator==(const uint64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt64::operator!=(const uint64_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

uint64_t sgct::SharedUInt64::operator+(const uint64_t & val)
{
    uint64_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

uint64_t sgct::SharedUInt64::operator-(const uint64_t & val)
{
    uint64_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

uint64_t sgct::SharedUInt64::operator*(const uint64_t & val)
{
    uint64_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

uint64_t sgct::SharedUInt64::operator/(const uint64_t & val)
{
    uint64_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

sgct::SharedUInt32::SharedUInt32()
{
    mVal = 0;
}

sgct::SharedUInt32::SharedUInt32(uint32_t val)
{
    mVal = val;
}

sgct::SharedUInt32::SharedUInt32(const SharedUInt32 & sf)
{
    mVal = sf.mVal;
}

uint32_t sgct::SharedUInt32::getVal()
{
    uint32_t tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedUInt32::setVal(uint32_t val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedUInt32::operator=(const SharedUInt32 & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedUInt32::operator=(const uint32_t & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedUInt32::operator+=(const uint32_t & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedUInt32::operator-=(const uint32_t & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedUInt32::operator*=(const uint32_t & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedUInt32::operator/=(const uint32_t & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedUInt32::operator++(int)
{
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void sgct::SharedUInt32::operator--(int)
{
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool sgct::SharedUInt32::operator<(const uint32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt32::operator<=(const uint32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt32::operator>(const uint32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt32::operator>=(const uint32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt32::operator==(const uint32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt32::operator!=(const uint32_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

uint32_t sgct::SharedUInt32::operator+(const uint32_t & val)
{
    uint32_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

uint32_t sgct::SharedUInt32::operator-(const uint32_t & val)
{
    uint32_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

uint32_t sgct::SharedUInt32::operator*(const uint32_t & val)
{
    uint32_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

uint32_t sgct::SharedUInt32::operator/(const uint32_t & val)
{
    uint32_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

sgct::SharedUInt16::SharedUInt16()
{
    mVal = 0;
}

sgct::SharedUInt16::SharedUInt16(uint16_t val)
{
    mVal = val;
}

sgct::SharedUInt16::SharedUInt16(const SharedUInt16 & sf)
{
    mVal = sf.mVal;
}

uint16_t sgct::SharedUInt16::getVal()
{
    uint16_t tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedUInt16::setVal(uint16_t val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedUInt16::operator=(const SharedUInt16 & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedUInt16::operator=(const uint16_t & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedUInt16::operator+=(const uint16_t & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedUInt16::operator-=(const uint16_t & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedUInt16::operator*=(const uint16_t & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedUInt16::operator/=(const uint16_t & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedUInt16::operator++(int)
{
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void sgct::SharedUInt16::operator--(int)
{
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool sgct::SharedUInt16::operator<(const uint16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt16::operator<=(const uint16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt16::operator>(const uint16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt16::operator>=(const uint16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt16::operator==(const uint16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt16::operator!=(const uint16_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

uint16_t sgct::SharedUInt16::operator+(const uint16_t & val)
{
    uint16_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

uint16_t sgct::SharedUInt16::operator-(const uint16_t & val)
{
    uint16_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

uint16_t sgct::SharedUInt16::operator*(const uint16_t & val)
{
    uint16_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

uint16_t sgct::SharedUInt16::operator/(const uint16_t & val)
{
    uint16_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

sgct::SharedUInt8::SharedUInt8()
{
    mVal = 0;
}

sgct::SharedUInt8::SharedUInt8(uint8_t val)
{
    mVal = val;
}

sgct::SharedUInt8::SharedUInt8(const SharedUInt8 & sf)
{
    mVal = sf.mVal;
}

uint8_t sgct::SharedUInt8::getVal()
{
    uint8_t tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedUInt8::setVal(uint8_t val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedUInt8::operator=(const SharedUInt8 & sf)
{
    mMutex.lock();
    mVal = sf.mVal;
    mMutex.unlock();
}

void sgct::SharedUInt8::operator=(const uint8_t & val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedUInt8::operator+=(const uint8_t & val)
{
    mMutex.lock();
    mVal += val;
    mMutex.unlock();
}

void sgct::SharedUInt8::operator-=(const uint8_t & val)
{
    mMutex.lock();
    mVal -= val;
    mMutex.unlock();
}

void sgct::SharedUInt8::operator*=(const uint8_t & val)
{
    mMutex.lock();
    mVal *= val;
    mMutex.unlock();
}

void sgct::SharedUInt8::operator/=(const uint8_t & val)
{
    mMutex.lock();
    mVal /= val;
    mMutex.unlock();
}

void sgct::SharedUInt8::operator++(int)
{
    mMutex.lock();
    mVal++;
    mMutex.unlock();
}

void sgct::SharedUInt8::operator--(int)
{
    mMutex.lock();
    mVal--;
    mMutex.unlock();
}

bool sgct::SharedUInt8::operator<(const uint8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal < val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt8::operator<=(const uint8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal <= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt8::operator>(const uint8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal > val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt8::operator>=(const uint8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal >= val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt8::operator==(const uint8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedUInt8::operator!=(const uint8_t & val)
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

uint8_t sgct::SharedUInt8::operator+(const uint8_t & val)
{
    uint8_t tmpI;
    mMutex.lock();
    tmpI = (mVal + val);
    mMutex.unlock();
    return tmpI;
}

uint8_t sgct::SharedUInt8::operator-(const uint8_t & val)
{
    uint8_t tmpI;
    mMutex.lock();
    tmpI = (mVal - val);
    mMutex.unlock();
    return tmpI;
}

uint8_t sgct::SharedUInt8::operator*(const uint8_t & val)
{
    uint8_t tmpI;
    mMutex.lock();
    tmpI = (mVal * val);
    mMutex.unlock();
    return tmpI;
}

uint8_t sgct::SharedUInt8::operator/(const uint8_t & val)
{
    uint8_t tmpI;
    mMutex.lock();
    tmpI = (mVal / val);
    mMutex.unlock();
    return tmpI;
}

sgct::SharedUChar::SharedUChar()
{
    mVal = 0;
}

sgct::SharedUChar::SharedUChar(unsigned char val)
{
    mVal = val;
}

unsigned char sgct::SharedUChar::getVal()
{
    unsigned char tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedUChar::setVal(unsigned char val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

sgct::SharedBool::SharedBool()
{
    mVal = false;
}

sgct::SharedBool::SharedBool(bool val)
{
    mVal = val;
}

sgct::SharedBool::SharedBool( const SharedBool & sd )
{
    mVal = sd.mVal;
}

bool sgct::SharedBool::getVal()
{
    bool tmpVal;

    mMutex.lock();
    tmpVal = mVal;
    mMutex.unlock();

    return tmpVal;
}

void sgct::SharedBool::setVal(bool val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedBool::toggle()
{
    mMutex.lock();
    mVal = !mVal;
    mMutex.unlock();
}

void sgct::SharedBool::operator=( const bool & val )
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}

void sgct::SharedBool::operator=(const SharedBool & sb)
{
    mMutex.lock();
    mVal = sb.mVal;
    mMutex.unlock();
}

bool sgct::SharedBool::operator==( const bool & val )
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal == val);
    mMutex.unlock();
    return tmpB;
}

bool sgct::SharedBool::operator!=( const bool & val )
{
    bool tmpB;
    mMutex.lock();
    tmpB = (mVal != val);
    mMutex.unlock();
    return tmpB;
}

sgct::SharedString::SharedString()
{
    ;
}

sgct::SharedString::SharedString(const std::string & str)
{
    mStr.assign(str);
}

sgct::SharedString::SharedString(const SharedString & ss)
{
    mStr = ss.mStr;
}

std::string sgct::SharedString::getVal()
{
    std::string tmpStr;

    mMutex.lock();
    tmpStr.assign( mStr );
    mMutex.unlock();

    return tmpStr;
}

void sgct::SharedString::setVal(const std::string & str)
{
    mMutex.lock();
    mStr.assign(str);
    mMutex.unlock();
}

void sgct::SharedString::clear()
{
    mMutex.lock();
    mStr.clear();
    mMutex.unlock();
}

void sgct::SharedString::operator=(const std::string & str)
{
    mMutex.lock();
    mStr = str;
    mMutex.unlock();
}

void sgct::SharedString::operator=(const SharedString & ss)
{
    mMutex.lock();
    mStr = ss.mStr;
    mMutex.unlock();
}

/*
template <class T>
sgct::SharedObject<T>::SharedObject()
{
    ;
}

template <class T>
sgct::SharedObject<T>::SharedObject(T val)
{
    mVal = val;
}

template <class T>
T sgct::SharedObject<T>::getVal()
{
    T tmpT;
    mMutex.lock();
    tmpT = mVal;
    mMutex.unlock();
    return tmpT;
}

template <class T>
void sgct::SharedObject<T>::setVal(T val)
{
    mMutex.lock();
    mVal = val;
    mMutex.unlock();
}
*/
