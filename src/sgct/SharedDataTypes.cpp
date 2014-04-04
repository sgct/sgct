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

float sgct::SharedFloat::getVal()
{
	float tmpVal;

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	tmpVal = mVal;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );

	return tmpVal;
}

void sgct::SharedFloat::setVal(float val)
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mVal = val;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
}

sgct::SharedDouble::SharedDouble()
{
	mVal = 0.0;
}

sgct::SharedDouble::SharedDouble(double val)
{
	mVal = val;
}

double sgct::SharedDouble::getVal()
{
	double tmpVal;

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	tmpVal = mVal;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );

	return tmpVal;
}

void sgct::SharedDouble::setVal(double val)
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mVal = val;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
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

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	tmpVal = mVal;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );

	return tmpVal;
}

void sgct::SharedInt::setVal(int val)
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mVal = val;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
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

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	tmpVal = mVal;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );

	return tmpVal;
}

void sgct::SharedUChar::setVal(unsigned char val)
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mVal = val;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
}

sgct::SharedBool::SharedBool()
{
	mVal = false;
}

sgct::SharedBool::SharedBool(bool val)
{
	mVal = val;
}

bool sgct::SharedBool::getVal()
{
	bool tmpVal;

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	tmpVal = mVal;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );

	return tmpVal;
}

void sgct::SharedBool::setVal(bool val)
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mVal = val;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
}

void sgct::SharedBool::toggle()
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mVal = !mVal;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
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

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	tmpVal = mVal;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );

	return tmpVal;
}

void sgct::SharedShort::setVal(short val)
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mVal = val;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
}

sgct::SharedString::SharedString()
{
	;
}

sgct::SharedString::SharedString(const std::string & str)
{
	mStr.assign(str);
}

std::string sgct::SharedString::getVal()
{
	std::string tmpStr;

	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	tmpStr.assign( mStr );
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );

	return tmpStr;
}

void sgct::SharedString::setVal(const std::string & str)
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mStr.assign(str);
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
}

void sgct::SharedString::clear()
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mStr.clear();
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
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
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	tmpT = mVal;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
	return tmpT;
}

template <class T>
void sgct::SharedObject<T>::setVal(T val)
{
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::SharedVariableMutex );
	mVal = val;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::SharedVariableMutex );
}
*/
