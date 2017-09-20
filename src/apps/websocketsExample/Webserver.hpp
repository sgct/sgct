#ifndef _WEB_SERVER_
#define _WEB_SERVER_

#include <thread>
#include <mutex>
#include <atomic>

//Webserver singleton
class Webserver
{
public:
	typedef void(*WebMessageCallbackFn)(const char *, size_t);

	/*! Get the Webserver instance */
	static Webserver * instance()
	{
		if (mInstance == NULL)
		{
			mInstance = new Webserver();
		}
		return mInstance;
	}

	/*! Destroy the Webserver */
	static void destroy()
	{
		if (mInstance != NULL)
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

	void start(int port, int timeout_ms = 5);
	void setCallback(WebMessageCallbackFn cb);
	inline int getPort() { return mPort; }
    inline int getTimeout() { return mTimeout; }
    unsigned int generateSessionIndex();

	WebMessageCallbackFn mWebMessageCallbackFn;
	//std::atomic<bool> mRunning;
	std::atomic<bool> mRunning;
	
private:
	Webserver();
	~Webserver();

	// Don't implement these, should give compile warning if used
	Webserver(const Webserver & ws);
	Webserver(Webserver &&rhs);
	const Webserver & operator=(const Webserver & rhs);

	static void worker();

	static Webserver * mInstance;
    int mPort;
    int mTimeout; //in ms
    unsigned int mSessionIndex;
	std::mutex mMutex;
	std::thread * mMainThreadPtr;
};

#endif