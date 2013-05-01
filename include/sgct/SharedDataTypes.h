/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SHARED_DATA_TYPES
#define _SHARED_DATA_TYPES

#include <string>

namespace sgct //simple graphics cluster toolkit
{
	/*!
	Mutex protected float for multi-thread data sharing
	*/
	class SharedFloat
	{
	public:
		SharedFloat();
		SharedFloat(float val);
		float getVal();
		void setVal(float val);

	private:
		SharedFloat( const SharedFloat & sf );
		const SharedFloat & operator=(const SharedFloat & sf );
		float mVal;
	};

	/*!
	Mutex protected double for multi-thread data sharing
	*/
	class SharedDouble
	{
	public:
		SharedDouble();
		SharedDouble(double val);
		double getVal();
		void setVal(double val);

	private:
		SharedDouble( const SharedDouble & sd );
		const SharedDouble & operator=(const SharedDouble & sd );
		double mVal;
	};

	/*!
	Mutex protected int for multi-thread data sharing
	*/
	class SharedInt
	{
	public:
		SharedInt();
		SharedInt(int val);
		int getVal();
		void setVal(int val);

	private:
		SharedInt( const SharedInt & si );
		const SharedInt & operator=(const SharedInt & si );
		int mVal;
	};

	/*!
	Mutex protected unsigned char for multi-thread data sharing
	*/
	class SharedUChar
	{
	public:
		SharedUChar();
		SharedUChar(unsigned char val);
		unsigned char getVal();
		void setVal(unsigned char val);

	private:
		SharedUChar( const SharedUChar & suc );
		const SharedUChar & operator=(const SharedUChar & suc );
		unsigned char mVal;
	};

	/*!
	Mutex protected bool for multi-thread data sharing
	*/
	class SharedBool
	{
	public:
		SharedBool();
		SharedBool(bool val);
		bool getVal();
		void setVal(bool val);
		void toggle();

	private:
		SharedBool( const SharedBool & sb );
		const SharedBool & operator=(const SharedBool & sb );
		bool mVal;
	};

	/*!
	Mutex protected short/int16 for multi-thread data sharing
	*/
	class SharedShort
	{
	public:
		SharedShort();
		SharedShort(short val);
		short getVal();
		void setVal(short val);

	private:
		SharedShort( const SharedShort & ss );
		const SharedShort & operator=(const SharedShort & ss );
		short mVal;
	};

	/*!
	Mutex protected std::string for multi-thread data sharing
	*/
	class SharedString
	{
	public:
		SharedString();
		SharedString(const std::string & str);
		std::string getVal();
		void setVal(const std::string & str);

	private:
		SharedString( const SharedString & ss );
		const SharedString & operator=(const SharedString & ss );
		std::string mStr;
	};

	/*!
	Mutex protected template for multi-thread data sharing
	*/
	template <class T>
	class SharedObject
	{
	public:
		SharedObject() {;}
		SharedObject(T val) { mVal = val; }

		T getVal()
		{
			T tmpT;
			SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::SharedDataMutex );
			tmpT = mVal;
			SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::SharedDataMutex );
			return tmpT;
		}

		void setVal(T val)
		{
			SGCTMutexManager::Instance()->lockMutex( SGCTMutexManager::SharedDataMutex );
			mVal = val;
			SGCTMutexManager::Instance()->unlockMutex( SGCTMutexManager::SharedDataMutex );
		}

	private:
		SharedObject( const SharedObject & so );
		const SharedObject & operator=(const SharedObject & so );
		T mVal;
	};
}

#endif