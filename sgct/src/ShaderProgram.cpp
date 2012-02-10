#include "sgct/ShaderProgram.h"

/*!
Default only sets the program name.Shaders objects won't be created until
the any shader source code is set. The program will be created when the 
createAndLink() function is called. Make sure the shader sources are
set before calling it.
@param	name	Name of the shader program. Must be unique
*/
sgct::ShaderProgram::ShaderProgram( const std::string & name ) :
	mName( name ),
	mIsLinked( false ),
	mProgramId( 0 ),
	mVertexShader( core_sgct::Shader::VERTEX ),
	mFragmentShader( core_sgct::Shader::FRAGMENT )
{
	; // Do nothing
}
//----------------------------------------------------------------------------//

/*!
The destructor does nothing. The program have to be destroyed explicitly
by calling destroy. This is so that programs can be copied when storing
in containers.
*/
sgct::ShaderProgram::~ShaderProgram(void)
{
}
//----------------------------------------------------------------------------//

/*!
Will deattach all attached shaders, delete them and then delete the program
*/
void sgct::ShaderProgram::deleteProgram()
{
	if( mVertexShader.getId() > 0 )
	{
		glDetachShader( mProgramId, mVertexShader.getId() );
		mVertexShader.deleteShader();
	}

	if( mFragmentShader.getId() > 0 )
	{
		glDetachShader( mProgramId, mFragmentShader.getId() );
		mFragmentShader.getId();
	}

	if( mProgramId > 0 )
	{
		glDeleteProgram( mProgramId );
	}
}

/*!
Will set the vertex shader source code.
@param	src			Where the source is found, can be either a file path or shader source string
@param	sSrcType	What type of source code should be read, file or string
@return	Wheter the source code was set correctly or not
*/
bool sgct::ShaderProgram::setVertexShaderSrc( const std::string & src, sgct::ShaderProgram::ShaderSourceType sSrcType )
{
	if( sSrcType == SHADER_SOURCE_FILE )
	{
		return mVertexShader.setSourceFromFile( src );
	}
	else
	{
		return mVertexShader.setSourceFromString( src );
	}
}
//----------------------------------------------------------------------------//

/*!
Will set the fragment shader source code.
@param	src			Where the source is found, can be either a file path or shader source string
@param	sSrcType	What type of source code should be read, file or string
@return	Wheter the source code was set correctly or not
*/
bool sgct::ShaderProgram::setFramgentShaderSrc( const std::string & src, sgct::ShaderProgram::ShaderSourceType sSrcType )
{
	if( sSrcType == SHADER_SOURCE_FILE )
	{
		return mFragmentShader.setSourceFromFile( src );
	}
	else
	{
		return mFragmentShader.setSourceFromString( src );
	}
}
//----------------------------------------------------------------------------//

/*!
Will create the program and link the shaders. The shader sources must have been set before the
program can be linked. After the program is created and linked no modification to the shader
sources can be made.
@return	Wheter the program was created and linked correctly or not
*/
bool sgct::ShaderProgram::createAndLinkProgram()
{
	//
	// If the shader id's are 0 they are not created yet
	//
	if( mVertexShader.getId() == 0 || mFragmentShader.getId() == 0 )
	{
		fprintf( stderr, "Can't create shader program [%s], all shader sources not properly set.\n", mName.c_str() );
		return false;
	}

	//
	// Create the program
	//
	if( !createProgram() )
	{
		// Error text handled in createProgram()
		return false;
	}

	//
	// Link shaders
	//
	glAttachShader( mProgramId, mVertexShader.getId() );
	glAttachShader( mProgramId, mFragmentShader.getId() );
	
	glLinkProgram( mProgramId );

	return mIsLinked = checkLinkStatus();
}
//----------------------------------------------------------------------------//

/*!
Will create the program.
@return	Wheter the program was properly created or not
*/
bool sgct::ShaderProgram::createProgram()
{
	if( mProgramId > 0 )
	{
		// If the program is already created don't recreate it.
		// but should only return true if it hasn't been linked yet.
		// if it has been linked already it can't be reused
		if( mIsLinked )
		{
			fprintf( stderr, "Could not create shader program [%s]: Already linked to shaders.\n", mName.c_str() );
			return false;
		}

		// If the program is already created but not linked yet it can be reused
		return true;
	}

	mProgramId = glCreateProgram();

	if( mProgramId == 0 )
	{
		fprintf( stderr, "Could not create shader program [%s]: Unknown error.\n", mName.c_str() );
		return false;
	}

	return true;	
}
//----------------------------------------------------------------------------//	

/*!
Will check the link status of the program and output any errors from the program log
return	Status of the compilation
*/
bool sgct::ShaderProgram::checkLinkStatus() const
{
	GLint linkStatus;
	glGetProgramiv( mProgramId, GL_LINK_STATUS, &linkStatus );

    if( linkStatus == GL_FALSE )
	{
		GLint logLength;
		glGetProgramiv( mProgramId, GL_INFO_LOG_LENGTH, &logLength );
		
		GLchar * log = new GLchar[logLength];
		glGetProgramInfoLog( mProgramId, logLength, NULL, log );

		fprintf( stderr, "Shader program[%s] linking error: %s\n", mName.c_str(), log );

		delete[] log;
		return false;
	}
	
	return linkStatus == GL_TRUE;
}
//----------------------------------------------------------------------------//

/*!
Use the shader program in the current rendering pipeline
*/
bool sgct::ShaderProgram::use() const
{
	//
	// Make sure the program is linked before it can be used
	//
	if( !mIsLinked ) 
	{
		fprintf( stderr, "Could not set shader program [%s] as active: Program is not linked.\n", mName.c_str() );
		return false; 
	}

	glUseProgram( mProgramId );
	return true;
}
//----------------------------------------------------------------------------//