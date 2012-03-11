#include "../include/sgct/SharedData.h"
#include "../include/sgct/NetworkManager.h"
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

		// @TODO JOEL: Removed to make examples run on Mac Os X
		// glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
		glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );

		//re-allocate buffer if needed
		if( (receivedLenght + static_cast<int>(core_sgct::SGCTNetwork::syncHeaderSize)) > static_cast<int>(dataBlock.capacity()) )
			dataBlock.reserve(receivedLenght + core_sgct::SGCTNetwork::syncHeaderSize);
		dataBlock.assign(headerSpace, headerSpace + core_sgct::SGCTNetwork::syncHeaderSize);
		dataBlock.insert(dataBlock.end(), receivedData, receivedData+receivedLenght);

		//reset
		pos = core_sgct::SGCTNetwork::syncHeaderSize;

		if( mDecodeFn != NULL )
			mDecodeFn();

		// @TODO JOEL: Removed to make examples run on Mac Os X
		// glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
		glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );
	}
}

void SharedData::encode()
{
//@TODO Decide whatever weshould have the mutex lock in encode or implicitly in mEncodeFn through
//e.g writeDouble()

// @TODO JOEL: Removed to make examples run on Mac Os X
// glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
	dataBlock.clear();

	//reserve header space

	dataBlock.insert( dataBlock.begin(), headerSpace, headerSpace+core_sgct::SGCTNetwork::syncHeaderSize );

	if( mEncodeFn != NULL )
		mEncodeFn();
// @TODO JOEL: Removed to make examples run on Mac Os X
// glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );
}

void SharedData::writeFloat(float f)
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
	unsigned char *p = (unsigned char *)&f;
	dataBlock.insert( dataBlock.end(), p, p+4);
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );
}

void SharedData::writeDouble(double d)
{
    glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
 	unsigned char *p = (unsigned char *)&d;
	dataBlock.insert( dataBlock.end(), p, p+8);
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );
}

void SharedData::writeInt32(int i)
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
	unsigned char *p = (unsigned char *)&i;
	dataBlock.insert( dataBlock.end(), p, p+4);
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );
}

void SharedData::writeUChar(unsigned char c)
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
	unsigned char *p = &c;
	dataBlock.push_back(*p);
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );
}

void SharedData::writeBool(bool b)
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
	if( b )
		dataBlock.push_back(1);
	else
		dataBlock.push_back(0);
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );
}

float SharedData::readFloat()
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
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
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );

	return cf.f;
}

double SharedData::readDouble()
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
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
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );

	return cf.d;
}

int SharedData::readInt32()
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
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
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );

	return ci.i;
}

unsigned char SharedData::readUChar()
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
	unsigned char c;
	c = dataBlock[pos];
	pos += 1;
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );

	return c;
}

bool SharedData::readBool()
{
	glfwLockMutex( core_sgct::NetworkManager::gDecoderMutex );
	bool b;
	b = dataBlock[pos] == 1 ? true : false;
	pos += 1;
	glfwUnlockMutex( core_sgct::NetworkManager::gDecoderMutex );

	return b;
}
