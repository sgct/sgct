#include "../include/sgct/SharedData.h"
#include "../include/sgct/SGCTNetwork.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace sgct;

SharedData * SharedData::mInstance = NULL;

SharedData::SharedData()
{
	mEncodeFn = NULL;
	mDecodeFn = NULL;
	dataBlock.reserve(512);
}

SharedData::~SharedData()
{
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
	//re-allocate buffer if needed
	if( receivedLenght > static_cast<int>(dataBlock.capacity()) )
		dataBlock.reserve(receivedLenght);
	dataBlock.assign(receivedData, receivedData+receivedLenght);

	//ignore the header byte
	pos = 1;

	if( mDecodeFn != NULL )
		mDecodeFn();
}

void SharedData::encode()
{
	dataBlock.clear();

	//add header byte
	dataBlock.push_back(core_sgct::SGCTNetwork::SyncHeader);

	if( mEncodeFn != NULL )
		mEncodeFn();
}

void SharedData::writeFloat(float f)
{
	unsigned char *p = (unsigned char *)&f;
	dataBlock.insert( dataBlock.end(), p, p+4);
}

void SharedData::writeDouble(double d)
{
	unsigned char *p = (unsigned char *)&d;
	dataBlock.insert( dataBlock.end(), p, p+8);
}

void SharedData::writeInt32(int i)
{
	unsigned char *p = (unsigned char *)&i;
	dataBlock.insert( dataBlock.end(), p, p+4);
}

void SharedData::writeUChar(unsigned char c)
{
	unsigned char *p = &c;
	dataBlock.push_back(*p);
}

float SharedData::readFloat()
{
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

	return cf.f;
}

double SharedData::readDouble()
{
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

	return cf.d;
}

int SharedData::readInt32()
{
	union
	{
		int i;
		unsigned char c[4];
	} cf;

	cf.c[0] = dataBlock[pos];
	cf.c[1] = dataBlock[pos+1];
	cf.c[2] = dataBlock[pos+2];
	cf.c[3] = dataBlock[pos+3];
	pos += 4;

	return cf.i;
}

unsigned char SharedData::readUChar()
{
	unsigned char c;
	c = dataBlock[pos];
	pos += 1;

	return c;
}
