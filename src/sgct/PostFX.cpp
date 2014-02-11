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

bool sgct::PostFX::mDeleted = false;

/*!
	Default constructor (doesn't require an OpenGL context)
*/
sgct::PostFX::PostFX()
{
	mUpdateFn = NULL;
	mRenderFn = NULL;

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

	if( !mShaderProgram.addShaderSrc( vertShaderSrc, GL_VERTEX_SHADER, srcType ) )
	{
		MessageHandler::instance()->print( MessageHandler::NOTIFY_ERROR, "PostFX: Pass '%s' failed to load or set vertex shader.\n", mName.c_str() );
		return false;
	}

	if( !mShaderProgram.addShaderSrc( fragShaderSrc, GL_FRAGMENT_SHADER, srcType ) )
	{
		MessageHandler::instance()->print( MessageHandler::NOTIFY_ERROR, "PostFX: Pass '%s' failed to load or set fragment shader.\n", mName.c_str() );
		return false;
	}

	if( !mShaderProgram.createAndLinkProgram() )
	{
		MessageHandler::instance()->print( MessageHandler::NOTIFY_ERROR, "PostFX: Pass '%s' failed to link shader!\n", mName.c_str() );
		return false;
	}

	if( sgct::Engine::instance()->isOGLPipelineFixed() )
		mRenderFn = &PostFX::internalRenderFixedPipeline;
	else
		mRenderFn = &PostFX::internalRender;

	return true;
}

void sgct::PostFX::destroy()
{
	MessageHandler::instance()->print( MessageHandler::NOTIFY_INFO, "PostFX: Pass '%s' destroying shader and texture...\n", mName.c_str() );

	mRenderFn = NULL;
	mUpdateFn = NULL;

	if( !mDeleted )
	{
		mShaderProgram.deleteProgram();
		mDeleted = true;
	}
}

/*!
	Render this pass
*/
void sgct::PostFX::render()
{
	if( mRenderFn != NULL )
		(this->*mRenderFn)();
}

void sgct::PostFX::setUpdateUniformsFunction( void(*fnPtr)() )
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

void sgct::PostFX::internalRender()
{
	sgct::SGCTWindow * win = sgct_core::ClusterManager::instance()->getThisNodePtr()->getActiveWindowPtr();

	//bind target FBO
	win->mFinalFBO_Ptr->attachColorTexture( mOutputTexture );

	mXSize =  win->getXFramebufferResolution();
	mYSize =  win->getYFramebufferResolution();

	//if for some reson the active texture has been reset
	glViewport(0, 0, mXSize, mYSize);
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, mInputTexture );

	mShaderProgram.bind();

	if( mUpdateFn != NULL )
		mUpdateFn();

	win->bindVAO( sgct::SGCTWindow::RenderQuad );
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	win->unbindVAO();

	ShaderProgram::unbind();
}

void sgct::PostFX::internalRenderFixedPipeline()
{
	sgct::SGCTWindow * win = sgct_core::ClusterManager::instance()->getThisNodePtr()->getActiveWindowPtr();

	//bind target FBO
	win->mFinalFBO_Ptr->attachColorTexture( mOutputTexture );

	mXSize =  win->getXFramebufferResolution();
	mYSize =  win->getYFramebufferResolution();

	//if for some reson the active texture has been reset
	glActiveTexture(GL_TEXTURE0); //Open Scene Graph or the user may have changed the active texture
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);

	//if for some reson the active texture has been reset
	glViewport(0, 0, mXSize, mYSize);
    
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glBindTexture(GL_TEXTURE_2D, mInputTexture );

	mShaderProgram.bind();

	if( mUpdateFn != NULL )
		mUpdateFn();

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	win->bindVBO( sgct::SGCTWindow::RenderQuad );
	glClientActiveTexture(GL_TEXTURE0);

	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(0));

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 5*sizeof(float), reinterpret_cast<void*>(8));
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	win->unbindVBO();

	ShaderProgram::unbind();

	glPopClientAttrib();
}
