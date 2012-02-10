#ifndef _SHADER_MANAGER_H_
#define _SHADER_MANAGER_H_

#include <string>
#include <vector>
#include "ShaderProgram.h"

namespace sgct
{

/*!
For managing shader programs. Implemented as a singleton
The current implementation of shader programs only support vertex and fragment shaders.
*/
class ShaderManager
{
public:
	enum ShaderSourceType
	{ 
		SHADER_SRC_FILE, 
		SHADER_SRC_STRING
	};

	~ShaderManager(void);

	bool addShader( 
		const std::string & name,
		const std::string & vertexSrc,
		const std::string & fragmentSrc,
		ShaderSourceType sSrcType = SHADER_SRC_FILE );

	bool removeShader( const std::string & name );
	bool useShader( const std::string & name ) const;

	bool shaderExists( const std::string & name ) const;
	
	const ShaderProgram & getShader( const std::string & name ) const;

	/*! Get the manager instance */
	static ShaderManager * Instance()
	{
		if( mInstance == NULL )
		{
			mInstance = new ShaderManager();
		}

		return mInstance;
	}

	/*! Destroy the ShaderManager */
	static void Destroy()
	{
		if( mInstance != NULL )
		{
			delete mInstance;
			mInstance = NULL;
		}
	}

private:
	ShaderManager(void);

	// Don't implement these, should give compile warning if used
	ShaderManager( const ShaderManager & tm );
	const ShaderManager & operator=(const ShaderManager & rhs );

public:
	// A shader program that never will be initialized.
	// Will be returned for not found programs and can be used as 
	// comparison for NULL values
	ShaderProgram NullShader;	
	
private:

	static ShaderManager * mInstance;		// Instantiation of the manager

	std::vector<ShaderProgram> mShaders;	// Active shaders in the manager
};

} // sgct
#endif