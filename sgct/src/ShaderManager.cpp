#include "sgct/ShaderManager.h"

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
	; // Do nothing
}
//----------------------------------------------------------------------------//

/*!
Destructor deallocates and deletes all shaders
*/
sgct::ShaderManager::~ShaderManager(void)
{
	std::vector<ShaderProgram>::iterator it = mShaders.begin();
	std::vector<ShaderProgram>::iterator end = mShaders.end();

	for( ; it != end; ++it )
	{
		it->deleteProgram();
	}

	mShaders.clear();
}
//----------------------------------------------------------------------------//

/*!
Add a shader program to the manager. The shaders will be compiled and linked to
the program. The name of the shader needs to be unique or it won't be added. 
Both vertex shader and fragment shader source need to be provided, either as a 
link to a shader source code file or as shader source code.
@param	name		Unique name of the shader
@param	vertexSrc	The vertex shader source code, can be a file path or source code
@param	fragmentSrc	The fragment shader source code, van be a file path or source code
@param	sSrcTyp		Shader source code type, if it is a link to a file or source code
@return	Wether the shader was created, linked and added to the manager correctly or not.
*/
bool sgct::ShaderManager::addShader( 
	const std::string & name,
	const std::string & vertexSrc,
	const std::string & fragmentSrc,
	ShaderManager::ShaderSourceType sSrcType )
{
	//
	// Check if shader already exists
	//
	if( shaderExists( name ) )
	{
		fprintf( stderr, "Unable to add shader program [%s]: Name already exists.\n", name.c_str() );
		return false;
	}

	//
	// If shader don't exist, create it and add to container
	//
	ShaderProgram::ShaderSourceType srcType = (sSrcType == SHADER_SRC_FILE ) ?
		ShaderProgram::SHADER_SOURCE_FILE :
		ShaderProgram::SHADER_SOURCE_STRING;

	ShaderProgram sp( name );
	
	if( !sp.setVertexShaderSrc( vertexSrc, srcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}
	
	if( !sp.setFramgentShaderSrc( fragmentSrc, srcType ) )
	{
		// Error messaging handled when setting source
		return false;
	}

	if( sp.createAndLinkProgram() )
	{
		mShaders.push_back( sp );
		return true;
	}

	// If arrived here the creation and linking of the program didn't work.
	// Return false but printing errors is handled in createAndLinkProgram()
	return false;
}
//----------------------------------------------------------------------------//

/*!
Removes a shader program from the manager.
All resources allocated for the program will be deallocated and remved
@param	name	Name of the shader program to remove
@return	If the shader program was removed correctly
*/
bool sgct::ShaderManager::removeShader( const std::string & name )
{
	std::vector<ShaderProgram>::iterator shaderIt = std::find( mShaders.begin(), mShaders.end(), name );

	if( shaderIt == mShaders.end() )
	{
		fprintf( stderr, "Unable to remove shader program [%s]: Not found in manager.\n", name.c_str() );
		return false;
	}

	shaderIt->deleteProgram();
	mShaders.erase( shaderIt );

	return true;
}
//----------------------------------------------------------------------------//

/*!
Set a shader program to be used in the current rendering pipeline
@param	name	Name of the shader program to set as active
@return	Wether the specified shader was set as active or not.
*/
bool sgct::ShaderManager::useShader( const std::string & name ) const
{
	ShaderProgram sp = getShader( name );

	if( sp == NullShader )
	{
		fprintf( stderr, "Could not set shader program [%s] as active: Not found in manager.\n", name.c_str() );
		return false;
	}

	return sp.use();
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
	std::vector<ShaderProgram>::const_iterator shaderIt = std::find( mShaders.begin(), mShaders.end(), name );

	return (shaderIt != mShaders.end() ) ?
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
	std::vector<ShaderProgram>::const_iterator exists = std::find( mShaders.begin(), mShaders.end(), name );

	return exists != mShaders.end();
}
//----------------------------------------------------------------------------//