#include "SharedData.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace sgct;

SharedData::SharedData(unsigned int bufferSize)
{
	mBufferSize = bufferSize;
	mEncodeFn = NULL;
	mDecodeFn = NULL;
	dataBlock.reserve(mBufferSize);
	//dataBlock = (unsigned char *) malloc(BUFFER_SIZE);
	/*
	dt = 0.0;
	time = 0.0;
	showFPS = false;
	flags = 0;
	*/
}

SharedData::~SharedData()
{
	//free(dataBlock);
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

void SharedData::decode(const char * receivedData, int receivedLenght)
{	
	pos = 0;
	//memcpy(dataBlock, receivedData, receivedLenght);
	dataBlock.assign(receivedData, receivedData+receivedLenght);

	/*
		Make sure that all the data fits into the buffer. There is no check right now.
	*/

	if( mDecodeFn != NULL )
		mDecodeFn();
	/*dt = readDouble();
	time = readDouble();
	flags = readUChar();

	showFPS	= flags & 0x0001;*/
}

void SharedData::encode()
{
	//pos = 0;
	dataBlock.clear();

	if( mEncodeFn != NULL )
		mEncodeFn();

	/*flags = showFPS	? flags | 1 : flags & ~1;

	writeDouble(dt);
	writeDouble(time);
	writeUChar(flags);*/
}

void SharedData::writeFloat(float f)
{
	unsigned char *p = (unsigned char *)&f;
	if( (dataBlock.size() + 4) > mBufferSize )
		return;
	dataBlock.insert( dataBlock.end(), p, p+4);
	/*dataBlock[pos] = p[0];
	dataBlock[pos+1] = p[1];
	dataBlock[pos+2] = p[2];
	dataBlock[pos+3] = p[3];
	pos += 4;*/


}

void SharedData::writeDouble(double d)
{
	unsigned char *p = (unsigned char *)&d;
	if( (dataBlock.size() + 8) > mBufferSize )
		return;
	dataBlock.insert( dataBlock.end(), p, p+8);
	/*dataBlock[pos] = p[0];
	dataBlock[pos+1] = p[1];
	dataBlock[pos+2] = p[2];
	dataBlock[pos+3] = p[3];
	dataBlock[pos+4] = p[4];
	dataBlock[pos+5] = p[5];
	dataBlock[pos+6] = p[6];
	dataBlock[pos+7] = p[7];
	pos += 8;*/
}

void SharedData::writeInt32(int i)
{
	unsigned char *p = (unsigned char *)&i;
	if( (dataBlock.size() + 4) > mBufferSize )
		return;
	dataBlock.insert( dataBlock.end(), p, p+4);
	/*dataBlock[pos] = p[0];
	dataBlock[pos+1] = p[1];
	dataBlock[pos+2] = p[2];
	dataBlock[pos+3] = p[3];
	pos += 4;*/
}

void SharedData::writeUChar(unsigned char c)
{
	unsigned char *p = &c;
	if( (dataBlock.size() + 1) > mBufferSize )
		return;
	dataBlock.push_back(*p);
	//dataBlock[pos] = p[0];
	//pos += 1;
}

float SharedData::readFloat()
{
	union
	{
		float f;
		unsigned char c[4];
	} cf;

	/*cf.c[0] = *(dataBlock + pos);
	cf.c[1] = *(dataBlock + pos + 1);
	cf.c[2] = *(dataBlock + pos + 2);
	cf.c[3] = *(dataBlock + pos + 3);*/
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

	/*cf.c[0] = *(dataBlock + pos);
	cf.c[1] = *(dataBlock + pos + 1);
	cf.c[2] = *(dataBlock + pos + 2);
	cf.c[3] = *(dataBlock + pos + 3);
	cf.c[4] = *(dataBlock + pos + 4);
	cf.c[5] = *(dataBlock + pos + 5);
	cf.c[6] = *(dataBlock + pos + 6);
	cf.c[7] = *(dataBlock + pos + 7);*/

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

	/*cf.c[0] = *(dataBlock + pos);
	cf.c[1] = *(dataBlock + pos + 1);
	cf.c[2] = *(dataBlock + pos + 2);
	cf.c[3] = *(dataBlock + pos + 3);*/
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
	//c = *(dataBlock + pos);
	pos += 1;

	return c;
}