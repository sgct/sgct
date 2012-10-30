/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
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
	headerSpace		= reinterpret_cast<unsigned char*>( malloc(core_sgct::SGCTNetwork::mHeaderSize) );

	for(unsigned int i=0; i<core_sgct::SGCTNetwork::mHeaderSize; i++)
		headerSpace[i] = core_sgct::SGCTNetwork::SyncByte;
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
	return mUseCompression ? mCompressedSize : (dataBlock.size() - core_sgct::SGCTNetwork::mHeaderSize);
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
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);

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
            Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
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

    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

    if( mDecodeFn != NULL )
        mDecodeFn();
}

void SharedData::encode()
{
    MessageHandler::Instance()->printDebug("SharedData::encode\n");

    Engine::lockMutex(core_sgct::NetworkManager::gMutex);

	dataBlock.clear();
	if(mUseCompression)
		dataBlockToCompress.clear();

	//reserve header space
	dataBlock.insert( dataBlock.begin(), headerSpace, headerSpace+core_sgct::SGCTNetwork::mHeaderSize );

	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	if( mEncodeFn != NULL )
		mEncodeFn();

	if(mUseCompression && dataBlockToCompress.size() > 0)
	{
		Engine::lockMutex(core_sgct::NetworkManager::gMutex);

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
			Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
			MessageHandler::Instance()->print("SharedData: Failed to compress data (error %d).\n", err);
			return;
		}

		Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
	}
}

void SharedData::writeFloat(float f)
{
    MessageHandler::Instance()->printDebug("SharedData::writeFloat\nFloat = %f", f);
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&f;
	(*currentStorage).insert( (*currentStorage).end(), p, p+4);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeDouble(double d)
{
    MessageHandler::Instance()->printDebug("SharedData::writeDouble\nDouble = %f\n", d);
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
 	unsigned char *p = (unsigned char *)&d;
	(*currentStorage).insert( (*currentStorage).end(), p, p+8);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeInt32(int i)
{
    MessageHandler::Instance()->printDebug("SharedData::writeInt32\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&i;
	(*currentStorage).insert( (*currentStorage).end(), p, p+4);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeUChar(unsigned char c)
{
    MessageHandler::Instance()->printDebug("SharedData::writeUChar\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = &c;
	(*currentStorage).push_back(*p);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeBool(bool b)
{
    MessageHandler::Instance()->printDebug("SharedData::writeBool\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	if( b )
		(*currentStorage).push_back(1);
	else
		(*currentStorage).push_back(0);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeShort(short s)
{
    MessageHandler::Instance()->printDebug("SharedData::writeShort\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&s;
	(*currentStorage).insert( (*currentStorage).end(), p, p+2);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeString(const std::string& s)
{
    MessageHandler::Instance()->printDebug("SharedData::writeString\n");
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
    const char* stringData = s.c_str();
    unsigned int length = s.size() + 1;  // +1 for the \0 character
    unsigned char *p = (unsigned char *)&length;
    (*currentStorage).insert( (*currentStorage).end(), p, p+4);
    (*currentStorage).insert( (*currentStorage).end(), stringData, stringData+length);
    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeUCharArray(unsigned char * c, size_t length)
{
    MessageHandler::Instance()->printDebug("SharedData::writeUCharArray\n");
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
    (*currentStorage).insert( (*currentStorage).end(), c, c+length);
    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

float SharedData::readFloat()
{
    MessageHandler::Instance()->printDebug("SharedData::readFloat\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
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
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

    MessageHandler::Instance()->printDebug("Float = %f\n", cf.f);

	return cf.f;
}

double SharedData::readDouble()
{
    MessageHandler::Instance()->printDebug("SharedData::readDouble\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
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
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

    MessageHandler::Instance()->printDebug("Double = %f\n", cf.d);

	return cf.d;
}

int SharedData::readInt32()
{
    MessageHandler::Instance()->printDebug("SharedData::readInt32\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
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
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	return ci.i;
}

unsigned char SharedData::readUChar()
{
    MessageHandler::Instance()->printDebug("SharedData::readUChar\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char c;
	c = dataBlock[pos];
	pos += 1;
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	return c;
}

bool SharedData::readBool()
{
    MessageHandler::Instance()->printDebug("SharedData::readBool\n");
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	bool b;
	b = dataBlock[pos] == 1 ? true : false;
	pos += 1;
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	return b;
}

short SharedData::readShort()
{
    MessageHandler::Instance()->printDebug("SharedData::readShort\n");
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	union
	{
		short s;
		unsigned char c[2];
	} cs;

	cs.c[0] = dataBlock[pos];
	cs.c[1] = dataBlock[pos+1];

	pos += 2;
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	return cs.s;
}

std::string SharedData::readString()
{
    MessageHandler::Instance()->printDebug("SharedData::readString\n");
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
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
    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
    return result;
}

unsigned char * SharedData::readUCharArray(size_t length)
{
    MessageHandler::Instance()->printDebug("SharedData::readUCharArray\n");
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);

	unsigned char * p = &dataBlock[pos];
    pos += length;

	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

    return p;
}
