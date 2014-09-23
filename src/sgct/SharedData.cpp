/*************************************************************************
Copyright (c) 2012-2014 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/SharedData.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/MessageHandler.h"
#ifndef SGCT_DONT_USE_EXTERNAL
#include "../include/external/zlib.h"
#else
#include <zlib.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace sgct;

#define DEFAULT_SIZE 1024

SharedData * SharedData::mInstance = NULL;

SharedData::SharedData()
{
	mEncodeFn = NULL;
	mDecodeFn = NULL;

	mCompressedBuffer = new (std::nothrow) unsigned char[DEFAULT_SIZE];
	mCompressedBufferSize = DEFAULT_SIZE;

	dataBlock.reserve(DEFAULT_SIZE);
	dataBlockToCompress.reserve(DEFAULT_SIZE);

	mUseCompression = false;
	mCompressionRatio = 1.0f;
	mCompressionLevel = Z_BEST_SPEED;
	mCompressedSize = 0;

	if(mUseCompression)
		currentStorage = &dataBlockToCompress;
	else
		currentStorage = &dataBlock;

	headerSpace		= new (std::nothrow) unsigned char[sgct_core::SGCTNetwork::mHeaderSize];

	if( !mCompressedBuffer || !headerSpace )
	{
		fprintf(stderr, "Fatal error! Failed to allocate SharedData memory pool.\n");
		return;
	}
	
	for(unsigned int i=0; i<sgct_core::SGCTNetwork::mHeaderSize; i++)
		headerSpace[i] = sgct_core::SGCTNetwork::DataId;
}

SharedData::~SharedData()
{
	delete [] headerSpace;
    headerSpace = NULL;

	delete [] mCompressedBuffer;
	mCompressedBuffer = NULL;

	dataBlock.clear();
	dataBlockToCompress.clear();
}

void SharedData::setCompression(bool state, int level)
{
	mUseCompression = state;
	mCompressionLevel = level;

	if(mUseCompression)
		currentStorage = &dataBlockToCompress;
	else
	{
		currentStorage = &dataBlock;
		mCompressionRatio = 1.0f;
	}
}

/*!
	Returns the data size used by the user. Header size is excluded.
*/
std::size_t SharedData::getUserDataSize()
{
	return mUseCompression ? mCompressedSize : (dataBlock.size() - sgct_core::SGCTNetwork::mHeaderSize);
}

/*!
Set the encode callback.

Sample of a encode function:
\code{.cpp}
void myEncodeFun()
{
	sgct::SharedData::instance()->writeDouble( curr_time );
}
\endcode
*/
void SharedData::setEncodeFunction(void(*fnPtr)(void))
{
	mEncodeFn = fnPtr;
}

/*!
Set the decoder callback.

Sample of a decode function:
\code{.cpp}
void myDecodeFun()
{
	curr_time = sgct::SharedData::instance()->readDouble();
}
\endcode
*/
void SharedData::setDecodeFunction(void(*fnPtr)(void))
{
	mDecodeFn = fnPtr;
}

