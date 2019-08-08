/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/NetworkManager.h>
#include <sgct/SharedData.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
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

    // use a compression buffer twice as large
    // to fit huffman tree + data which can be
    // larger than original data in some cases.
    // Normally a sixe x 1.1 should be enough.
    mCompressedBuffer = new (std::nothrow) unsigned char[DEFAULT_SIZE * 2];
    mCompressedBufferSize = DEFAULT_SIZE * 2;

    dataBlock.reserve(DEFAULT_SIZE);
    dataBlockToCompress.reserve(DEFAULT_SIZE);

    mUseCompression = false;
    mCompressionRatio = 1.0f;
    mCompressionLevel = Z_BEST_SPEED;

    if(mUseCompression)
        currentStorage = &dataBlockToCompress;
    else
        currentStorage = &dataBlock;

    headerSpace        = new (std::nothrow) unsigned char[sgct_core::SGCTNetwork::mHeaderSize];

    if( !mCompressedBuffer || !headerSpace )
    {
        fprintf(stderr, "Fatal error! Failed to allocate SharedData memory pool.\n");
        return;
    }
    
    headerSpace[0] = sgct_core::SGCTNetwork::DataId;
    
    //fill rest of header with SGCTNetwork::DefaultId
    memset(headerSpace+1, sgct_core::SGCTNetwork::DefaultId, sgct_core::SGCTNetwork::mHeaderSize-1);
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

/*!
 Compression levels 1-9.
 -1 = Default compression
 0 = No compression
 1 = Best speed
 9 = Best compression
 */
void SharedData::setCompression(bool state, int level)
{
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    mUseCompression = state;
    mCompressionLevel = level;

    if(mUseCompression)
        currentStorage = &dataBlockToCompress;
    else
    {
        currentStorage = &dataBlock;
        mCompressionRatio = 1.0f;
    }
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
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

    if( receivedlength > static_cast<int>(dataBlock.capacity()) )
        dataBlock.reserve(receivedlength);
    dataBlock.insert(dataBlock.end(), receivedData, receivedData+receivedlength);

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
    {
        dataBlockToCompress.clear();
        headerSpace[0] = sgct_core::SGCTNetwork::CompressedDataId;
    }
    else
    {
        headerSpace[0] = sgct_core::SGCTNetwork::DataId;
    }

    //reserve header space
    dataBlock.insert( dataBlock.begin(), headerSpace, headerSpace+sgct_core::SGCTNetwork::mHeaderSize );

    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

    if( mEncodeFn != NULL )
        mEncodeFn();

    if(mUseCompression && dataBlockToCompress.size() > 0)
    {
        SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );

        // re-allocatate if needed
        // use a compression buffer twice as large
        // to fit huffman tree + data which can be
        // larger than original data in some cases.
        // Normally a sixe x 1.1 should be enough.
        if(mCompressedBufferSize < (dataBlockToCompress.size()/2))
        {
            delete [] mCompressedBuffer;
            mCompressedBufferSize = dataBlockToCompress.size()*2;
            mCompressedBuffer = new (std::nothrow) unsigned char[ mCompressedBufferSize ];
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
            //add original size
            uint32_t uncompressedSize = static_cast<uint32_t>(dataBlockToCompress.size());
            unsigned char *p = reinterpret_cast<unsigned char *>(&uncompressedSize);

            dataBlock[9] = p[0];
            dataBlock[10] = p[1];
            dataBlock[11] = p[2];
            dataBlock[12] = p[3];
            
            mCompressionRatio = static_cast<float>(compressed_size) / static_cast<float>(uncompressedSize);

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

std::size_t SharedData::getUserDataSize()
{
    return dataBlock.size()-sgct_core::SGCTNetwork::mHeaderSize;
}

void SharedData::writeFloat(SharedFloat * sf)
{
#ifdef __SGCT_NETWORK_DEBUG__    
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeFloat\nFloat = %f", sf->getVal());
#endif

    float val = sf->getVal();
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
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
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert( (*currentStorage).end(), p, p+8);
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeInt64(SharedInt64 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeInt64\nInt = %ld\n", si->getVal());
#endif

    int64_t val = si->getVal();
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert((*currentStorage).end(), p, p + sizeof(int64_t));
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeInt32(SharedInt32 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeInt32\nInt = %d\n", si->getVal());
#endif

    int32_t val = si->getVal();
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert((*currentStorage).end(), p, p + sizeof(int32_t));
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);
}

void SharedData::writeInt16(SharedInt16 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeInt16\nInt = %d\n", si->getVal());
#endif

    int16_t val = si->getVal();
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert((*currentStorage).end(), p, p + sizeof(int16_t));
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);
}

void SharedData::writeInt8(SharedInt8 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeInt8\nInt = %d\n", si->getVal());
#endif

    int8_t val = si->getVal();
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert((*currentStorage).end(), p, p + sizeof(int8_t));
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);
}

void SharedData::writeUInt64(SharedUInt64 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->pruintDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeUInt64\nUInt = %lu\n", si->getVal());
#endif

    uint64_t val = si->getVal();
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert((*currentStorage).end(), p, p + sizeof(uint64_t));
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);
}

