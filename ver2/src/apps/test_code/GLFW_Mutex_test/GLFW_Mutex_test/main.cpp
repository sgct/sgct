#include <stdio.h>

#define USE_STD_MUTEX
//#define _GLIBCXX_HAS_GTHREADS 1
//#define __GTHREADS_CXX0X 1

#ifdef USE_STD_MUTEX
#include <mutex>
#include <windows.h>
std::mutex mutex;
#else
#include <GL/glfw.h>
GLFWmutex mutex = NULL;
#endif

int test = 0;

void lockMutex()
{
	#ifdef USE_STD_MUTEX
        // mutex.lock(); //crash if used several times
		while( !mutex.try_lock() )
		{
			;
		}
	#else
		glfwLockMutex(mutex);
	#endif
}

void unlockMutex()
{
	#ifdef USE_STD_MUTEX
		mutex.unlock();
	#else
		glfwUnlockMutex(mutex);
	#endif
}

void sleepFun()
{
	#ifdef USE_STD_MUTEX
		Sleep(200);
	#else
		glfwSleep(0.2);
	#endif
}

void doit()
{
	lockMutex();
		test++;
	unlockMutex();
}

int main(int argc, char* argv[])
{
#ifdef USE_STD_MUTEX

#else
	if( glfwInit() )
	{
		//create mutex
		mutex = glfwCreateMutex();
	}
	else
		return -1;
#endif

	unsigned int counter = 0;

	while(true)
	{
		fprintf(stderr, "Iteration %u\n", counter);

		lockMutex();
			doit();
		unlockMutex();

		counter++;

		sleepFun();

		if(counter > 10)
			break;
	}

	system("pause");

	return 0;
}
