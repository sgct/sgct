#include "../include/sgct/SharedData.h"
#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/MessageHandler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace sgct;

SharedData * SharedData::mInstance = NULL;

SharedData::SharedData()
{
	mEncodeFn = NULL;
	mDecodeFn = NULL;
	dataBlock.reserve(1024);

	headerSpace		= NULL;
	headerSpace		= (unsigned char*) malloc(core_sgct::SGCTNetwork::syncHeaderSize);

	for(unsigned int i=0; i<core_sgct::SGCTNetwork::syncHeaderSize; i++)
		headerSpace[i] = core_sgct::SGCTNetwork::SyncHeader;
}

SharedData::~SharedData()
{
	free(headerSpace);
    headerSpace = NULL;

	dataBlock.clear();
}

void SharedData::setEncodeFunction(void(*fnPtr)(void))
{
	mEncodeFn = fnPtr;
}

void SharedData::setDecodeFunction(void(*fnPtr)(void))
{
	mDecodeFn = fnPtr;
}

void SharedData::decode(const char * receivedData, int receivedLenght, int clientIndex)
{
	if(receivedLenght > 0)
	{
#ifdef __SGCT_DEBUG__
        sgct::MessageHandler::Instance()->print("SharedData::decode\n");
#endif


		Engine::lockMutex(core_sgct::NetworkManager::gMutex);

		//re-allocate buffer if needed
		if( (receivedLenght + static_cast<int>(core_sgct::SGCTNetwork::syncHeaderSize)) > static_cast<int>(dataBlock.capacity()) )
			dataBlock.reserve(receivedLenght + core_sgct::SGCTNetwork::syncHeaderSize);
		dataBlock.assign(headerSpace, headerSpace + core_sgct::SGCTNetwork::syncHeaderSize);
		dataBlock.insert(dataBlock.end(), receivedData, receivedData+receivedLenght);

		//reset
		pos = core_sgct::SGCTNetwork::syncHeaderSize;

		Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

		if( mDecodeFn != NULL )
			mDecodeFn();
	}
}

void SharedData::encode()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::encode\n");
#endif

    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	dataBlock.clear();

	//reserve header space
	dataBlock.insert( dataBlock.begin(), headerSpace, headerSpace+core_sgct::SGCTNetwork::syncHeaderSize );

    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	if( mEncodeFn != NULL )
		mEncodeFn();
}

void SharedData::writeFloat(float f)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::writeFloat\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&f;
	dataBlock.insert( dataBlock.end(), p, p+4);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeDouble(double d)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::writeDouble\n");
#endif
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
 	unsigned char *p = (unsigned char *)&d;
	dataBlock.insert( dataBlock.end(), p, p+8);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeInt32(int i)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::writeInt32\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&i;
	dataBlock.insert( dataBlock.end(), p, p+4);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeUChar(unsigned char c)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::writeUChar\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = &c;
	dataBlock.push_back(*p);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeBool(bool b)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::writeBool\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	if( b )
		dataBlock.push_back(1);
	else
		dataBlock.push_back(0);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeShort(short s)
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::writeShort\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&s;
	dataBlock.insert( dataBlock.end(), p, p+2);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

float SharedData::readFloat()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::readFloat\n");
#endif
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

	return cf.f;
}

double SharedData::readDouble()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::readDouble\n");
#endif
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

	return cf.d;
}

int SharedData::readInt32()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::readInt32\n");
#endif
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
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::readUChar\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char c;
	c = dataBlock[pos];
	pos += 1;
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	return c;
}

bool SharedData::readBool()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::readBool\n");
#endif
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	bool b;
	b = dataBlock[pos] == 1 ? true : false;
	pos += 1;
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

	return b;
}

short SharedData::readShort()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::readShort\n");
#endif
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