void SharedData::writeUInt32(SharedUInt32 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->pruintDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeUInt32\nUInt = %u\n", si->getVal());
#endif

    uint32_t val = si->getVal();
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert((*currentStorage).end(), p, p + sizeof(uint32_t));
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);
}

void SharedData::writeUInt16(SharedUInt16 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->pruintDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeUInt16\nUInt = %u\n", si->getVal());
#endif

    uint16_t val = si->getVal();
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert((*currentStorage).end(), p, p + sizeof(uint16_t));
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);
}

void SharedData::writeUInt8(SharedUInt8 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->pruintDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeUInt8\nUInt = %u\n", si->getVal());
#endif

    uint8_t val = si->getVal();
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    unsigned char *p = reinterpret_cast<unsigned char *>(&val);
    (*currentStorage).insert((*currentStorage).end(), p, p + sizeof(uint8_t));
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);
}

void SharedData::writeUChar(SharedUChar * suc)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeUChar\n");
#endif

    unsigned char val = suc->getVal();
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    (*currentStorage).push_back(val);
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

void SharedData::writeString(SharedString * ss)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeString\n");
#endif
    
    std::string tmpStr( ss->getVal() );
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    uint32_t length = static_cast<uint32_t>(tmpStr.size());
    unsigned char *p = reinterpret_cast<unsigned char *>(&length);
    
    (*currentStorage).insert((*currentStorage).end(), p, p+4);
    (*currentStorage).insert((*currentStorage).end(), tmpStr.data(), tmpStr.data() + length);
    
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeWString(SharedWString * ss)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeWString\n");
#endif

    std::wstring tmpStr(ss->getVal());
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    uint32_t length = static_cast<uint32_t>(tmpStr.size());
    unsigned char *p = reinterpret_cast<unsigned char *>(&length);
    unsigned char *ws = reinterpret_cast<unsigned char *>(&tmpStr[0]);

    (*currentStorage).insert((*currentStorage).end(), p, p + 4);
    (*currentStorage).insert((*currentStorage).end(), ws, ws + length*sizeof(wchar_t));

    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);
}

void SharedData::writeUCharArray(unsigned char * c, uint32_t length)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeUCharArray\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    (*currentStorage).insert( (*currentStorage).end(), c, c+length);
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::writeSize(uint32_t size)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::writeSize\n");
#endif
    
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    unsigned char *p = reinterpret_cast<unsigned char *>(&size);
    (*currentStorage).insert( (*currentStorage).end(), p, p + 4);
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
}

void SharedData::readFloat(SharedFloat * sf)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readFloat\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    
    float val = (*(reinterpret_cast<float*>(&dataBlock[pos])));
    pos += sizeof(float);
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Float = %f\n", val);
#endif

    sf->setVal( val );
}

void SharedData::readDouble(SharedDouble * sd)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readDouble\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    double val = (*(reinterpret_cast<double*>(&dataBlock[pos])));
    pos += sizeof(double);
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Double = %lf\n", val);
#endif

    sd->setVal( val );
}

