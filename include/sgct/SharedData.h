/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SHARED_DATA
#define _SHARED_DATA

#include <stddef.h> //get definition for NULL
#include <vector>
#include <string>
#include "SharedDataTypes.h"

namespace sgct //simple graphics cluster toolkit
{

/*!
This class shares application data between nodes in a cluster where the master encodes and transmits the data and the slaves receives and decode the data.
If a large number of strings are used for the synchronization then the data can be compressed using the setCompression function.
The process of synchronization is serial which means that the order of encoding must be the same as in decoding.
*/
class SharedData
{
public:
	/*! Get the SharedData instance */
	static SharedData * instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new SharedData();
		}

		return mInstance;
	}

	/*! Destroy the SharedData */
	static void destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	/*!
		Compression levels 1-9.
		-1 = Default compression
		0 = No compression
		1 = Best speed
		9 = Best compression
	*/
	void setCompression(bool state, int level = 1);
	/*! Get the compresson ratio:
	\f[ratio = \frac{compressed data size + Huffman tree}{original data size}\f]
		If the ratio is larger than 1.0 then there is no use for using compression.
	*/
	inline float getCompressionRatio() { return mCompressionRatio; }

	template<class T>
	void writeObj(SharedObject<T> * sobj);
	void writeFloat(SharedFloat * sf);
	void writeDouble(SharedDouble * sd);
	void writeInt(SharedInt * si);
	void writeUChar(SharedUChar * suc);
	void writeBool(SharedBool * sb);
	void writeShort(SharedShort * ss);
    void writeString(SharedString * ss);
	template<class T>
	void writeVector(SharedVector<T> * vector);

	template<class T>
	void readObj(SharedObject<T> * sobj);
	void readFloat(SharedFloat * f);
	void readDouble(SharedDouble * d);
	void readInt(SharedInt * si);
	void readUChar(SharedUChar * suc);
	void readBool(SharedBool * sb);
	void readShort(SharedShort * ss);
    void readString(SharedString * ss);
	template<class T>
	void readVector(SharedVector<T> * vector);

	void setEncodeFunction( void(*fnPtr)(void) );
	void setDecodeFunction( void(*fnPtr)(void) );

	/*
		Compression error/info codes:

		Z_OK            0
		Z_STREAM_END    1
		Z_NEED_DICT     2
		Z_ERRNO        (-1)
		Z_STREAM_ERROR (-2)
		Z_DATA_ERROR   (-3)
		Z_MEM_ERROR    (-4)
		Z_BUF_ERROR    (-5)
		Z_VERSION_ERROR (-6)
	*/
	void encode();
	void decode(const char * receivedData, int receivedlength, int clientIndex);

	inline unsigned char * getDataBlock() { return &dataBlock[0]; }
	inline std::size_t getDataSize() { return dataBlock.size(); }
	inline std::size_t getBufferSize() { return dataBlock.capacity(); }
	std::size_t getUserDataSize();

private:
	SharedData();
	~SharedData();

	// Don't implement these, should give compile warning if used
	SharedData( const SharedData & tm );
	const SharedData & operator=(const SharedData & rhs );

	void writeUCharArray(unsigned char * c, std::size_t length);
	unsigned char * readUCharArray(std::size_t length);

	void writeSize( std::size_t size );
	std::size_t readSize();

private:
	//function pointers
	void (*mEncodeFn) (void);
	void (*mDecodeFn) (void);

	static SharedData * mInstance;
	std::vector<unsigned char> dataBlock;
	std::vector<unsigned char> dataBlockToCompress;
	std::vector<unsigned char> * currentStorage;
	unsigned char * mCompressedBuffer;
    std::size_t mCompressedBufferSize;
	std::size_t mCompressedSize;
	unsigned char * headerSpace;
	unsigned int pos;
	int mCompressionLevel;
	float mCompressionRatio;
	bool mUseCompression;
};

template <class T>
void SharedData::writeObj( SharedObject<T> * sobj )
{
	T val = sobj->getVal();
	unsigned char *p = (unsigned char *)&val;
    std::size_t size = sizeof(val);
    writeUCharArray(p, size);
}

template<class T>
void SharedData::readObj(SharedObject<T> * sobj)
{
	std::size_t size = sizeof(T);
    unsigned char* data = new unsigned char[size];
	unsigned char* c = readUCharArray(size);

	for(std::size_t i = 0; i < size; i++)
		data[i] = c[i];

    T val = *reinterpret_cast<T*>(data);
    delete[] data;
    
	sobj->setVal( val );
}

template<class T>
void SharedData::writeVector(SharedVector<T> * vector)
{
	std::vector<T> tmpVec = vector->getVal();

	unsigned char *p = reinterpret_cast<unsigned char *>(&tmpVec[0]);
    std::size_t element_size = sizeof(T);
    
	writeSize( tmpVec.size() );
	writeUCharArray(p, element_size * tmpVec.size());
}

template<class T>
void SharedData::readVector(SharedVector<T> * vector)
{
	std::size_t size = readSize();
	std::size_t totalSize = size * sizeof(T);
    unsigned char* data = new unsigned char[ totalSize ];
	unsigned char* c = readUCharArray( totalSize );

	for(std::size_t i = 0; i < totalSize; i++)
		data[i] = c[i];

	std::vector<T> tmpVec;
	tmpVec.insert( tmpVec.begin(), reinterpret_cast<T*>(data), reinterpret_cast<T*>(data)+size);

    vector->setVal( tmpVec );
    delete[] data;
}

}

#endif
