/* MessageHandler.h

© 2012 Miroslav Andel

*/

#ifndef _MESSAGE_HANDLER
#define _MESSAGE_HANDLER

#include <stddef.h> //get definition for NULL
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

	void decode(const char * receivedData, int receivedLenght, int clientIndex);
	void print(const char *fmt, ...);
    void sendMessagesToServer( bool state ) { mLocal = !state; }
    void clearBuffer();

	inline unsigned int getDataSize() { return mBuffer.size(); }
	inline unsigned int getTrimmedDataSize() { return mSwapBuffer1.size(); }

	char * getMessage();
    char * getTrimmedMessage( int unsigned indexOfLastChar );

private:
	MessageHandler(void);
	~MessageHandler(void);

	// Don't implement these, should give compile warning if used
	MessageHandler( const MessageHandler & tm );
	const MessageHandler & operator=(const MessageHandler & rhs );

private:
	static MessageHandler * mInstance;

	//int pos;
	//int swapSize;
	char * mParseBuffer;
	std::vector<char> mSwapBuffer1;
	std::vector<char> mSwapBuffer2;
	std::vector<char> mBuffer;
	std::vector<char> mRecBuffer;
	unsigned char  * headerSpace;
	bool mLocal;
};

}

#endif
