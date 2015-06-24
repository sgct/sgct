/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _MESSAGE_HANDLER
#define _MESSAGE_HANDLER

#include <stddef.h> //get definition for NULL
#include <stdarg.h>
#include <vector>
#include "helpers/SGCTCPPEleven.h"

#ifndef SGCT_DONT_USE_EXTERNAL
#include "external/tinythread.h"
#else
#include <tinythread.h>
#endif

#define TIME_BUFFER_SIZE 9
#define LOG_FILENAME_BUFFER_SIZE 1024 //include path

namespace sgct //simple graphics cluster toolkit
{

class MessageHandler
{
public:
	/*!
		Different notify levels for messages
	*/
	enum NotifyLevel { NOTIFY_ERROR = 0, NOTIFY_IMPORTANT, NOTIFY_VERSION_INFO, NOTIFY_INFO, NOTIFY_WARNING, NOTIFY_DEBUG, NOTIFY_ALL };
	
	/*! Get the MessageHandler instance */
	static MessageHandler * instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new MessageHandler();
		}

		return mInstance;
	}

	/*! Destroy the MessageHandler */
	static void destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	void decode(const char * receivedData, int receivedlength, int clientIndex);
	void print(const char *fmt, ...);
	void print(NotifyLevel nl, const char *fmt, ...);
    void printDebug(NotifyLevel nl, const char *fmt, ...);
    void printIndent(NotifyLevel nl, unsigned int indentation, const char* fmt, ...);
	void sendMessageToServer(const char *fmt);
	void setSendFeedbackToServer(bool state);
    void clearBuffer();
	void setNotifyLevel( NotifyLevel nl );
	NotifyLevel getNotifyLevel();
	void setShowTime( bool state );
	bool getShowTime();
	void setLogToConsole( bool state );
	void setLogToFile( bool state );
	void setLogPath(const char * path, int nodeId = -1);
	void setLogToCallback( bool state );
	void setLogCallback(void(*fnPtr)(const char *));
#ifdef __LOAD_CPP11_FUN__
	void setLogCallback(sgct_cppxeleven::function<void(const char *)> fn);
#endif
	const char * getTimeOfDayStr();
	inline std::size_t getDataSize() { return mBuffer.size(); }

	char * getMessage();

private:
	MessageHandler(void);
	~MessageHandler(void);

	// Don't implement these, should give compile warning if used
	MessageHandler( const MessageHandler & tm );
	const MessageHandler & operator=(const MessageHandler & rhs );
	void printv(const char *fmt, va_list ap);
	void logToFile(const char * buffer);

private:
#ifdef __LOAD_CPP11_FUN__
	typedef sgct_cppxeleven::function<void(const char *)> MessageCallbackFn;
#else
	typedef void(*MessageCallbackFn)(const char *);
#endif

	static MessageHandler * mInstance;

	char * mParseBuffer;
	char * mCombinedBuffer;
	
	std::vector<char> mBuffer;
	std::vector<char> mRecBuffer;
	unsigned char  * headerSpace;

	tthread::atomic<int> mLevel;
	tthread::atomic<bool> mLocal;
	tthread::atomic<bool> mShowTime;
	tthread::atomic<bool> mLogToConsole;
	tthread::atomic<bool> mLogToFile;
	tthread::atomic<bool> mLogToCallback;

	MessageCallbackFn mMessageCallback;
	char mTimeBuffer[TIME_BUFFER_SIZE];
	std::string mFilename;
    size_t mMaxMessageSize;
    size_t mCombinedMessageSize;
};

}

#endif
