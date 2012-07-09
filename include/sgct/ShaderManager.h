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

#ifndef _SHADER_MANAGER_H_
#define _SHADER_MANAGER_H_

#include <string>
#include <vector>
#include "ShaderProgram.h"

namespace sgct
{

/*!
For managing shader programs. Implemented as a singleton
The current implementation of shader programs only support vertex and fragment shaders.
*/
class ShaderManager
{
public:
	enum ShaderSourceType
	{ 
		SHADER_SRC_FILE, 
		SHADER_SRC_STRING
	};

	~ShaderManager(void);

	bool addShader( 
		const std::string & name,
		const std::string & vertexSrc,
		const std::string & fragmentSrc,
		ShaderSourceType sSrcType = SHADER_SRC_FILE );

	bool removeShader( const std::string & name );
	bool bindShader( const std::string & name ) const;
	void unBindShader();

	bool shaderExists( const std::string & name ) const;
	
	const ShaderProgram & getShader( const std::string & name ) const;

	/*! Get the manager instance */
	static ShaderManager * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new ShaderManager();
		}

		return mInstance;
	}

	/*! Destroy the ShaderManager */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

private:
	ShaderManager(void);

	// Don't implement these, should give compile warning if used
	ShaderManager( const ShaderManager & tm );
	const ShaderManager & operator=(const ShaderManager & rhs );

public:
	// A shader program that never will be initialized.
	// Will be returned for not found programs and can be used as 
	// comparison for NULL values
	ShaderProgram NullShader;	
	
private:

	static ShaderManager * mInstance;		// Instantiation of the manager

	std::vector<ShaderProgram> mShaders;	// Active shaders in the manager
};

} // sgct
#endif