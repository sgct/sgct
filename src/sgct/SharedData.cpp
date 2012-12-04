/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/SharedData.h"
#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/external/zlib.h"
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

	mCompressedBuffer = reinterpret_cast<unsigned char*>( malloc(DEFAULT_SIZE) );
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

	headerSpace		= NULL;
	headerSpace		= reinterpret_cast<unsigned char*>( malloc(sgct_core::SGCTNetwork::mHeaderSize) );

	for(unsigned int i=0; i<sgct_core::SGCTNetwork::mHeaderSize; i++)
		headerSpace[i] = sgct_core::SGCTNetwork::SyncByte;
}

SharedData::~SharedData()
{
	free(headerSpace);
    headerSpace = NULL;

	free(mCompressedBuffer);
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

unsigned int SharedData::getUserDataSize()
{
	return mUseCompression ? mCompressedSize : (dataBlock.size() - sgct_core::SGCTNetwork::mHeaderSize);
}

void SharedData::setEncodeFunction(void(*fnPtr)(void))
{
	mEncodeFn = fnPtr;
}

void SharedData::setDecodeFunction(void(*fnPtr)(void))
{
	mDecodeFn = fnPtr;
}

void SharedData::decode(const char * receivedData, int receivedlength, int clientIndex)
{
    MessageHandler::Instance()->printDebug("SharedData::decode\n");
    Engine::lockMutex(sgct_core::NetworkManager::gMutex);

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
            mCompressedBuffer = reinterpret_cast<unsigned char*>( realloc(mCompressedBuffer, cui.ui) );
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
            Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
            MessageHandler::Instance()->print("SharedData: Failed to un-compress data (error: %d). Received %d bytes.\n", err, receivedlength);
            return;
        }
    }
    else //not using compression
    {
        if( receivedlength > static_cast<int>(dataBlock.capacity()) )
            dataBlock.reserve(receivedlength);
        dataBlock.insert(dataBlock.end(), receivedData, receivedData+receivedlength);
    }

    Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

    if( mDecodeFn != NULL )
        mDecodeFn();
}

void SharedData::encode()
{
    MessageHandler::Instance()->printDebug("SharedData::encode\n");

    Engine::lockMutex(sgct_core::NetworkManager::gMutex);

	dataBlock.clear();
	if(mUseCompression)
		dataBlockToCompress.clear();

	//reserve header space
	dataBlock.insert( dataBlock.begin(), headerSpace, headerSpace+sgct_core::SGCTNetwork::mHeaderSize );

	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

	if( mEncodeFn != NULL )
		mEncodeFn();

	if(mUseCompression && dataBlockToCompress.size() > 0)
	{
		Engine::lockMutex(sgct_core::NetworkManager::gMutex);

		//re-allocatate if needed
		if(mCompressedBufferSize < dataBlockToCompress.size())
		{
			mCompressedBuffer = reinterpret_cast<unsigned char*>(realloc(mCompressedBuffer, dataBlockToCompress.size()));
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
			unsigned int originalSize = dataBlockToCompress.size();
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
			Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
			MessageHandler::Instance()->print("SharedData: Failed to compress data (error %d).\n", err);
			return;
		}

		Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
	}
}

