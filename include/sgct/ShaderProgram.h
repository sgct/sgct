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

#ifndef _SHADER_PROGRAM_H_
#define _SHADER_PROGRAM_H_

#include "Shader.h"

namespace sgct
{

/*!
Helper class for handling compiling, linking and using shader programs.
Current implementation only supports vertex and fragment shader. Uniform and
attribute handling must be managed explicitly but it is possible to poll the
Shader program for uniform and attribute locations.
*/
class ShaderProgram
{
public:
	/*! If shader source should be loaded from file or read as is */
	enum ShaderSourceType{ SHADER_SOURCE_FILE, SHADER_SOURCE_STRING };

	ShaderProgram( const std::string & name );
	~ShaderProgram( void );

	void deleteProgram();

	bool setVertexShaderSrc( const std::string & src, ShaderSourceType sSrcType = SHADER_SOURCE_FILE );
	bool setFramgentShaderSrc( const std::string & src, ShaderSourceType sSrcType = SHADER_SOURCE_FILE );

	bool createAndLinkProgram();

	bool use() const;

	int getAttribLocation( const std::string & name ) const;
	int getUniformLocation( const std::string & name ) const;

	/*! Less than ShaderProgram operator */
	inline bool operator<( const ShaderProgram & rhs ) const { return mName < rhs.mName; }

	/*! Equal to ShaderProgram operator */
	inline bool operator==( const ShaderProgram & rhs ) const { return mName == rhs.mName; }

	/*! Not equal to ShaderProgram operator */
	inline bool operator!=( const ShaderProgram & rhs ) const { return mName != rhs.mName; }

	/*! Equal to string operator */
	inline bool operator==( const std::string & rhs ) const { return mName == rhs; }

private:

	bool createProgram();
	bool checkLinkStatus() const;

private:

	std::string mName;					// Name of the program, has to be unique
	bool mIsLinked;						// If this program has been linked
	int mProgramId;						// Unique program id

	core_sgct::Shader mVertexShader;	// Handler for the vertex shader
	core_sgct::Shader mFragmentShader;	// Handler for the fragment shader
};

} // sgct
#endif