void SharedData::readInt64(SharedInt64 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readInt64\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    int64_t val = (*(reinterpret_cast<int64_t*>(&dataBlock[pos])));
    pos += sizeof(int64_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Int64 = %ld\n", val);
#endif

    si->setVal(val);
}

void SharedData::readInt32(SharedInt32 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readInt32\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    int32_t val = (*(reinterpret_cast<int32_t*>(&dataBlock[pos])));
    pos += sizeof(int32_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Int32 = %d\n", val);
#endif

    si->setVal(val);
}

void SharedData::readInt16(SharedInt16 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readInt16\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    int16_t val = (*(reinterpret_cast<int16_t*>(&dataBlock[pos])));
    pos += sizeof(int16_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Int16 = %d\n", val);
#endif

    si->setVal(val);
}

void SharedData::readInt8(SharedInt8 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readInt8\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    int8_t val = (*(reinterpret_cast<int8_t*>(&dataBlock[pos])));
    pos += sizeof(int8_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Int8 = %d\n", val);
#endif

    si->setVal(val);
}

void SharedData::readUInt64(SharedUInt64 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readUInt64\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    uint64_t val = (*(reinterpret_cast<uint64_t*>(&dataBlock[pos])));
    pos += sizeof(uint64_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "UInt64 = %lu\n", val);
#endif

    si->setVal(val);
}

void SharedData::readUInt32(SharedUInt32 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readUInt32\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    uint32_t val = (*(reinterpret_cast<uint32_t*>(&dataBlock[pos])));
    pos += sizeof(uint32_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "UInt32 = %u\n", val);
#endif

    si->setVal(val);
}

void SharedData::readUInt16(SharedUInt16 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readUInt16\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    uint16_t val = (*(reinterpret_cast<uint16_t*>(&dataBlock[pos])));
    pos += sizeof(uint16_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "UInt16 = %u\n", val);
#endif

    si->setVal(val);
}

void SharedData::readUInt8(SharedUInt8 * si)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readUInt8\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);
    uint8_t val = (*(reinterpret_cast<uint8_t*>(&dataBlock[pos])));
    pos += sizeof(uint8_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "UInt8 = %u\n", val);
#endif

    si->setVal(val);
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

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "UChar = %d\n", c);
#endif
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

#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "Bool = %d\n", b);
#endif
    sb->setVal( b );
}

void SharedData::readString(SharedString * ss)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readString\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    
    uint32_t length = (*(reinterpret_cast<uint32_t*>(&dataBlock[pos])));
    pos += sizeof(uint32_t);

    if (length == 0)
    {
        ss->clear();
        return;
    }

    char * stringData = new (std::nothrow) char[length+1];
    if (stringData)
    {
        memcpy(stringData, &dataBlock[pos], length);
        //add string terminator
        stringData[length] = '\0';
    }

    pos += length;
    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "String = '%s'\n", stringData);
#endif

    ss->setVal(stringData);

    delete [] stringData;
    stringData = NULL;
}

void SharedData::readWString(SharedWString * ss)
{
#ifdef __SGCT_NETWORK_DEBUG__     
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readWString\n");
#endif
    SGCTMutexManager::instance()->lockMutex(sgct::SGCTMutexManager::DataSyncMutex);

    uint32_t length = (*(reinterpret_cast<uint32_t*>(&dataBlock[pos])));
    pos += sizeof(uint32_t);

    if (length == 0)
    {
        ss->clear();
        return;
    }

    wchar_t * stringData = new (std::nothrow) wchar_t[length + 1];
    if (stringData)
    {
        memcpy(stringData, &dataBlock[pos], length*sizeof(wchar_t));
        //add string terminator
        stringData[length] = L'\0';
    }

    pos += length*sizeof(wchar_t);
    SGCTMutexManager::instance()->unlockMutex(sgct::SGCTMutexManager::DataSyncMutex);

    ss->setVal(stringData);

    delete[] stringData;
    stringData = NULL;
}

unsigned char * SharedData::readUCharArray(uint32_t length)
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readUCharArray\n");
#endif
    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );

    unsigned char * p = &dataBlock[pos];
    pos += length;

    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

    return p;
}

uint32_t SharedData::readSize()
{
#ifdef __SGCT_NETWORK_DEBUG__ 
    MessageHandler::instance()->printDebug(sgct::MessageHandler::NOTIFY_INFO, "SharedData::readSize\n");
#endif

    SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::DataSyncMutex );
    
    uint32_t size = (*(reinterpret_cast<uint32_t*>(&dataBlock[pos])));
    pos += sizeof(uint32_t);

    SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::DataSyncMutex );

    return size;
}
