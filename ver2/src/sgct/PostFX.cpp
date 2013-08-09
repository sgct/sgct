/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/ShaderProgram.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/SGCTWindow.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/PostFX.h"

/*!
	Default constructor (doesn't require an OpenGL context)
*/
sgct::PostFX::PostFX()
{
	mUpdateFn = NULL;

	mInputTexture = GL_FALSE;
	mOutputTexture = GL_FALSE;

	mXSize = 1;
	mYSize = 1;
}

/*!
	\returns true if shader and output/target texture created successfully 
*/
bool sgct::PostFX::init( const std::string & name, const std::string & vertShaderSrc, const std::string & fragShaderSrc, ShaderProgram::ShaderSourceType srcType )
{
	mName = name;
	mShaderProgram.setName( name );

	if( !mShaderProgram.setVertexShaderSrc( vertShaderSrc, srcType ) )
	{
		MessageHandler::Instance()->print( MessageHandler::NOTIFY_ERROR, "PostFX: Pass '%s' failed to load or set vertex shader.\n", mName.c_str() );
		return false;
	}

	if( !mShaderProgram.setFragmentShaderSrc( fragShaderSrc, srcType ) )
	{
		MessageHandler::Instance()->print( MessageHandler::NOTIFY_ERROR, "PostFX: Pass '%s' failed to load or set fragment shader.\n", mName.c_str() );
		return false;
	}

	if( !mShaderProgram.createAndLinkProgram() )
	{
		MessageHandler::Instance()->print( MessageHandler::NOTIFY_ERROR, "PostFX: Pass '%s' failed to link shader!\n", mName.c_str() );
		return false;
	}

	return true;
}

void sgct::PostFX::destroy()
{
	MessageHandler::Instance()->print( MessageHandler::NOTIFY_INFO, "PostFX: Pass '%s' destroying shader and texture...", mName.c_str() );
	
	mUpdateFn = NULL;
	
	mShaderProgram.deleteProgram();
}

/*!
	Render this pass
*/
void sgct::PostFX::render()
{
	sgct_core::SGCTWindow * win = sgct_core::ClusterManager::Instance()->getThisNodePtr()->getActiveWindowPtr();
	
	//bind fisheye target FBO
	win->mFinalFBO_Ptr->attachColorTexture( mOutputTexture );

	mXSize =  win->getXFramebufferResolution();
	mYSize =  win->getYFramebufferResolution();

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/*
		The code below flips the viewport vertically. Top & bottom coords are flipped.
	*/
	glm::mat4 orthoMat = glm::ortho(0.0f, 1.0f, 0.0f, 1.0f);

	//if for some reson the active texture has been reset
	glViewport(0, 0, mXSize, mYSize);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mInputTexture );

	mShaderProgram.bind();

	if( mUpdateFn != NULL )
		mUpdateFn( &orthoMat[0][0] );

	win->bindVAO( sgct_core::SGCTWindow::RenderQuad );
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	win->unbindVAO();

	ShaderProgram::unbind();
}

void sgct::PostFX::setUpdateUniformsFunction( void(*fnPtr)(float * mat) )
{
	mUpdateFn = fnPtr;
}

void sgct::PostFX::setInputTexture( unsigned int inputTex )
{
	mInputTexture = inputTex;
}

void sgct::PostFX::setOutputTexture( unsigned int outputTex )
{
	mOutputTexture = outputTex;
}
