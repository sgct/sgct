/* SharedData.h

© 2012 Miroslav Andel

*/

#ifndef _SHARED_DATA
#define _SHARED_DATA

#include <stddef.h> //get definition for NULL
#include <vector>

namespace sgct //simple graphics cluster toolkit
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

	void writeFloat(float f);
	void writeDouble(double d);
	void writeInt32(int i);
	void writeUChar(unsigned char c);
	void writeBool(bool b);
	void writeShort(short s);

	float			readFloat();
	double			readDouble();
	int				readInt32();
	unsigned char	readUChar();
	bool			readBool();
	short			readShort();

	void setEncodeFunction( void(*fnPtr)(void) );
	void setDecodeFunction( void(*fnPtr)(void) );

	void encode();
	void decode(const char * receivedData, int receivedLenght, int clientIndex);

	inline unsigned char * getDataBlock() { return &dataBlock[0]; }
	inline unsigned int getDataSize() { return dataBlock.size(); }
	inline unsigned int getBufferSize() { return dataBlock.capacity(); }

private:
	SharedData();
	~SharedData();

	// Don't implement these, should give compile warning if used
	SharedData( const SharedData & tm );
	const SharedData & operator=(const SharedData & rhs );

private:
	//function pointers
	void (*mEncodeFn) (void);
	void (*mDecodeFn) (void);

	static SharedData * mInstance;
	std::vector<unsigned char> dataBlock;
	unsigned char * headerSpace;
	unsigned int pos;
};

}

#endif
