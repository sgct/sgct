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

    template<class T>
    void writeObj(const T& obj);
    template<class T>
    T readObj();

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

template<class T>
void sgct::SharedData::writeObj( const T& obj )
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::writeObj\n");
#endif
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
    unsigned char *p = (unsigned char *)&obj;
    size_t size = sizeof(obj);
    dataBlock.insert( dataBlock.end(), p, p+size);
    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}


template<class T>
T sgct::SharedData::readObj()
{
#ifdef __SGCT_DEBUG__
    sgct::MessageHandler::Instance()->print("SharedData::readFloat\n");
#endif
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
    size_t size = sizeof(T);
    unsigned char* data = new unsigned char[size];

    //union
    //{
    //    T f;
    //    unsigned char* c;
    //} cf;
    //cf.c = new unsigned char[sizeof(T)];

    for(size_t i = 0; i < size; ++i)
    {
        data[i] = dataBlock[pos + i];0
    }
    pos += size;
    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

    T result = reinterpret_cast<T>(data);
    delete[] data;
    return result;
}


}

#endif