void SharedData::writeFloat(float f)
{
    MessageHandler::Instance()->printDebug("SharedData::writeFloat\nFloat = %f", f);
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&f;
	(*currentStorage).insert( (*currentStorage).end(), p, p+4);
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

void SharedData::writeDouble(double d)
{
    MessageHandler::Instance()->printDebug("SharedData::writeDouble\nDouble = %f\n", d);
    Engine::lockMutex(sgct_core::NetworkManager::gMutex);
 	unsigned char *p = (unsigned char *)&d;
	(*currentStorage).insert( (*currentStorage).end(), p, p+8);
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

void SharedData::writeInt32(int i)
{
    MessageHandler::Instance()->printDebug("SharedData::writeInt32\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&i;
	(*currentStorage).insert( (*currentStorage).end(), p, p+4);
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

void SharedData::writeUChar(unsigned char c)
{
    MessageHandler::Instance()->printDebug("SharedData::writeUChar\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	unsigned char *p = &c;
	(*currentStorage).push_back(*p);
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

void SharedData::writeBool(bool b)
{
    MessageHandler::Instance()->printDebug("SharedData::writeBool\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	if( b )
		(*currentStorage).push_back(1);
	else
		(*currentStorage).push_back(0);
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

void SharedData::writeShort(short s)
{
    MessageHandler::Instance()->printDebug("SharedData::writeShort\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&s;
	(*currentStorage).insert( (*currentStorage).end(), p, p+2);
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

void SharedData::writeString(const std::string& s)
{
    MessageHandler::Instance()->printDebug("SharedData::writeString\n");
    Engine::lockMutex(sgct_core::NetworkManager::gMutex);
    const char* stringData = s.c_str();
    unsigned int length = s.size() + 1;  // +1 for the \0 character
    unsigned char *p = (unsigned char *)&length;
    (*currentStorage).insert( (*currentStorage).end(), p, p+4);
    (*currentStorage).insert( (*currentStorage).end(), stringData, stringData+length);
    Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

void SharedData::writeUCharArray(unsigned char * c, size_t length)
{
    MessageHandler::Instance()->printDebug("SharedData::writeUCharArray\n");
    Engine::lockMutex(sgct_core::NetworkManager::gMutex);
    (*currentStorage).insert( (*currentStorage).end(), c, c+length);
    Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

float SharedData::readFloat()
{
    MessageHandler::Instance()->printDebug("SharedData::readFloat\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
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
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

    MessageHandler::Instance()->printDebug("Float = %f\n", cf.f);

	return cf.f;
}

double SharedData::readDouble()
{
    MessageHandler::Instance()->printDebug("SharedData::readDouble\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
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
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

    MessageHandler::Instance()->printDebug("Double = %f\n", cf.d);

	return cf.d;
}

int SharedData::readInt32()
{
    MessageHandler::Instance()->printDebug("SharedData::readInt32\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
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
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

	return ci.i;
}

unsigned char SharedData::readUChar()
{
    MessageHandler::Instance()->printDebug("SharedData::readUChar\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	unsigned char c;
	c = dataBlock[pos];
	pos += 1;
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

	return c;
}

bool SharedData::readBool()
{
    MessageHandler::Instance()->printDebug("SharedData::readBool\n");
    Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	bool b;
	b = dataBlock[pos] == 1 ? true : false;
	pos += 1;
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

	return b;
}

short SharedData::readShort()
{
    MessageHandler::Instance()->printDebug("SharedData::readShort\n");
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	union
	{
		short s;
		unsigned char c[2];
	} cs;

	cs.c[0] = dataBlock[pos];
	cs.c[1] = dataBlock[pos+1];

	pos += 2;
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

	return cs.s;
}

std::string SharedData::readString()
{
    MessageHandler::Instance()->printDebug("SharedData::readString\n");
    Engine::lockMutex(sgct_core::NetworkManager::gMutex);
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

    char* stringData = (char*)malloc(ci.i);
    for (unsigned int i = 0; i < ci.i; ++i) {
        stringData[i] = dataBlock[pos + i];
    }
    //memcpy(stringData, (void*)dataBlock[pos], ci.i);
    pos += ci.i;
    std::string result(stringData);
    free(stringData);
    Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
    return result;
}

unsigned char * SharedData::readUCharArray(size_t length)
{
    MessageHandler::Instance()->printDebug("SharedData::readUCharArray\n");
    Engine::lockMutex(sgct_core::NetworkManager::gMutex);

	unsigned char * p = &dataBlock[pos];
    pos += length;

	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);

    return p;
}
