/* MessageHandler.h

© 2012 Miroslav Andel

*/

#ifndef _MESSAGE_HANDLER
#define _MESSAGE_HANDLER

#include <string>

namespace sgct //small graphics cluster toolkit
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

	void decode(const char * receivedData, int receivedLenght, int clientIndex);
private:
	MessageHandler(void);

	// Don't implement these, should give compile warning if used
	MessageHandler( const MessageHandler & tm );
	const MessageHandler & operator=(const MessageHandler & rhs );

private:
	static MessageHandler * mInstance;
	std::string mBuffer;
};

}

#endif