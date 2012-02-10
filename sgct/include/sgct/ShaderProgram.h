#ifndef _SHADER_PROGRAM_H_
#define _SHADER_PROGRAM_H_

#include "sgct/Shader.h"

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
	
	/*! 
	Get the location of the attribute, no explicit error checks are performed. 
	Users are responsible of checking the return value of the attribute location
	@param	name Name of the attribute
	@return	Uniform location within the program, -1 if not an active attribute
	*/
	inline GLint getAttribLocation( const std::string & name ) const { return glGetAttribLocation( mProgramId, name.c_str() ); }
	
	/*! 
	Get the location of the attribute, no explicit error checks are performed. 
	Users are responsible of checking the return value of the attribute location
	@param	name Name of the uniform
	@return	Uniform location within the program, -1 if not an active uniform
	*/
	inline GLint getUniformLocation( const std::string & name ) const { return glGetUniformLocation( mProgramId, name.c_str() ); }

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

	bool mIsLinked;						// If this program has been linked

	std::string mName;					// Name of the program, has to be unique
	GLint mProgramId;					// Unique program id

	core_sgct::Shader mVertexShader;	// Handler for the vertex shader
	core_sgct::Shader mFragmentShader;	// Handler for the fragment shader
};

} // sgct
#endif