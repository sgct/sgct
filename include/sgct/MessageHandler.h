/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _MESSAGE_HANDLER
#define _MESSAGE_HANDLER

#include <stddef.h> //get definition for NULL
#include <stdarg.h>
#include <vector>

namespace sgct //simple graphics cluster toolkit
{

class MessageHandler
{
public:
	/*! Get the MessageHandler instance */
	static MessageHandler * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new MessageHandler();
		}

		return mInstance;
	}

	/*! Destroy the MessageHandler */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	void decode(const char * receivedData, int receivedlength, int clientIndex);
	void print(const char *fmt, ...);
    void printDebug(const char *fmt, ...);
    void printIndent(unsigned int indentation, const char* fmt, ...);
	void sendMessageToServer(const char *fmt);
    void setSendFeedbackToServer( bool state ) { mLocal = !state; }
    void clearBuffer();

	inline unsigned int getDataSize() { return mBuffer.size(); }

	char * getMessage();

private:
	MessageHandler(void);
	~MessageHandler(void);

	// Don't implement these, should give compile warning if used
	MessageHandler( const MessageHandler & tm );
	const MessageHandler & operator=(const MessageHandler & rhs );

private:
    void printv(const char *fmt, va_list ap);

	static MessageHandler * mInstance;

	char * mParseBuffer;
	std::vector<char> mBuffer;
	std::vector<char> mRecBuffer;
	unsigned char  * headerSpace;
	bool mLocal;
};

}

#endif
