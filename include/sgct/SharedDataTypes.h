/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SHARED_DATA_TYPES
#define _SHARED_DATA_TYPES

#include <string>
#include <vector>
#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/tinythread.h"
#else
#include <tinythread.h>
#endif

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
		SharedFloat(const SharedFloat & sf);

		float getVal();
		void setVal(float val);

		void operator=(const SharedFloat & sf);
		void operator=(const float & val);
		void operator+=(const float & val);
		void operator-=(const float & val);
		void operator*=(const float & val);
		void operator/=(const float & val);
		void operator++();
		void operator--();

		bool operator<(const float & val);
		bool operator<=(const float & val);
		bool operator>(const float & val);
		bool operator>=(const float & val);
		bool operator==(const float & val);
		bool operator!=(const float & val);

		float operator+(const float & val);
		float operator-(const float & val);
		float operator*(const float & val);
		float operator/(const float & val);

	private:
		float mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected double for multi-thread data sharing
	*/
	class SharedDouble
	{
	public:
		SharedDouble();
		SharedDouble( double val );
        SharedDouble( const SharedDouble & sd );
        
		double getVal();
		void setVal(double val);
        
		void operator=(const SharedDouble & sd);
        void operator=( const double & val );
        void operator+=( const double & val );
        void operator-=( const double & val );
        void operator*=( const double & val );
        void operator/=( const double & val );
        void operator++();
        void operator--();
        
        bool operator<( const double & val );
        bool operator<=( const double & val );
        bool operator>( const double & val );
        bool operator>=( const double & val );
        bool operator==( const double & val );
        bool operator!=( const double & val );
        
        double operator+( const double & val );
        double operator-( const double & val );
        double operator*( const double & val );
        double operator/( const double & val );
        
	private:
		double mVal;
		tthread::mutex mMutex;
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
		tthread::mutex mMutex;
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
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected bool for multi-thread data sharing
	*/
	class SharedBool
	{
	public:
		SharedBool();
		SharedBool(bool val);
        SharedBool( const SharedBool & sb );
		
        bool getVal();
		void setVal(bool val);
		void toggle();
        
        void operator=( const bool & val );
		void operator=(const SharedBool & sb);
        bool operator==( const bool & val );
        bool operator!=( const bool & val );

	private:
		bool mVal;
		tthread::mutex mMutex;
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
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected std::string for multi-thread data sharing
	*/
	class SharedString
	{
	public:
		SharedString();
		SharedString(const std::string & str);
		SharedString(const SharedString & ss);

		std::string getVal();
		void setVal(const std::string & str);
		void clear();

		void operator=(const std::string & str);
		void operator=(const SharedString & ss);

	private:
		std::string mStr;
		tthread::mutex mMutex;
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
			mMutex.lock();
			tmpT = mVal;
			mMutex.unlock();
			return tmpT;
		}

		void setVal(T val)
		{
			mMutex.lock();
			mVal = val;
			mMutex.unlock();
		}

	private:
		SharedObject( const SharedObject & so );
		const SharedObject & operator=(const SharedObject & so );
		T mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected std::vector template for multi-thread data sharing
	*/
	template <class T>
	class SharedVector
	{
	public:
		SharedVector() {;}
		SharedVector(std::size_t size) { mVector.reserve(size); }

		T getValAt(std::size_t index)
		{
			T tmpT;
			mMutex.lock();
			tmpT = mVector[ index ];
			mMutex.unlock();
			return tmpT;
		}

		std::vector<T> getVal()
		{
			std::vector<T> mCopy;
			mMutex.lock();
			mCopy = mVector;
			mMutex.unlock();
			return mCopy;
		}

		void setValAt(std::size_t index, T val)
		{
			mMutex.lock();
			mVector[ index ] = val;
			mMutex.unlock();
		}

		void addVal(T val)
		{
			mMutex.lock();
			mVector.push_back(val);
			mMutex.unlock();
		}

		void setVal( std::vector<T> mCopy )
		{
			mMutex.lock();
			mVector.assign(mCopy.begin(), mCopy.end());
			mMutex.unlock();
		}

		void clear()
		{
			mMutex.lock();
			mVector.clear();
			mMutex.unlock();
		}

		std::size_t getSize()
		{
			std::size_t size = 0;
			mMutex.lock();
			size = mVector.size();
			mMutex.unlock();
			return size;
		}

	private:
		SharedVector( const SharedVector & sv );
		const SharedVector & operator=(const SharedVector & sv );
		std::vector<T> mVector;
		tthread::mutex mMutex;
	};
}

#endif
