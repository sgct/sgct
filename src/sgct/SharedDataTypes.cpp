/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SharedDataTypes.h"
#include "../include/sgct/SGCTMutexManager.h"

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

void sgct::SharedFloat::operator++()
{
	mMutex.lock();
	mVal += 1.0f;
	mMutex.unlock();
}

void sgct::SharedFloat::operator--()
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

void sgct::SharedDouble::operator++()
{
    mMutex.lock();
	mVal += 1.0;
	mMutex.unlock();
}

void sgct::SharedDouble::operator--()
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


sgct::SharedInt::SharedInt()
{
	mVal = 0;
}

sgct::SharedInt::SharedInt(int val)
{
	mVal = val;
}

int sgct::SharedInt::getVal()
{
	int tmpVal;

	mMutex.lock();
	tmpVal = mVal;
	mMutex.unlock();

	return tmpVal;
}

void sgct::SharedInt::setVal(int val)
{
	mMutex.lock();
	mVal = val;
	mMutex.unlock();
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

sgct::SharedShort::SharedShort()
{
	mVal = 0;
}

sgct::SharedShort::SharedShort(short val)
{
	mVal = val;
}

short sgct::SharedShort::getVal()
{
	short tmpVal;

	mMutex.lock();
	tmpVal = mVal;
	mMutex.unlock();

	return tmpVal;
}

void sgct::SharedShort::setVal(short val)
{
	mMutex.lock();
	mVal = val;
	mMutex.unlock();
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
