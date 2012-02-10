#ifndef _SHADER_H_
#define _SHADER_H_

#include <GL/glew.h>
#include <GL/wglew.h>
#include <GL/glfw.h>

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
	enum ShaderType { VERTEX = GL_VERTEX_SHADER, FRAGMENT = GL_FRAGMENT_SHADER};

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
	inline GLint getId() const { return mShaderId; }

	/*! Delete the shader */
	inline void deleteShader() { glDeleteShader( mShaderId ); }

private:
	bool checkCompilationStatus() const;

private:

	ShaderType mShaderType;	// The shader type
	GLint mShaderId;		// The shader id used for reference
};

} // core_sgct
#endif