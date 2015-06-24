/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SHADER_H_
#define _SHADER_H_

#include <string>

namespace sgct_core
{

/*!
Simple helper class for handling shaders. Shader can't be used directly, they must be linked to a program.
Current implementation only supports vertex and fragment shader.
*/
class Shader
{
public:
	/*! Enum for deciding shader type */
	typedef int ShaderType;

	Shader();
	Shader( ShaderType shaderType );
	~Shader(void);

	void setShaderType( ShaderType shaderType );
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

} // sgct_core
#endif
