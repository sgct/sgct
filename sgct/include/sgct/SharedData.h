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
	/*! Get the SharedData instance */
	static SharedData * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new SharedData();
		}

		return mInstance;
	}

	/*! Destroy the SharedData */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	SharedData();
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

	unsigned char * getDataBlock() { return &dataBlock[0]; }
	unsigned int getDataSize() { return dataBlock.size(); }
	unsigned int getBufferSize() { return dataBlock.capacity(); }

private:
	//function pointers
	void (*mEncodeFn) (void);
	void (*mDecodeFn) (void);

	static SharedData * mInstance;
	std::vector<unsigned char> dataBlock;
	unsigned int pos;
};

}

#endif