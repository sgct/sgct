/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <sgct/ogl_headers.h>
#include <sgct/ShaderProgram.h>
#include <sgct/MessageHandler.h>

/*!
	Creates a dummy ShaderProgram
*/
sgct::ShaderProgram::ShaderProgram() :
	mIsLinked( false ),
	mProgramId( 0 )
{
	mName.assign("SGCT_NULL");
}
//----------------------------------------------------------------------------//

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
	mProgramId( 0 )
{
	; // Do nothing
}
//----------------------------------------------------------------------------//

/*!
The destructor clears the shader data vector but the program can still be used. The program have to be destroyed explicitly
by calling deleteProgram. This is so that programs can be copied when storing
in containers.
*/
sgct::ShaderProgram::~ShaderProgram(void)
{
	mShaders.clear();
}
//----------------------------------------------------------------------------//

/*!
Will deattach all attached shaders, delete them and then delete the program
*/
void sgct::ShaderProgram::deleteProgram()
{		
	for(std::size_t i=0; i<mShaders.size(); i++)
	{
		if( mShaders[i].mShader.getId() > GL_FALSE )
		{
			glDetachShader( mProgramId, mShaders[i].mShader.getId() );
			mShaders[i].mShader.deleteShader();
		}
	}

	if( mProgramId > GL_FALSE )
	{
		glDeleteProgram( mProgramId );
		mProgramId = GL_FALSE;
	}
}

/*!
@param name Name of the shader program
*/
void sgct::ShaderProgram::setName( const std::string & name )
{
	mName = name;
}

/*!
Will add a shader to the program
@param	src			Where the source is found, can be either a file path or shader source string
@param  type		Type of shader can be one of the following: GL_COMPUTE_SHADER, GL_VERTEX_SHADER, GL_TESS_CONTROL_SHADER, GL_TESS_EVALUATION_SHADER, GL_GEOMETRY_SHADER, or GL_FRAGMENT_SHADER
@param	sSrcType	What type of source code should be read, file or string
@return	Wheter the source code was set correctly or not
*/
bool sgct::ShaderProgram::addShaderSrc( const std::string & src, sgct_core::Shader::ShaderType type, ShaderSourceType sSrcType )
{
	sgct_core::ShaderData sd;
	sd.mShader.setShaderType( type );
	sd.mIsSrcFile = (sSrcType == SHADER_SRC_FILE) ? true : false;
	sd.mShaderSrc = src;

	bool success;
	if( sd.mIsSrcFile )
		success = sd.mShader.setSourceFromFile( src );
	else
		success = sd.mShader.setSourceFromString( src );

	if( success )
		mShaders.push_back( sd );

	return success;
}

/*!
Get the location of the attribute, no explicit error checks are performed.
Users are responsible of checking the return value of the attribute location
@param	name Name of the attribute
@return	Uniform location within the program, -1 if not an active attribute
*/
int sgct::ShaderProgram::getAttribLocation( const std::string & name ) const
{
	return glGetAttribLocation( mProgramId, name.c_str() );
}

/*!
Get the location of the attribute, no explicit error checks are performed.
Users are responsible of checking the return value of the attribute location
@param	name Name of the uniform
@return	Uniform location within the program, -1 if not an active uniform
*/
int sgct::ShaderProgram::getUniformLocation( const std::string & name ) const
{
	return glGetUniformLocation( mProgramId, name.c_str() );
}

/*!
Wrapper for glBindFragDataLocation
bind a user-defined varying out variable to a fragment shader color number
@param	colorNumber The color number to bind the user-defined varying out variable to
@param	name The name of the user-defined varying out variable whose binding to modify
*/
void sgct::ShaderProgram::bindFragDataLocation( unsigned int colorNumber, const std::string & name ) const
{
	glBindFragDataLocation( mProgramId, colorNumber, name.c_str() );
}

/*!
Will create the program and link the shaders. The shader sources must have been set before the
program can be linked. After the program is created and linked no modification to the shader
sources can be made.
@return	Wheter the program was created and linked correctly or not
*/
bool sgct::ShaderProgram::createAndLinkProgram()
{
	if( mShaders.empty() )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ShaderProgram: No shaders has been added to program '%s'!\n", mName.c_str() );
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
	for(std::size_t i=0; i<mShaders.size(); i++)
		if( mShaders[i].mShader.getId() > GL_FALSE )
		{
			glAttachShader( mProgramId, mShaders[i].mShader.getId() );
		}

	glLinkProgram( mProgramId );

	return mIsLinked = checkLinkStatus();
}
//----------------------------------------------------------------------------//

/*!
Reloads a shader by deleting, recompiling and re-linking.
@return	Wheter the program was created and linked correctly or not
*/
bool sgct::ShaderProgram::reload()
{
	sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "ShaderProgram: Reloading program '%s'\n", mName.c_str() );
	
	deleteProgram();

	for(std::size_t i=0; i<mShaders.size(); i++)
	{
		bool success;
		if( mShaders[i].mIsSrcFile )
			success = mShaders[i].mShader.setSourceFromFile( mShaders[i].mShaderSrc );
		else
			success = mShaders[i].mShader.setSourceFromString( mShaders[i].mShaderSrc );

		if( !success )
		{
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "ShaderProgram: Failed to load '%s'!\n", mShaders[i].mShaderSrc.c_str() );
			return false;
		}
	}

	return createAndLinkProgram();
}

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
			sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Could not create shader program [%s]: Already linked to shaders.\n", mName.c_str() );
			return false;
		}

		// If the program is already created but not linked yet it can be reused
		return true;
	}

	mProgramId = glCreateProgram();

	if( mProgramId == 0 )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Could not create shader program [%s]: Unknown error.\n", mName.c_str() );
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

		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Shader program[%s] linking error: %s\n", mName.c_str(), log );

		delete[] log;
		return false;
	}

	return linkStatus == GL_TRUE;
}
//----------------------------------------------------------------------------//

/*!
Use the shader program in the current rendering pipeline
*/
bool sgct::ShaderProgram::bind() const
{
	//
	// Make sure the program is linked before it can be used
	//
	if( !mIsLinked )
	{
		//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Could not set shader program [%s] as active: Program is not linked.\n", mName.c_str() );
		return false;
	}

	glUseProgram( mProgramId );
	return true;
}
//----------------------------------------------------------------------------//

/*!
Unset the shader program in the current rendering pipeline
*/
void sgct::ShaderProgram::unbind()
{
	glUseProgram( GL_FALSE );
}
//----------------------------------------------------------------------------//
