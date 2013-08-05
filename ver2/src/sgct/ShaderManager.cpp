/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/ShaderManager.h"
#include "../include/sgct/MessageHandler.h"

#include <algorithm>

// Initiate the manager to NULL
sgct::ShaderManager * sgct::ShaderManager::mInstance = NULL;

/*!
Default constructor does nothing only sets the NULL value shader program
that can be used for comparison will NULL value shader programs
*/
sgct::ShaderManager::ShaderManager(void) :
	NullShader( "SGCT_NULL" )
{
	mCurrentBin = SHADER_BIN_0; //Internal bin
}
//----------------------------------------------------------------------------//

/*!
Destructor deallocates and deletes all shaders
*/
sgct::ShaderManager::~ShaderManager(void)
{
	for(unsigned int i=0; i < NUMBER_OF_SHADER_BINS; i++)
	{
		std::vector<ShaderProgram>::iterator it = mShaders[i].begin();
		std::vector<ShaderProgram>::iterator end = mShaders[i].end();

		for( ; it != end; ++it )
		{
			it->deleteProgram();
		}

		mShaders[i].clear();
	}
}
//----------------------------------------------------------------------------//

/*!
Add a shader program to the manager. The shaders will be compiled and linked to
the program. The name of the shader needs to be unique or it won't be added. 
Both vertex shader and fragment shader source need to be provided, either as a 
link to a shader source code file or as shader source code.
@param	name		Unique name of the shader
@param	vertexSrc	The vertex shader source code, can be a file path or source code
@param	fragmentSrc	The fragment shader source code, can be a file path or source code
@param	sSrcTyp		Shader source code type, if it is a link to a file or source code
@return	Wether the shader was created, linked and added to the manager correctly or not.
*/
bool sgct::ShaderManager::addShader( 
	const std::string & name,
	const std::string & vertexSrc,
	const std::string & fragmentSrc,
	sgct::ShaderProgram::ShaderSourceType sSrcType )
{
	//
	// Check if shader already exists
	//
	if( shaderExists( name ) )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Unable to add shader program [%s]: Name already exists.\n", name.c_str() );
		return false;
	}

	//
	// If shader don't exist, create it and add to container
	//

	ShaderProgram sp( name );
	
	if( !sp.setVertexShaderSrc( vertexSrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}
	
	if( !sp.setFragmentShaderSrc( fragmentSrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}

	if( sp.createAndLinkProgram() )
	{
		mShaders[mCurrentBin].push_back( sp );
		return true;
	}

	// If arrived here the creation and linking of the program didn't work.
	// Return false but printing errors is handled in createAndLinkProgram()
	return false;
}
//----------------------------------------------------------------------------//

/*!
Add a shader program to the manager. The shaders will be compiled and linked to
the program. The name of the shader needs to be unique or it won't be added. 
Both vertex shader and fragment shader source need to be provided, either as a 
link to a shader source code file or as shader source code.
@param	shaderPtr	Pointer to ShaderProgram
@param	name		Unique name of the shader
@param	vertexSrc	The vertex shader source code, can be a file path or source code
@param	fragmentSrc	The fragment shader source code, can be a file path or source code
@param	sSrcTyp		Shader source code type, if it is a link to a file or source code
@return	Wether the shader was created, linked and added to the manager correctly or not.
*/
bool sgct::ShaderManager::addShader( 
	ShaderProgram & shader,
	const std::string & name,
	const std::string & vertexSrc,
	const std::string & fragmentSrc,
	sgct::ShaderProgram::ShaderSourceType sSrcType )
{
	//if something failes set shader pointer to NullShader
	shader = NullShader;
	
	//
	// Check if shader already exists
	//
	if( shaderExists( name ) )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Unable to add shader program [%s]: Name already exists.\n", name.c_str() );
		return false;
	}

	ShaderProgram sp( name );
	
	if( !sp.setVertexShaderSrc( vertexSrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}
	
	if( !sp.setFragmentShaderSrc( fragmentSrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}

	if( sp.createAndLinkProgram() )
	{
		mShaders[mCurrentBin].push_back( sp );
		shader = mShaders[mCurrentBin].back();
		return true;
	}

	// If arrived here the creation and linking of the program didn't work.
	// Return false but printing errors is handled in createAndLinkProgram()
	return false;
}
//----------------------------------------------------------------------------//

/*!
Add a shader program to the manager. The shaders will be compiled and linked to
the program. The name of the shader needs to be unique or it won't be added. 
Both vertex shader and fragment shader source need to be provided, either as a 
link to a shader source code file or as shader source code.
@param	name		Unique name of the shader
@param	vertexSrc	The vertex shader source code, can be a file path or source code
@param	fragmentSrc	The fragment shader source code, can be a file path or source code
@param	geometrySrc	The geometry shader source code, can be a file path or source code
@param	sSrcTyp		Shader source code type, if it is a link to a file or source code
@return	Wether the shader was created, linked and added to the manager correctly or not.
*/
bool sgct::ShaderManager::addShader( 
	const std::string & name,
	const std::string & vertexSrc,
	const std::string & fragmentSrc,
	const std::string & geometrySrc,
	sgct::ShaderProgram::ShaderSourceType sSrcType )
{
	//
	// Check if shader already exists
	//
	if( shaderExists( name ) )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Unable to add shader program [%s]: Name already exists.\n", name.c_str() );
		return false;
	}

	//
	// If shader don't exist, create it and add to container
	//
	ShaderProgram sp( name );
	
	if( !sp.setVertexShaderSrc( vertexSrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}
	
	if( !sp.setFragmentShaderSrc( fragmentSrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}

	if( !sp.setGeometryShaderSrc( geometrySrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}

	if( sp.createAndLinkProgram() )
	{
		mShaders[mCurrentBin].push_back( sp );
		return true;
	}

	// If arrived here the creation and linking of the program didn't work.
	// Return false but printing errors is handled in createAndLinkProgram()
	return false;
}
//----------------------------------------------------------------------------//

/*!
Add a shader program to the manager. The shaders will be compiled and linked to
the program. The name of the shader needs to be unique or it won't be added. 
Both vertex shader and fragment shader source need to be provided, either as a 
link to a shader source code file or as shader source code.
@param	shaderPtr	Pointer to ShaderProgram
@param	name		Unique name of the shader
@param	vertexSrc	The vertex shader source code, can be a file path or source code
@param	fragmentSrc	The fragment shader source code, can be a file path or source code
@param	geometrySrc	The geometry shader source code, can be a file path or source code
@param	sSrcTyp		Shader source code type, if it is a link to a file or source code
@return	Wether the shader was created, linked and added to the manager correctly or not.
*/
bool sgct::ShaderManager::addShader( 
	ShaderProgram & shader,
	const std::string & name,
	const std::string & vertexSrc,
	const std::string & fragmentSrc,
	const std::string & geometrySrc,
	sgct::ShaderProgram::ShaderSourceType sSrcType )
{
	//if something failes set shader pointer to NullShader
	shader = NullShader;
	
	//
	// Check if shader already exists
	//
	if( shaderExists( name ) )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Unable to add shader program [%s]: Name already exists.\n", name.c_str() );
		return false;
	}

	//
	// If shader don't exist, create it and add to container
	//
	ShaderProgram sp( name );
	
	if( !sp.setVertexShaderSrc( vertexSrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}
	
	if( !sp.setFragmentShaderSrc( fragmentSrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}

	if( !sp.setGeometryShaderSrc( geometrySrc, sSrcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}

	if( sp.createAndLinkProgram() )
	{
		mShaders[mCurrentBin].push_back( sp );
		shader = mShaders[mCurrentBin].back();
		return true;
	}

	// If arrived here the creation and linking of the program didn't work.
	// Return false but printing errors is handled in createAndLinkProgram()
	return false;
}
//----------------------------------------------------------------------------//

/*!
	Select which shader bin to use in the manager.

	\param bin the bin to use in the shader manager
*/
void sgct::ShaderManager::setShaderBin( ShaderBinIndex bin )
{
	mCurrentBin = bin;
}

/*!
Removes a shader program from the manager.
All resources allocated for the program will be deallocated and remved
@param	name	Name of the shader program to remove
@return	If the shader program was removed correctly
*/
bool sgct::ShaderManager::removeShader( const std::string & name )
{
	std::vector<ShaderProgram>::iterator shaderIt = std::find( mShaders[mCurrentBin].begin(), mShaders[mCurrentBin].end(), name );

	if( shaderIt == mShaders[mCurrentBin].end() )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Unable to remove shader program [%s]: Not found in current bin.\n", name.c_str() );
		return false;
	}

	shaderIt->deleteProgram();
	mShaders[mCurrentBin].erase( shaderIt );

	return true;
}
//----------------------------------------------------------------------------//

/*!
Set a shader program to be used in the current rendering pipeline
@param	name	Name of the shader program to set as active
@return	Wether the specified shader was set as active or not.
*/
bool sgct::ShaderManager::bindShader( const std::string & name ) const
{
	ShaderProgram sp = getShader( name );

	if( sp == NullShader )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Could not set shader program [%s] as active: Not found in manager.\n", name.c_str() );
		glUseProgram( GL_FALSE ); //unbind to prevent errors
		return false;
	}

	return sp.bind();
}
//----------------------------------------------------------------------------//

/*!
Set a shader program to be used in the current rendering pipeline
@param	shader	Reference to the shader program to set as active
@return	Wether the specified shader was set as active or not.
*/
bool sgct::ShaderManager::bindShader( const ShaderProgram & shader ) const
{
	if( shader == NullShader )
	{
		sgct::MessageHandler::Instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Could not set shader program [Invalid Pointer] as active: Not found in manager.\n");
		glUseProgram( GL_FALSE ); //unbind to prevent errors
		return false;
	}

	return shader.bind();
}
//----------------------------------------------------------------------------//

/*!
	Unbind/unset/diable current shader program in the rendering pipeline.
*/
void sgct::ShaderManager::unBindShader()
{
	glUseProgram( GL_FALSE );
}
//----------------------------------------------------------------------------//
/*!
Get the specified shader program from the shader manager. If the shader is not found
ShaderManager::NullShader will be returned which can be used for comparisons. The NullShader
can not be set as active or used in the rendering pipeline
@param	name	Name of the shader program
@return The specified shader program or ShaderManager::NullShader if shader is not found.
*/
const sgct::ShaderProgram & sgct::ShaderManager::getShader( const std::string & name ) const
{
	std::vector<ShaderProgram>::const_iterator shaderIt = std::find( mShaders[mCurrentBin].begin(), mShaders[mCurrentBin].end(), name );

	return (shaderIt != mShaders[mCurrentBin].end() ) ?
		*shaderIt :
		NullShader;
}

//----------------------------------------------------------------------------//

/*!
Check if a shader program exists in the manager
@param	name	Name of the shader program.
*/
bool sgct::ShaderManager::shaderExists( const std::string & name ) const
{
	std::vector<ShaderProgram>::const_iterator exists = std::find( mShaders[mCurrentBin].begin(), mShaders[mCurrentBin].end(), name );

	return exists != mShaders[mCurrentBin].end();
}
//----------------------------------------------------------------------------//
