/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _POST_FX_H_
#define _POST_FX_H_

#include "ShaderProgram.h"

namespace sgct
{

/*!
	Class that holds a post effect pass
*/
class PostFX
{
public:
	PostFX();
	bool init( const std::string & name, const std::string & vertShaderSrc, const std::string & fragShaderSrc, ShaderProgram::ShaderSourceType srcType = ShaderProgram::SHADER_SRC_FILE);
	void destroy();
	void render();
	void setUpdateUniformsFunction( void(*fnPtr)() );
	void setInputTexture( unsigned int inputTex );
	void setOutputTexture( unsigned int outputTex );
	
	/*!
		\returns the output texture
	*/
	inline unsigned int getOutputTexture() { return mOutputTexture; }
	/*!
		\returns the input texture
	*/
	inline unsigned int getInputTexture() { return mInputTexture; }
	/*!
		\returns the shader pointer
	*/
	inline ShaderProgram * getShaderProgram() { return &mShaderProgram; }
	/*!
		\returns name of this post effect pass
	*/
	inline const std::string & getName() { return mName; }

private:
	void internalRender();
	void internalRenderFixedPipeline();

private:
	void (*mUpdateFn)();
	void (PostFX::*mRenderFn)(void);

	ShaderProgram mShaderProgram;
	unsigned int mInputTexture;
	unsigned int mOutputTexture;
	
	int mXSize, mYSize;
	std::string mName;
	static bool mDeleted;
};
}

#endif