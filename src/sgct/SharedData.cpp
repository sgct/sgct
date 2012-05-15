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

void SharedData::decode(const char * receivedData, int receivedlength, int clientIndex)
{
	if(receivedlength > 0)
	{
#ifdef __SGCT_DEBUG__
        MessageHandler::Instance()->print("SharedData::decode\n");
#endif


		Engine::lockMutex(core_sgct::NetworkManager::gMutex);

		//re-allocate buffer if needed
		if( (receivedlength + static_cast<int>(core_sgct::SGCTNetwork::syncHeaderSize)) > static_cast<int>(dataBlock.capacity()) )
			dataBlock.reserve(receivedlength + core_sgct::SGCTNetwork::syncHeaderSize);
		dataBlock.assign(headerSpace, headerSpace + core_sgct::SGCTNetwork::syncHeaderSize);
		dataBlock.insert(dataBlock.end(), receivedData, receivedData+receivedlength);

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
    MessageHandler::Instance()->print("SharedData::encode\n");
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
    MessageHandler::Instance()->print("SharedData::writeFloat\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&f;
	dataBlock.insert( dataBlock.end(), p, p+4);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeDouble(double d)
{
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::writeDouble\n");
#endif
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
 	unsigned char *p = (unsigned char *)&d;
	dataBlock.insert( dataBlock.end(), p, p+8);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeInt32(int i)
{
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::writeInt32\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&i;
	dataBlock.insert( dataBlock.end(), p, p+4);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeUChar(unsigned char c)
{
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::writeUChar\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = &c;
	dataBlock.push_back(*p);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeBool(bool b)
{
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::writeBool\n");
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
    MessageHandler::Instance()->print("SharedData::writeShort\n");
#endif
	Engine::lockMutex(core_sgct::NetworkManager::gMutex);
	unsigned char *p = (unsigned char *)&s;
	dataBlock.insert( dataBlock.end(), p, p+2);
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeString(const std::string& s)
{
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::writeString\n");
#endif
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
    const char* stringData = s.c_str();
    unsigned int length = s.size() + 1;  // +1 for the \0 character
    unsigned char *p = (unsigned char *)&length;
    dataBlock.insert( dataBlock.end(), p, p+4);
    dataBlock.insert( dataBlock.end(), stringData, stringData+length);
    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

void SharedData::writeUCharArray(unsigned char * c, size_t length)
{
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::writeUCharArray\n");
#endif
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
    dataBlock.insert( dataBlock.end(), c, c+length);
    Engine::unlockMutex(core_sgct::NetworkManager::gMutex);
}

float SharedData::readFloat()
{
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::readFloat\n");
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
    MessageHandler::Instance()->print("SharedData::readDouble\n");
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
    MessageHandler::Instance()->print("SharedData::readInt32\n");
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
    MessageHandler::Instance()->print("SharedData::readUChar\n");
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
    MessageHandler::Instance()->print("SharedData::readBool\n");
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
    MessageHandler::Instance()->print("SharedData::readShort\n");
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

std::string SharedData::readString()
{
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::readString\n");
#endif
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
#ifdef __SGCT_DEBUG__
    MessageHandler::Instance()->print("SharedData::readUCharArray\n");
#endif
    Engine::lockMutex(core_sgct::NetworkManager::gMutex);
    
	unsigned char * p = &dataBlock[pos];
    pos += length;
    
	Engine::unlockMutex(core_sgct::NetworkManager::gMutex);

    return p;
}
