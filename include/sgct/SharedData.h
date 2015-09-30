/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SHARED_DATA
#define _SHARED_DATA

#include <stddef.h> //get definition for NULL
#include <vector>
#include <string>
#include <string.h> //for memcpy
#include "SharedDataTypes.h"
#include "SGCTMutexManager.h"

#ifndef SGCT_DEPRECATED
#if defined(_MSC_VER) //if visual studio
	#define SGCT_DEPRECATED __declspec(deprecated)
#else
	#define SGCT_DEPRECATED __attribute__((deprecated))
#endif
#endif

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

	void setCompression(bool state, int level = 1);
	/*! Get the compresson ratio:
	\n
	ratio = (compressed data size + Huffman tree)/(original data size)
	\n
	If the ratio is larger than 1.0 then there is no use for using compression.
	*/
	inline float getCompressionRatio() { return mCompressionRatio; }

	template<class T>
	void writeObj(SharedObject<T> * sobj);
	void writeFloat(SharedFloat * sf);
	void writeDouble(SharedDouble * sd);
	SGCT_DEPRECATED void writeInt(SharedInt * si); //deprecated
	SGCT_DEPRECATED void writeShort(SharedShort * si); //deprecated
	
	void writeInt64(SharedInt64 * si);
	void writeInt32(SharedInt32 * si);
	void writeInt16(SharedInt16 * si);
	void writeInt8(SharedInt8 * si);
	
	void writeUInt64(SharedUInt64 * si);
	void writeUInt32(SharedUInt32 * si);
	void writeUInt16(SharedUInt16 * si);
	void writeUInt8(SharedUInt8 * si);
	
	void writeUChar(SharedUChar * suc);
	void writeBool(SharedBool * sb);
    void writeString(SharedString * ss);
	template<class T>
	void writeVector(SharedVector<T> * vector);

	template<class T>
	void readObj(SharedObject<T> * sobj);
	void readFloat(SharedFloat * f);
	void readDouble(SharedDouble * d);
	SGCT_DEPRECATED void readInt(SharedInt * si); //deprecated
	SGCT_DEPRECATED void readShort(SharedShort * ss); //deprecated
	
	void readInt64(SharedInt64 * si);
	void readInt32(SharedInt32 * si);
	void readInt16(SharedInt16 * si);
	void readInt8(SharedInt8 * si);
	
	void readUInt64(SharedUInt64 * si);
	void readUInt32(SharedUInt32 * si);
	void readUInt16(SharedUInt16 * si);
	void readUInt8(SharedUInt8 * si);
	
	void readUChar(SharedUChar * suc);
	void readBool(SharedBool * sb);
    void readString(SharedString * ss);
	template<class T>
	void readVector(SharedVector<T> * vector);

	void setEncodeFunction( void(*fnPtr)(void) );
	void setDecodeFunction( void(*fnPtr)(void) );

	void encode();
	void decode(const char * receivedData, int receivedlength, int clientIndex);

    std::size_t getUserDataSize();
	inline unsigned char * getDataBlock() { return &dataBlock[0]; }
	inline std::size_t getDataSize() { return dataBlock.size(); }
	inline std::size_t getBufferSize() { return dataBlock.capacity(); }

private:
	SharedData();
	~SharedData();

	// Don't implement these, should give compile warning if used
	SharedData( const SharedData & tm );
	const SharedData & operator=(const SharedData & rhs );

	void writeUCharArray(unsigned char * c, uint32_t length);
	unsigned char * readUCharArray(uint32_t length);

	void writeSize(uint32_t size);
	uint32_t readSize();

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
	
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::DataSyncMutex);
	unsigned char *p = reinterpret_cast<unsigned char *>(&val);
	(*currentStorage).insert((*currentStorage).end(), p, p + sizeof(T));
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::DataSyncMutex);
}

template<class T>
void SharedData::readObj(SharedObject<T> * sobj)
{
	SGCTMutexManager::instance()->lockMutex(SGCTMutexManager::DataSyncMutex);
	T val = (*(reinterpret_cast<T*>(&dataBlock[pos])));
	pos += sizeof(T);
	SGCTMutexManager::instance()->unlockMutex(SGCTMutexManager::DataSyncMutex);
    
	sobj->setVal( val );
}

template<class T>
void SharedData::writeVector(SharedVector<T> * vector)
{
	std::vector<T> tmpVec = vector->getVal();

	unsigned char *p;
	p = tmpVec.size() ? reinterpret_cast<unsigned char *>(&tmpVec[0]) : NULL;
	
	uint32_t element_size = sizeof(T);
	uint32_t vector_size = static_cast<uint32_t>(tmpVec.size());

	writeSize(vector_size);
	if (p)
		writeUCharArray(p, element_size * vector_size);
}

template<class T>
void SharedData::readVector(SharedVector<T> * vector)
{
	uint32_t size = readSize();
	if(size == 0)
	{
		vector->clear();
		return;
	}

	uint32_t totalSize = size * sizeof(T);
	unsigned char* data = new unsigned char[ totalSize ];
	unsigned char* c = readUCharArray( totalSize );

	//for(std::size_t i = 0; i < totalSize; i++)
	//	data[i] = c[i];
	memcpy(data, c, totalSize);

	std::vector<T> tmpVec;
	tmpVec.insert( tmpVec.begin(), reinterpret_cast<T*>(data), reinterpret_cast<T*>(data)+size);

	vector->setVal( tmpVec );
	delete[] data;
}

}

#endif
