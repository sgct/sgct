#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/SGCTNetwork.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define MESSAGE_HANDLER_MAX_SIZE 8192
#define BUFFER_HANDLER_MAX_SIZE 32768

sgct::MessageHandler * sgct::MessageHandler::mInstance = NULL;

sgct::MessageHandler::MessageHandler(void)
{
    mParseBuffer	= NULL;
	mParseBuffer	= (char*) malloc(MESSAGE_HANDLER_MAX_SIZE);

	headerSpace		= NULL;
	headerSpace		= (unsigned char*) malloc(core_sgct::SGCTNetwork::syncHeaderSize);

	mRecBuffer.reserve(MESSAGE_HANDLER_MAX_SIZE);

	mBuffer.reserve(BUFFER_HANDLER_MAX_SIZE);
	mSwapBuffer1.reserve(BUFFER_HANDLER_MAX_SIZE);
	mSwapBuffer2.reserve(BUFFER_HANDLER_MAX_SIZE);

	for(unsigned int i=0; i<core_sgct::SGCTNetwork::syncHeaderSize; i++)
		headerSpace[i] = core_sgct::SGCTNetwork::SyncHeader;
	mBuffer.insert(mBuffer.begin(), headerSpace, headerSpace+core_sgct::SGCTNetwork::syncHeaderSize);

    mLocal = true;
}

sgct::MessageHandler::~MessageHandler(void)
{
    free(mParseBuffer);
    mParseBuffer = NULL;

	free(headerSpace);
    headerSpace = NULL;

	mBuffer.clear();
	mSwapBuffer1.clear();
	mSwapBuffer2.clear();
	mRecBuffer.clear();
}

void sgct::MessageHandler::decode(const char * receivedData, int receivedLenght, int clientIndex)
{
	if(receivedLenght > 0)
	{
		mRecBuffer.clear();
		mRecBuffer.insert(mRecBuffer.end(), receivedData, receivedData + receivedLenght);
		mRecBuffer.push_back('\0');
		fprintf(stderr, "\n[client %d]: %s\n", clientIndex, &mRecBuffer[0]);
	}
}

void sgct::MessageHandler::print(const char *fmt, ...)
{
    va_list		ap;			// Pointer To List Of Arguments

	if (fmt == NULL)		// If There's No Text
	{
		*mParseBuffer=0;	// Do Nothing
		return;
	}
	else
	{
        va_start(ap, fmt);	// Parses The String For Variables
#if (_MSC_VER >= 1400) //visual studio 2005 or later
	    vsprintf_s(mParseBuffer, MESSAGE_HANDLER_MAX_SIZE, fmt, ap);	// And Converts Symbols To Actual Numbers
#else
		vsprintf(mParseBuffer, fmt, ap);
#endif
        va_end(ap);		// Results Are Stored In Text
	}

    //print local
    fprintf(stderr, mParseBuffer);

    //if client send to server
    if(!mLocal)
    {
        if(mBuffer.empty())
			mBuffer.insert(mBuffer.begin(), headerSpace, headerSpace+core_sgct::SGCTNetwork::syncHeaderSize);
		mBuffer.insert(mBuffer.end(), mParseBuffer, mParseBuffer+strlen(mParseBuffer));
    }
}

void sgct::MessageHandler::clearBuffer()
{
	mBuffer.clear();
}

char * sgct::MessageHandler::getMessage()
{
	return &mBuffer[0];
}

char * sgct::MessageHandler::getTrimmedMessage( unsigned int indexOfLastChar )
{
    if( mBuffer.size() > indexOfLastChar)
    {
        mSwapBuffer1.clear();
		mSwapBuffer2.clear();
		
		mSwapBuffer1.insert(mSwapBuffer1.begin(), &mBuffer[0], &mBuffer[0]+indexOfLastChar);
		
		mSwapBuffer2.insert(mSwapBuffer2.begin(), headerSpace, headerSpace+core_sgct::SGCTNetwork::syncHeaderSize);
		mSwapBuffer2.insert(mSwapBuffer2.end(), mBuffer.begin() + indexOfLastChar,
			mBuffer.end());
		
		mBuffer.assign(mSwapBuffer2.begin(), mSwapBuffer2.end());
		
		return &mSwapBuffer1[0];
    }
    else
    {
        return &mBuffer[0];
    }
}