/*!
This fuction is called internally by SGCT and shouldn't be used by the user.
*/
void SharedData::decode(const char * receivedData, int receivedlength, int clientIndex)
{
#ifdef __SGCT_NETWORK_DEBUG__
	MessageHandler::instance()->printDebug( sgct::MessageHandler::NOTIFY_ALL, "SharedData::decode\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );

    //reset
    pos = 0;
    dataBlock.clear();

    if(mUseCompression)
    {
        //get original size (fist 4 bytes after header)
        union
        {
            unsigned int ui;
            unsigned char c[4];
        } cui;

        cui.c[0] = receivedData[0];
        cui.c[1] = receivedData[1];
        cui.c[2] = receivedData[2];
        cui.c[3] = receivedData[3];

        //re-allocatate if needed
        if(mCompressedBufferSize < cui.ui)
        {
            delete [] mCompressedBuffer;
			mCompressedBuffer = new (std::nothrow) unsigned char[ cui.ui ];
            mCompressedBufferSize = cui.ui;
        }

        //get compressed data block size
        cui.c[0] = receivedData[4];
        cui.c[1] = receivedData[5];
        cui.c[2] = receivedData[6];
        cui.c[3] = receivedData[7];
        mCompressedSize = cui.ui;

        uLongf data_size = static_cast<uLongf>(mCompressedSize);
        uLongf uncompressed_size = static_cast<uLongf>(mCompressedBufferSize);
        const Bytef * data = reinterpret_cast<const Bytef *>(receivedData + 8);

        int err = uncompress(
            mCompressedBuffer,
            &uncompressed_size,
            data,
            data_size);

        if(err == Z_OK)
        {
            //re-allocate buffer if needed
            if( uncompressed_size > dataBlock.capacity() )
                dataBlock.reserve(uncompressed_size);
            dataBlock.insert(dataBlock.end(), mCompressedBuffer, mCompressedBuffer + uncompressed_size);

            mCompressionRatio = static_cast<float>(mCompressedSize) / static_cast<float>(uncompressed_size);
        }
        else
        {
            SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
            MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SharedData: Failed to un-compress data (error: %d). Received %d bytes.\n", err, receivedlength);
            return;
        }
    }
    else //not using compression
    {
        if( receivedlength > static_cast<int>(dataBlock.capacity()) )
            dataBlock.reserve(receivedlength);
        dataBlock.insert(dataBlock.end(), receivedData, receivedData+receivedlength);
    }

    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

    if( mDecodeFn != NULL )
        mDecodeFn();
}

/*!
This fuction is called internally by SGCT and shouldn't be used by the user.
*/
void SharedData::encode()
{
#ifdef __SGCT_NETWORK_DEBUG__
	MessageHandler::instance()->printDebug( sgct::MessageHandler::NOTIFY_ALL, "SharedData::encode\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	dataBlock.clear();
	if(mUseCompression)
		dataBlockToCompress.clear();

	//reserve header space
	dataBlock.insert( dataBlock.begin(), headerSpace, headerSpace+sgct_core::SGCTNetwork::mHeaderSize );

	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	if( mEncodeFn != NULL )
		mEncodeFn();

	if(mUseCompression && dataBlockToCompress.size() > 0)
	{
		SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );

		//re-allocatate if needed
		if(mCompressedBufferSize < dataBlockToCompress.size())
		{
			delete [] mCompressedBuffer;
			mCompressedBuffer = new (std::nothrow) unsigned char[ dataBlockToCompress.size() ];
			mCompressedBufferSize = dataBlockToCompress.size();
		}

		uLongf compressed_size = static_cast<uLongf>(mCompressedBufferSize);
		uLongf data_size = static_cast<uLongf>(dataBlockToCompress.size());
		int err = compress2(
			mCompressedBuffer,
			&compressed_size,
			&dataBlockToCompress[0],
			data_size,
			mCompressionLevel);

		if(err == Z_OK)
		{
			//add size of uncompressed data
			std::size_t originalSize = dataBlockToCompress.size();
			unsigned char *p = (unsigned char *)&originalSize;
			dataBlock.insert( dataBlock.end(), p, p+4);

			//add size of compressed data (needed for cross-platform compability)
			mCompressedSize = static_cast<unsigned int>(compressed_size);
			unsigned char *p2 = (unsigned char *)&mCompressedSize;
			dataBlock.insert( dataBlock.end(), p2, p2+4);

			mCompressionRatio = static_cast<float>(mCompressedSize) / static_cast<float>(originalSize);

			//add the compressed block
			dataBlock.insert( dataBlock.end(), mCompressedBuffer, mCompressedBuffer + compressed_size );
		}
		else
		{
			SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
			MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "SharedData: Failed to compress data (error %d).\n", err);
			return;
		}

		SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	}
}

