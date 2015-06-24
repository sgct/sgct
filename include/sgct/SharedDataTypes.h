/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SHARED_DATA_TYPES
#define _SHARED_DATA_TYPES

#include <stdint.h>
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
		void operator++(int);
		void operator--(int);

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
        void operator++(int);
        void operator--(int);
        
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
	Mutex protected long for multi-thread data sharing
	*/
	class SharedInt64
	{
	public:
		SharedInt64();
		SharedInt64(int64_t val);
		SharedInt64(const SharedInt64 & si);

		int64_t getVal();
		void setVal(int64_t val);

		void operator=(const SharedInt64 & si);
		void operator=(const int64_t & val);
		void operator+=(const int64_t & val);
		void operator-=(const int64_t & val);
		void operator*=(const int64_t & val);
		void operator/=(const int64_t & val);
		void operator++(int);
		void operator--(int);

		bool operator<(const int64_t & val);
		bool operator<=(const int64_t & val);
		bool operator>(const int64_t & val);
		bool operator>=(const int64_t & val);
		bool operator==(const int64_t & val);
		bool operator!=(const int64_t & val);

		int64_t operator+(const int64_t & val);
		int64_t operator-(const int64_t & val);
		int64_t operator*(const int64_t & val);
		int64_t operator/(const int64_t & val);

	private:
		int64_t mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected int for multi-thread data sharing
	*/
	class SharedInt32
	{
	public:
		SharedInt32();
		SharedInt32(int32_t val);
		SharedInt32(const SharedInt32 & si);

		int32_t getVal();
		void setVal(int32_t val);

		void operator=(const SharedInt32 & si);
		void operator=(const int32_t & val);
		void operator+=(const int32_t & val);
		void operator-=(const int32_t & val);
		void operator*=(const int32_t & val);
		void operator/=(const int32_t & val);
		void operator++(int);
		void operator--(int);

		bool operator<(const int32_t & val);
		bool operator<=(const int32_t & val);
		bool operator>(const int32_t & val);
		bool operator>=(const int32_t & val);
		bool operator==(const int32_t & val);
		bool operator!=(const int32_t & val);

		int32_t operator+(const int32_t & val);
		int32_t operator-(const int32_t & val);
		int32_t operator*(const int32_t & val);
		int32_t operator/(const int32_t & val);

	private:
		int32_t mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected short/int16 for multi-thread data sharing
	*/
	class SharedInt16
	{
	public:
		SharedInt16();
		SharedInt16(int16_t val);
		SharedInt16(const SharedInt16 & si);

		int16_t getVal();
		void setVal(int16_t val);

		void operator=(const SharedInt16 & si);
		void operator=(const int16_t & val);
		void operator+=(const int16_t & val);
		void operator-=(const int16_t & val);
		void operator*=(const int16_t & val);
		void operator/=(const int16_t & val);
		void operator++(int);
		void operator--(int);

		bool operator<(const int16_t & val);
		bool operator<=(const int16_t & val);
		bool operator>(const int16_t & val);
		bool operator>=(const int16_t & val);
		bool operator==(const int16_t & val);
		bool operator!=(const int16_t & val);

		int16_t operator+(const int16_t & val);
		int16_t operator-(const int16_t & val);
		int16_t operator*(const int16_t & val);
		int16_t operator/(const int16_t & val);

	private:
		int16_t mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected int8 for multi-thread data sharing
	*/
	class SharedInt8
	{
	public:
		SharedInt8();
		SharedInt8(int8_t val);
		SharedInt8(const SharedInt8 & si);

		int8_t getVal();
		void setVal(int8_t val);

		void operator=(const SharedInt8 & si);
		void operator=(const int8_t & val);
		void operator+=(const int8_t & val);
		void operator-=(const int8_t & val);
		void operator*=(const int8_t & val);
		void operator/=(const int8_t & val);
		void operator++(int);
		void operator--(int);

		bool operator<(const int8_t & val);
		bool operator<=(const int8_t & val);
		bool operator>(const int8_t & val);
		bool operator>=(const int8_t & val);
		bool operator==(const int8_t & val);
		bool operator!=(const int8_t & val);

		int8_t operator+(const int8_t & val);
		int8_t operator-(const int8_t & val);
		int8_t operator*(const int8_t & val);
		int8_t operator/(const int8_t & val);

	private:
		int8_t mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected unsigned long for multi-thread data sharing
	*/
	class SharedUInt64
	{
	public:
		SharedUInt64();
		SharedUInt64(uint64_t val);
		SharedUInt64(const SharedUInt64 & si);

		uint64_t getVal();
		void setVal(uint64_t val);

		void operator=(const SharedUInt64 & si);
		void operator=(const uint64_t & val);
		void operator+=(const uint64_t & val);
		void operator-=(const uint64_t & val);
		void operator*=(const uint64_t & val);
		void operator/=(const uint64_t & val);
		void operator++(int);
		void operator--(int);

		bool operator<(const uint64_t & val);
		bool operator<=(const uint64_t & val);
		bool operator>(const uint64_t & val);
		bool operator>=(const uint64_t & val);
		bool operator==(const uint64_t & val);
		bool operator!=(const uint64_t & val);

		uint64_t operator+(const uint64_t & val);
		uint64_t operator-(const uint64_t & val);
		uint64_t operator*(const uint64_t & val);
		uint64_t operator/(const uint64_t & val);

	private:
		uint64_t mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected unsigned int for multi-thread data sharing
	*/
	class SharedUInt32
	{
	public:
		SharedUInt32();
		SharedUInt32(uint32_t val);
		SharedUInt32(const SharedUInt32 & si);

		uint32_t getVal();
		void setVal(uint32_t val);

		void operator=(const SharedUInt32 & si);
		void operator=(const uint32_t & val);
		void operator+=(const uint32_t & val);
		void operator-=(const uint32_t & val);
		void operator*=(const uint32_t & val);
		void operator/=(const uint32_t & val);
		void operator++(int);
		void operator--(int);

		bool operator<(const uint32_t & val);
		bool operator<=(const uint32_t & val);
		bool operator>(const uint32_t & val);
		bool operator>=(const uint32_t & val);
		bool operator==(const uint32_t & val);
		bool operator!=(const uint32_t & val);

		uint32_t operator+(const uint32_t & val);
		uint32_t operator-(const uint32_t & val);
		uint32_t operator*(const uint32_t & val);
		uint32_t operator/(const uint32_t & val);

	private:
		uint32_t mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected unsigned short/uint16 for multi-thread data sharing
	*/
	class SharedUInt16
	{
	public:
		SharedUInt16();
		SharedUInt16(uint16_t val);
		SharedUInt16(const SharedUInt16 & si);

		uint16_t getVal();
		void setVal(uint16_t val);

		void operator=(const SharedUInt16 & si);
		void operator=(const uint16_t & val);
		void operator+=(const uint16_t & val);
		void operator-=(const uint16_t & val);
		void operator*=(const uint16_t & val);
		void operator/=(const uint16_t & val);
		void operator++(int);
		void operator--(int);

		bool operator<(const uint16_t & val);
		bool operator<=(const uint16_t & val);
		bool operator>(const uint16_t & val);
		bool operator>=(const uint16_t & val);
		bool operator==(const uint16_t & val);
		bool operator!=(const uint16_t & val);

		uint16_t operator+(const uint16_t & val);
		uint16_t operator-(const uint16_t & val);
		uint16_t operator*(const uint16_t & val);
		uint16_t operator/(const uint16_t & val);

	private:
		uint16_t mVal;
		tthread::mutex mMutex;
	};

	/*!
	Mutex protected unsigned uint8 for multi-thread data sharing
	*/
	class SharedUInt8
	{
	public:
		SharedUInt8();
		SharedUInt8(uint8_t val);
		SharedUInt8(const SharedUInt8 & si);

		uint8_t getVal();
		void setVal(uint8_t val);

		void operator=(const SharedUInt8 & si);
		void operator=(const uint8_t & val);
		void operator+=(const uint8_t & val);
		void operator-=(const uint8_t & val);
		void operator*=(const uint8_t & val);
		void operator/=(const uint8_t & val);
		void operator++(int);
		void operator--(int);

		bool operator<(const uint8_t & val);
		bool operator<=(const uint8_t & val);
		bool operator>(const uint8_t & val);
		bool operator>=(const uint8_t & val);
		bool operator==(const uint8_t & val);
		bool operator!=(const uint8_t & val);

		uint8_t operator+(const uint8_t & val);
		uint8_t operator-(const uint8_t & val);
		uint8_t operator*(const uint8_t & val);
		uint8_t operator/(const uint8_t & val);

	private:
		uint8_t mVal;
		tthread::mutex mMutex;
	};

	//backwards compability
	typedef SharedInt16 SharedShort;
	typedef SharedInt32 SharedInt;

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
