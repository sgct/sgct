/* SharedData.h

© 2012 Miroslav Andel

*/

#ifndef _SHARED_DATA
#define _SHARED_DATA

#include <vector>

namespace sgct //small graphics cluster toolkit
{

class SharedData
{
public:
	SharedData(unsigned int bufferSize);
	~SharedData();

	void writeFloat(float f);
	void writeDouble(double d);
	void writeInt32(int i);
	void writeUChar(unsigned char c);

	float			readFloat();
	double			readDouble();
	int				readInt32();
	unsigned char	readUChar();

	void setEncodeFunction(void(*fnPtr)(void));
	void setDecodeFunction(void(*fnPtr)(void));

	void encode();
	void decode(const char * receivedData, int receivedLenght, int clientIndex);
	//unsigned char * getDataBlock() { return dataBlock; }
	unsigned char * getDataBlock() { return &dataBlock[0]; }
	//unsigned int getDataSize() { return pos; }
	unsigned int getDataSize() { return dataBlock.size(); }
	unsigned int getBufferSize() { return mBufferSize; }

private:
	//function pointers
	void (*mEncodeFn) (void);
	void (*mDecodeFn) (void);

	//unsigned char * dataBlock;
	std::vector<unsigned char> dataBlock;
	unsigned int pos;
	unsigned int mBufferSize;
};

}

#endif