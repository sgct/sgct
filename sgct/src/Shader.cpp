#include "sgct/Shader.h"

#include <fstream>
#include <sstream>

/*!
The constructor sets shader type
@param	shaderType	The shader type: vertex or fragment
*/
core_sgct::Shader::Shader( core_sgct::Shader::ShaderType shaderType ) :
	mShaderType( shaderType ),
	mShaderId( 0 )
{
}
//----------------------------------------------------------------------------//

/*!
Destructor does nothing, have to explicitly call delete if shader should be destroyed.
This is because copying between shaders should be allowed (i.e. when storing in containers)
*/
core_sgct::Shader::~Shader(void)
{
}
//----------------------------------------------------------------------------//

/*!
Set the shader source code from a file, will create and compile the shader if it is not already done.
At this point a compiled shader can't have its source reset. Recompilation of shaders is not supported
@param	file	Path to shader file
@return	If setting source and compilation went ok.
*/
bool core_sgct::Shader::setSourceFromFile( const std::string & file )
{
	//
	// Make sure file can be opened
	//
	std::ifstream shaderFile( file.c_str() );

	if( !shaderFile.is_open() )
	{
		fprintf( stderr, 
			"Could not open %s file[%s].\n", 
			getShaderTypeName( mShaderType ).c_str(), 
			file.c_str() );
		return false;
	}

	//
	// Create needed resources by reading file length
	//
	shaderFile.seekg( 0, std::ios_base::end );
	int fileLength = static_cast<int>( shaderFile.tellg() );
		
	shaderFile.seekg( 0, std::ios_base::beg );
	std::string shaderSrc( fileLength, '\0' );

	// 
	// Make sure the file is not empty
	//
	if( fileLength == 0 )
	{
		fprintf( stderr, 
			"Can't create source for %s: empty file [%s].\n",
			getShaderTypeName( mShaderType ).c_str(),
			file.c_str() );
		return false;
	}
	
	//
	// Copy file content to string
	//
	shaderFile.read( &shaderSrc[0], fileLength );
	shaderFile.close();

	//
	// Compile shader source
	//
	return setSourceFromString( shaderSrc );
}
//----------------------------------------------------------------------------//

/*!
Set the shader source code from a file, will create and compile the shader if it is not already done.
At this point a compiled shader can't have its source reset. Recompilation of shaders is not supported
@param	sourceString	String with shader source code
@return	If setting the source and compilation went ok.
*/
bool core_sgct::Shader::setSourceFromString( const std::string & sourceString )
{
	//
	// At this point no resetting of shaders are supported
	//
	if( mShaderId > 0 )
	{
		fprintf( stderr, 
			"%s is alread set for specified shader.\n", 
			getShaderTypeName( mShaderType ).c_str() );
		return false;
	}

	//
	// Prepare source code for shader
	//
	const char * shaderSrc[] = { sourceString.c_str() };

	mShaderId = glCreateShader( mShaderType );
	glShaderSource( mShaderId, 1, shaderSrc, NULL );

	//
	// Compile and check status
	//
	glCompileShader( mShaderId );

	return checkCompilationStatus();
}
//----------------------------------------------------------------------------//

/*!
Will check the compilation status of the shader and output any errors from the shader log
return	Status of the compilation
*/
bool core_sgct::Shader::checkCompilationStatus() const
{
	GLint compilationStatus;
	glGetShaderiv( mShaderId, GL_COMPILE_STATUS, &compilationStatus );

	if( compilationStatus == GL_FALSE )
   	{
		GLint logLength;
		glGetShaderiv( mShaderId, GL_INFO_LOG_LENGTH, &logLength );

		if( logLength == 0 )
		{
			fprintf( stderr, "%s compile error: Unknown error\n", getShaderTypeName( mShaderType ) );
			return false;
		}

		GLchar * log = new GLchar[logLength];
		
		glGetShaderInfoLog( mShaderId, logLength, NULL, log );
		fprintf( stderr, "%s compile error: %s\n", getShaderTypeName( mShaderType ).c_str(), log );

		delete[] log;

		return false;;
	}

	return compilationStatus == GL_TRUE;
}
//----------------------------------------------------------------------------//

/*!
Will return the name of the shader type
@param	shaderType	The shader type
@return	Sahder type name
*/
std::string core_sgct::Shader::getShaderTypeName( ShaderType shaderType ) const
{
	switch( shaderType )
	{
	case VERTEX:
		return "Vertex shader";
	case FRAGMENT:
		return "Fragment shader";
	default:
		return "Unknown shader";
	};
}
//----------------------------------------------------------------------------//