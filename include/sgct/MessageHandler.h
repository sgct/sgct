/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
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
    void sendMessagesToServer( bool state ) { mLocal = !state; }
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
