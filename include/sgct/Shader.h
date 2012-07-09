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

#ifndef _SHADER_H_
#define _SHADER_H_

#include <string>

namespace core_sgct
{

/*!
Simple helper class for handling shaders. Shader can't be used directly, they must be linked to a program.
Current implementation only supports vertex and fragment shader.
*/
class Shader
{
public:
	/*! Enum for deciding shader type */
	//enum ShaderType { VERTEX = 0, FRAGMENT};
	typedef int ShaderType;

	Shader( ShaderType shaderType );
	~Shader(void);

	bool setSourceFromFile( const std::string & file );
	bool setSourceFromString( const std::string & srcString );

	std::string getShaderTypeName( ShaderType shaderType ) const;

	/*!
	Get the id for this shader used for linking against programs. The shader source must be set
	befor the id can be used. The shader won't be created until it has the source set
	@return Shader id that can be used for program linking
	*/
	inline int getId() const { return mShaderId; }

	void deleteShader();

private:
	bool checkCompilationStatus() const;

private:

	ShaderType mShaderType;	// The shader type
	int mShaderId;		// The shader id used for reference
};

} // core_sgct
#endif