void SharedData::writeFloat(SharedFloat * sf)
{
#ifdef __SGCT_NETWORK_DEBUG__    
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeFloat\nFloat = %f", sf->getVal());
#endif

	float val = sf->getVal();
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	unsigned char *p = (unsigned char *)&val;
	(*currentStorage).insert( (*currentStorage).end(), p, p+4);
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeDouble(SharedDouble * sd)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeDouble\nDouble = %f\n", sd->getVal());
#endif

	double val = sd->getVal();
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	unsigned char *p = (unsigned char *)&val;
	(*currentStorage).insert( (*currentStorage).end(), p, p+8);
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeInt(SharedInt * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeInt\nInt = %d\n", si->getVal());
#endif

	int val = si->getVal();
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	unsigned char *p = (unsigned char *)&val;
	(*currentStorage).insert( (*currentStorage).end(), p, p+4);
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeUChar(SharedUChar * suc)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeUChar\n");
#endif

	unsigned char val = suc->getVal();
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	unsigned char *p = &val;
	(*currentStorage).push_back(*p);
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeBool(SharedBool * sb)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeBool\n");
#endif
	
	bool val = sb->getVal();
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	if( val )
		(*currentStorage).push_back(1);
	else
		(*currentStorage).push_back(0);
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeShort(SharedShort * ss)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeShort\n");
#endif

	short val = ss->getVal();
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	unsigned char *p = (unsigned char *)&val;
	(*currentStorage).insert( (*currentStorage).end(), p, p+2);
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeString(SharedString * ss)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeString\n");
#endif
    
	std::string tmpStr( ss->getVal() );
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	const char* stringData = tmpStr.c_str();
    std::size_t length = tmpStr.size() + 1;  // +1 for the \0 character
    unsigned char *p = (unsigned char *)&length;
    (*currentStorage).insert( (*currentStorage).end(), p, p+4);
    (*currentStorage).insert( (*currentStorage).end(), stringData, stringData+length);
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeUCharArray(unsigned char * c, std::size_t length)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeUCharArray\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    (*currentStorage).insert( (*currentStorage).end(), c, c+length);
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeSize( std::size_t size )
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeSize\n");
#endif
    
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	unsigned char *p = (unsigned char *)&size;
	(*currentStorage).insert( (*currentStorage).end(), p, p + sizeof( std::size_t ));
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::readFloat(SharedFloat * sf)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readFloat\n");
#endif
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	union
	{
		float f;
		unsigned char c[4];
	} cf;

	cf.c[0] = dataBlock[pos];
	cf.c[1] = dataBlock[pos+1];
	cf.c[2] = dataBlock[pos+2];
	cf.c[3] = dataBlock[pos+3];
	pos += 4;
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Float = %f\n", cf.f);
#endif

	sf->setVal( cf.f );
}

void SharedData::readDouble(SharedDouble * sd)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readDouble\n");
#endif
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	union
	{
		double d;
		unsigned char c[8];
	} cf;

	cf.c[0] = dataBlock[pos];
	cf.c[1] = dataBlock[pos+1];
	cf.c[2] = dataBlock[pos+2];
	cf.c[3] = dataBlock[pos+3];
	cf.c[4] = dataBlock[pos+4];
	cf.c[5] = dataBlock[pos+5];
	cf.c[6] = dataBlock[pos+6];
	cf.c[7] = dataBlock[pos+7];
	pos += 8;
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Double = %f\n", cf.d);
#endif

	sd->setVal( cf.d );
}

void SharedData::readInt(SharedInt * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readInt\n");
#endif
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	union
	{
		int i;
		unsigned char c[4];
	} ci;

	ci.c[0] = dataBlock[pos];
	ci.c[1] = dataBlock[pos+1];
	ci.c[2] = dataBlock[pos+2];
	ci.c[3] = dataBlock[pos+3];
	pos += 4;
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	si->setVal( ci.i );
}

void SharedData::readUChar(SharedUChar * suc)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readUChar\n");
#endif
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	unsigned char c;
	c = dataBlock[pos];
	pos += 1;
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	suc->setVal(c);
}

void SharedData::readBool(SharedBool * sb)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readBool\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	bool b;
	b = dataBlock[pos] == 1 ? true : false;
	pos += 1;
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	sb->setVal( b );
}

void SharedData::readShort(SharedShort * ss)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readShort\n");
#endif
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	union
	{
		short s;
		unsigned char c[2];
	} cs;

	cs.c[0] = dataBlock[pos];
	cs.c[1] = dataBlock[pos+1];

	pos += 2;
	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	ss->setVal( cs.s );
}

void SharedData::readString(SharedString * ss)
{
#ifdef __SGCT_NETWORK_DEBUG__     
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readString\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    union
    {
        unsigned int i;
        unsigned char c[4];
    } ci;

    ci.c[0] = dataBlock[pos];
    ci.c[1] = dataBlock[pos+1];
    ci.c[2] = dataBlock[pos+2];
    ci.c[3] = dataBlock[pos+3];
    pos += 4;

	if( ci.i == 0 )
	{
		ss->clear();
		return;
	}

    char * stringData = new (std::nothrow) char[ ci.i ];
    if( stringData )
		memcpy(stringData, &dataBlock[pos], ci.i);

    pos += ci.i;
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    
	ss->setVal(stringData);

	delete [] stringData;
	stringData = NULL;
}

unsigned char * SharedData::readUCharArray(std::size_t length)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readUCharArray\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	unsigned char * p = &dataBlock[pos];
    pos += static_cast<unsigned int>(length);

	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

    return p;
}

std::size_t SharedData::readSize()
{
#ifdef __SGCT_NETWORK_DEBUG__ 
	MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readSize\n");
#endif

	std::size_t size;
	
	SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
	union
	{
		std::size_t s;
		unsigned char c[ sizeof(std::size_t) ];
	} cs;

	for(unsigned int i=0; i<sizeof(std::size_t); i++)
		cs.c[i] = dataBlock[pos+i];

	pos += sizeof(std::size_t);
	
	size = cs.s;

	SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

	return size;
}
