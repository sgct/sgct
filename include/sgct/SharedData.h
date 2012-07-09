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

#ifndef _SHARED_DATA
#define _SHARED_DATA

#include <stddef.h> //get definition for NULL
#include <vector>
#include <string>

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

	/*!
		Compression levels 1-9.
		-1 = Default compression
		0 = No compression
		1 = Best speed
		9 = Best compression
	*/
	void setCompression(bool state, int level = 1);
	inline float getCompressionRatio() { return mCompressionRatio; }

	void writeFloat(float f);
	void writeDouble(double d);
	void writeInt32(int i);
	void writeUChar(unsigned char c);
	void writeUCharArray(unsigned char * c, size_t length);
	void writeBool(bool b);
	void writeShort(short s);
    void writeString(const std::string& s);
	template<class T> void writeObj(const T& obj);

	float			readFloat();
	double			readDouble();
	int				readInt32();
	unsigned char	readUChar();
	unsigned char * readUCharArray(size_t length);
	bool			readBool();
	short			readShort();
    std::string     readString();
    template<class T> T readObj();

	void setEncodeFunction( void(*fnPtr)(void) );
	void setDecodeFunction( void(*fnPtr)(void) );

	/*
		Compression error/info codes:

		Z_OK            0
		Z_STREAM_END    1
		Z_NEED_DICT     2
		Z_ERRNO        (-1)
		Z_STREAM_ERROR (-2)
		Z_DATA_ERROR   (-3)
		Z_MEM_ERROR    (-4)
		Z_BUF_ERROR    (-5)
		Z_VERSION_ERROR (-6)
	*/
	void encode();
	void decode(const char * receivedData, int receivedlength, int clientIndex);

	inline unsigned char * getDataBlock() { return &dataBlock[0]; }
	inline unsigned int getDataSize() { return dataBlock.size(); }
	inline unsigned int getBufferSize() { return dataBlock.capacity(); }
	unsigned int getUserDataSize();

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
	std::vector<unsigned char> dataBlockToCompress;
	std::vector<unsigned char> * currentStorage;
	unsigned char * mCompressedBuffer;
	unsigned int mCompressedBufferSize;
	unsigned int mCompressedSize;
	unsigned char * headerSpace;
	unsigned int pos;
	int mCompressionLevel;
	float mCompressionRatio;
	bool mUseCompression;
};

template<class T>
void SharedData::writeObj( const T& obj )
{
    unsigned char *p = (unsigned char *)&obj;
    size_t size = sizeof(obj);
    writeUCharArray(p, size);
}

template<class T>
T SharedData::readObj()
{
	size_t size = sizeof(T);
    unsigned char* data = new unsigned char[size];
	unsigned char* c = readUCharArray(size);

	for(size_t i = 0; i < size; i++)
		data[i] = c[i];

    T result = *reinterpret_cast<T*>(data);
    delete[] data;
    return result;
}

}

#endif
