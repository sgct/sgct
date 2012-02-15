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
	memset(mParseBuffer,	'\0',	MESSAGE_HANDLER_MAX_SIZE);

	mRecBuffer.reserve(MESSAGE_HANDLER_MAX_SIZE);

	mBuffer.reserve(BUFFER_HANDLER_MAX_SIZE);
	mSwapBuffer1.reserve(BUFFER_HANDLER_MAX_SIZE);
	mSwapBuffer2.reserve(BUFFER_HANDLER_MAX_SIZE);

	mBuffer.push_back(core_sgct::SGCTNetwork::SyncHeader);

    mLocal = true;
}

sgct::MessageHandler::~MessageHandler(void)
{
    free(mParseBuffer);
    mParseBuffer = NULL;

	mBuffer.clear();
	mSwapBuffer1.clear();
	mSwapBuffer2.clear();
	mRecBuffer.clear();
}

void sgct::MessageHandler::decode(const char * receivedData, int receivedLenght, int clientIndex)
{
	if( receivedLenght > 1 ) //header + '\0'
	{
        mRecBuffer.clear();
		mRecBuffer.insert(mRecBuffer.end(), receivedData, receivedData + receivedLenght);
		fprintf(stderr, "\n[client %d]: %s\n", clientIndex, mRecBuffer.c_str());
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
			mBuffer.push_back(core_sgct::SGCTNetwork::SyncHeader);
		mBuffer.insert(mBuffer.end(), mParseBuffer, mParseBuffer+strlen(mParseBuffer));
    }
}

void sgct::MessageHandler::clearBuffer()
{
	mBuffer.clear();
}

const char * sgct::MessageHandler::getMessage()
{
	return mBuffer.c_str();
}

const char * sgct::MessageHandler::getTrimmedMessage( unsigned int indexOfLastChar )
{
    if( mBuffer.size() > indexOfLastChar)
    {
        mSwapBuffer1.clear();
		mSwapBuffer2.clear();
		
		mSwapBuffer1 = mBuffer.substr(0,indexOfLastChar);
		
		mSwapBuffer2.push_back(core_sgct::SGCTNetwork::SyncHeader);
		mSwapBuffer2.insert(mSwapBuffer2.end(), mBuffer.begin() + indexOfLastChar,
			mBuffer.end());
		
		mBuffer.assign(mSwapBuffer2);
		
		return mSwapBuffer1.c_str();
    }
    else
    {
        return mBuffer.c_str();
    }
}
