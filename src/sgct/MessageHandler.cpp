/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/NetworkManager.h"
#include "../include/sgct/Engine.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#define MESSAGE_HANDLER_MAX_SIZE 8192

sgct::MessageHandler * sgct::MessageHandler::mInstance = NULL;

sgct::MessageHandler::MessageHandler(void)
{
    mParseBuffer	= NULL;
	mParseBuffer	= reinterpret_cast<char*>( malloc(MESSAGE_HANDLER_MAX_SIZE) );

	headerSpace		= NULL;
	headerSpace		= reinterpret_cast<unsigned char*>( malloc(sgct_core::SGCTNetwork::mHeaderSize) );

	mRecBuffer.reserve(MESSAGE_HANDLER_MAX_SIZE);
	mBuffer.reserve(MESSAGE_HANDLER_MAX_SIZE);

	for(unsigned int i=0; i<sgct_core::SGCTNetwork::mHeaderSize; i++)
		headerSpace[i] = sgct_core::SGCTNetwork::SyncByte;
	mBuffer.insert(mBuffer.begin(), headerSpace, headerSpace+sgct_core::SGCTNetwork::mHeaderSize);

    mLocal = true;
}

sgct::MessageHandler::~MessageHandler(void)
{
    free(mParseBuffer);
    mParseBuffer = NULL;

	free(headerSpace);
    headerSpace = NULL;

	mBuffer.clear();
	mRecBuffer.clear();
}

void sgct::MessageHandler::decode(const char * receivedData, int receivedlength, int clientIndex)
{
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
		mRecBuffer.clear();
		mRecBuffer.insert(mRecBuffer.end(), receivedData, receivedData + receivedlength);
		mRecBuffer.push_back('\0');
		fprintf(stderr, "\n[client %d]: %s [end]\n", clientIndex, &mRecBuffer[0]);
    Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}


void sgct::MessageHandler::printv(const char *fmt, va_list ap)
{
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    vsprintf_s(mParseBuffer, MESSAGE_HANDLER_MAX_SIZE, fmt, ap);	// And Converts Symbols To Actual Numbers
#else
    vsprintf(mParseBuffer, fmt, ap);
#endif
    va_end(ap);		// Results Are Stored In Text

    //print local
    std::cerr << mParseBuffer;

    //if client send to server
    sendMessageToServer(mParseBuffer);
}

void sgct::MessageHandler::print(const char *fmt, ...)
{
    va_list		ap;			// Pointer To List Of Arguments

	if (fmt == NULL)		// If There's No Text
	{
		*mParseBuffer=0;	// Do Nothing
		return;
	}

    va_start(ap, fmt);	// Parses The String For Variables
    printv(fmt, ap);
}

void sgct::MessageHandler::clearBuffer()
{
	Engine::lockMutex(sgct_core::NetworkManager::gMutex);
	mBuffer.clear();
	Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
}

char * sgct::MessageHandler::getMessage()
{
	return &mBuffer[0];
}

void sgct::MessageHandler::printDebug(const char *fmt, ...)
{
#ifdef __SGCT_DEBUG__
    va_list ap;
    if (fmt == NULL)
    {
        *mParseBuffer = 0;
        return;
    }
    va_start(ap, fmt);	// Parses The String For Variables
    printv(fmt, ap);
#endif
}

void sgct::MessageHandler::printIndent(unsigned int indentation, const char* fmt, ...)
{
    va_list ap;
    if (fmt == NULL)
    {
        *mParseBuffer = 0;
        return;
    }

    if (indentation > 0) {
        const std::string padding(indentation, ' ');
        const std::string fmtString = std::string(fmt);
        const std::string fmtComplete = padding + fmtString;

        const char *fmtIndented = fmtComplete.c_str();
        va_start(ap, fmt);	// Parses The String For Variables
        printv(fmtIndented, ap);
    }
    else {
        va_start(ap, fmt);	// Parses The String For Variables
        printv(fmt, ap);
    }
}

void sgct::MessageHandler::sendMessageToServer(const char * str)
{
	if(str == NULL)
		return;

	//if client send to server
    if(!mLocal && sgct_core::NetworkManager::gMutex != NULL)
    {
        Engine::lockMutex(sgct_core::NetworkManager::gMutex);
        if(mBuffer.empty())
            mBuffer.insert(mBuffer.begin(), headerSpace, headerSpace+sgct_core::SGCTNetwork::mHeaderSize);
        mBuffer.insert(mBuffer.end(), str, str + strlen(str));
        Engine::unlockMutex(sgct_core::NetworkManager::gMutex);
    }
}
