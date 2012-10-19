#include <GL/glfw.h>
#include <stdio.h>

GLFWmutex mutex = NULL;
int test = 0;

void doit()
{
	glfwLockMutex(mutex);
		test++;
	glfwUnlockMutex(mutex);
}

int main(int argc, char* argv[])
{	
	if( glfwInit() )
	{
		//create mutex
		mutex = glfwCreateMutex();
		
		unsigned int counter = 0;
		
		while(true)
		{
			fprintf(stderr, "Iteration %u\n", counter);

			glfwLockMutex(mutex);
			 doit();
			glfwUnlockMutex(mutex);

			counter++;

			glfwSleep(0.5);
		}

	}

	return 0;
}