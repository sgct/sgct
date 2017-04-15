/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/SphericalMirrorProjection.h>
#include <sgct/SGCTSettings.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/shaders/SGCTInternalSphericalProjectionShaders.h>
#include <sgct/shaders/SGCTInternalSphericalProjectionShaders_modern.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <sstream>
#include <algorithm>

//#define DebugCubemap

sgct_core::SphericalMirrorProjection::SphericalMirrorProjection()
{
    mInternalRenderFn = &SphericalMirrorProjection::renderInternalFixedPipeline;
    mInternalRenderCubemapFn = &SphericalMirrorProjection::renderCubemapInternalFixedPipeline;
    
    mTilt = 0.0f;
    mDiameter = 2.4f;
    
    mTexLoc = -1;
    mMatrixLoc = -1;
}

sgct_core::SphericalMirrorProjection::~SphericalMirrorProjection()
{
    
}

/*!
Update projection when aspect ratio changes for the viewport.
*/
void sgct_core::SphericalMirrorProjection::update(float width, float height)
{
    ;
}

/*!
Render the non linear projection to currently bounded FBO
*/
void sgct_core::SphericalMirrorProjection::render()
{
    (this->*mInternalRenderFn)();
}

/*!
Render the enabled faces of the cubemap
*/
void sgct_core::SphericalMirrorProjection::renderCubemap(std::size_t * subViewPortIndex)
{
    (this->*mInternalRenderCubemapFn)(subViewPortIndex);
}

/*!
Set the dome tilt angle used in the spherical mirror renderer.
The tilt angle is from the horizontal.

@param angle the tilt angle in degrees
*/
void sgct_core::SphericalMirrorProjection::setTilt(float angle)
{
    mTilt = angle;
}

/*!
Set the mesh path for selected cube face.

@param mt the mesh face
@param str the path to the mesh
*/
void sgct_core::SphericalMirrorProjection::setMeshPath(MeshFace mf, const char * str)
{
    mMeshPaths[mf].assign(str);
}

void sgct_core::SphericalMirrorProjection::initTextures()
{
    if (sgct::Engine::instance()->getRunMode() <= sgct::Engine::OpenGL_Compablity_Profile)
    {
        glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_TEXTURE_BIT);
        glEnable(GL_TEXTURE_2D);
    }

    for (unsigned int i = 0; i < 6; i++)
    {
        if (mSubViewports[i].isEnabled())
        {
            generateMap(static_cast<TextureIndex>(CubeFaceRight+i), mTextureInternalFormat, mTextureFormat, mTextureType);
            if (sgct::Engine::checkForOGLErrors())
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "NonLinearProjection: %dx%d cube face texture (id: %d) generated!\n",
                    mCubemapResolution, mCubemapResolution, mTextures[CubeFaceRight + i]);
            else
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "NonLinearProjection: Error occured while generating %dx%d cube face texture (id: %d)!\n",
                    mCubemapResolution, mCubemapResolution, mTextures[CubeFaceRight + i]);
        }
    }

    if (sgct::Engine::instance()->getRunMode() <= sgct::Engine::OpenGL_Compablity_Profile)
        glPopAttrib();
}

void sgct_core::SphericalMirrorProjection::initVBO()
{
    if (Viewport * vp = dynamic_cast<Viewport*>(sgct::Engine::instance()->getCurrentWindowPtr()->getCurrentViewport()))
    {
        for (int i = 0; i < LAST_MESH; i++)
            mMeshes[i].readAndGenerateMesh(
                mMeshPaths[i],
                vp);
    }
}

void sgct_core::SphericalMirrorProjection::initViewports()
{
    enum cubeFaces { Pos_X = 0, Neg_X, Pos_Y, Neg_Y, Pos_Z, Neg_Z };

    //radius is needed to calculate the distance to all view planes
    float radius = mDiameter / 2.0f;

    //setup base viewport that will be rotated to create the other cubemap views
    //+Z face
    const glm::vec4 lowerLeftBase(-radius, -radius, radius, 1.0f);
    const glm::vec4 upperLeftBase(-radius, radius, radius, 1.0f);
    const glm::vec4 upperRightBase(radius, radius, radius, 1.0f);

    //tilt
    glm::mat4 tiltMat = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f - mTilt), glm::vec3(1.0f, 0.0f, 0.0f));

    //add viewports
    for (unsigned int i = 0; i<6; i++)
    {
        std::stringstream ss;
        ss << "SphericalMirror " << i;
        mSubViewports[i].setName( ss.str());

        glm::vec4 lowerLeft = lowerLeftBase;
        glm::vec4 upperLeft = upperLeftBase;
        glm::vec4 upperRight = upperRightBase;

        glm::mat4 rotMat(1.0f);

        switch (i)
        {
        case Pos_X: //+X face, right
            rotMat = glm::rotate(tiltMat, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            break;

        case Neg_X: //-X face, left
            rotMat = glm::rotate(tiltMat, glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            break;

        case Pos_Y: //+Y face, bottom
            mSubViewports[i].setEnabled(false);
            rotMat = glm::rotate(tiltMat, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            break;

        case Neg_Y: //-Y face, top
            rotMat = glm::rotate(tiltMat, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
            break;

        case Pos_Z: //+Z face, front
            rotMat = tiltMat;
            break;

        case Neg_Z: //-Z face, back
            mSubViewports[i].setEnabled(false);
            rotMat = glm::rotate(tiltMat, glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            break;
        }//end switch

        //add viewplane vertices
        mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::LowerLeft, glm::vec3(rotMat * lowerLeft));
        mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::UpperLeft, glm::vec3(rotMat * upperLeft));
        mSubViewports[i].getProjectionPlane()->setCoordinate(sgct_core::SGCTProjectionPlane::UpperRight, glm::vec3(rotMat * upperRight));
    }//end for
}

void sgct_core::SphericalMirrorProjection::initShaders()
{
    if (sgct::Engine::instance()->isOGLPipelineFixed())
    {
        mInternalRenderFn = &SphericalMirrorProjection::renderInternalFixedPipeline;
        mInternalRenderCubemapFn = &SphericalMirrorProjection::renderCubemapInternalFixedPipeline;
    }
    else
    {
        mInternalRenderFn = &SphericalMirrorProjection::renderInternal;
        mInternalRenderCubemapFn = &SphericalMirrorProjection::renderCubemapInternal;
    }
    
    if (mStereo || mPreferedMonoFrustumMode != Frustum::MonoEye) // if any frustum mode other than Mono (or stereo)
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Stereo rendering not supported in spherical projection!\n");

    //reload shader program if it exists
    if (mShader.isLinked())
        mShader.deleteProgram();

    std::string sphericalMirrorFragmentShader;
    std::string sphericalMirrorVertexShader;

    if (sgct::Engine::instance()->isOGLPipelineFixed())
    {
        sphericalMirrorVertexShader = sgct_core::shaders::Spherical_Projection_Vert_Shader;
        sphericalMirrorFragmentShader = sgct_core::shaders::Spherical_Projection_Frag_Shader;
    }
    else //modern pipeline
    {
        sphericalMirrorVertexShader = sgct_core::shaders_modern::Spherical_Projection_Vert_Shader;
        sphericalMirrorFragmentShader = sgct_core::shaders_modern::Spherical_Projection_Frag_Shader;
    }

    //replace glsl version
    sgct_helpers::findAndReplace(sphericalMirrorVertexShader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());
    sgct_helpers::findAndReplace(sphericalMirrorFragmentShader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());

    if (!mShader.addShaderSrc(sphericalMirrorVertexShader, GL_VERTEX_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load spherical mirror vertex shader:\n%s\n", sphericalMirrorVertexShader.c_str());
    if (!mShader.addShaderSrc(sphericalMirrorFragmentShader, GL_FRAGMENT_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load spherical mirror fragment shader\n%s\n", sphericalMirrorFragmentShader.c_str());

    mShader.setName("SphericalMirrorShader");
    mShader.createAndLinkProgram();
    mShader.bind();

    mTexLoc = mShader.getUniformLocation("Tex");
    glUniform1i(mTexLoc, 0);

    if (!sgct::Engine::instance()->isOGLPipelineFixed())
        mMatrixLoc = mShader.getUniformLocation("MVP");

    sgct::ShaderProgram::unbind();
}

void sgct_core::SphericalMirrorProjection::drawCubeFace(const std::size_t & face)
{
    glLineWidth(1.0);
    sgct::Engine::instance()->getWireframe() ? glPolygonMode(GL_FRONT_AND_BACK, GL_LINE) : glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    //reset depth function (to opengl default)
    glDepthFunc(GL_LESS);

    setupViewport(face);

#if defined DebugCubemap
    float color[4];
    switch (face)
    {
    case 0:
        color[0] = 0.5f;
        color[1] = 0.0f;
        color[2] = 0.0f;
        color[3] = 1.0f;
        break;

    case 1:
        color[0] = 0.5f;
        color[1] = 0.5f;
        color[2] = 0.0f;
        color[3] = 1.0f;
        break;

    case 2:
        color[0] = 0.0f;
        color[1] = 0.5f;
        color[2] = 0.0f;
        color[3] = 1.0f;
        break;

    case 3:
        color[0] = 0.0f;
        color[1] = 0.5f;
        color[2] = 0.5f;
        color[3] = 1.0f;
        break;

    case 4:
        color[0] = 0.0f;
        color[1] = 0.0f;
        color[2] = 0.5f;
        color[3] = 1.0f;
        break;

    case 5:
        color[0] = 0.5f;
        color[1] = 0.0f;
        color[2] = 0.5f;
        color[3] = 1.0f;
        break;
    }
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
#else
    if (sgct::Engine::mInstance->mClearBufferFnPtr != SGCT_NULL_PTR)
        sgct::Engine::mInstance->mClearBufferFnPtr();
    else
    {
        const float * colorPtr = sgct::Engine::instance()->getClearColor();
        glClearColor(colorPtr[0], colorPtr[1], colorPtr[2], colorPtr[3]);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
#endif

    if (sgct::Engine::instance()->isOGLPipelineFixed())
    {
        glMatrixMode(GL_PROJECTION);
        SGCTProjection * proj = mSubViewports[face].getProjection(sgct::Engine::instance()->getCurrentFrustumMode());
        glLoadMatrixf(glm::value_ptr(proj->getProjectionMatrix()));
        glMatrixMode(GL_MODELVIEW);
        glLoadMatrixf(glm::value_ptr(proj->getViewMatrix() * sgct::Engine::instance()->getModelMatrix()));
    }

    //render
    sgct::Engine::mInstance->mDrawFnPtr();

    //restore polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void sgct_core::SphericalMirrorProjection::blitCubeFace(TextureIndex &ti)
{
    //copy AA-buffer to "regular"/non-AA buffer
    mCubeMapFBO_Ptr->bindBlit(); //bind separate read and draw buffers to prepare blit operation
    attachTextures(ti);
    mCubeMapFBO_Ptr->blit();
}

void sgct_core::SphericalMirrorProjection::attachTextures(TextureIndex &ti)
{
    mCubeMapFBO_Ptr->attachColorTexture(mTextures[ti]);
}

void sgct_core::SphericalMirrorProjection::renderInternal()
{
    sgct::Engine::mInstance->enterCurrentViewport();

    sgct::SGCTWindow * winPtr = sgct::Engine::instance()->getCurrentWindowPtr();
    sgct_core::BaseViewport * vpPtr = winPtr->getCurrentViewport();

    float aspect = winPtr->getAspectRatio() *
        (vpPtr->getXSize() / vpPtr->getYSize());

    glm::mat4 MVP = glm::ortho(-aspect, aspect, -1.0f, 1.0f, -1.0f, 1.0f);

    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bindShaderProgram();

    //if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);
    
    glDisable(GL_CULL_FACE);
    bool alpha = sgct::Engine::mInstance->getCurrentWindowPtr()->getAlpha();
    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
        glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glUniform1i(mTexLoc, 0);
    glUniformMatrix4fv(mMatrixLoc, 1, GL_FALSE, &MVP[0][0]);

    glBindTexture(GL_TEXTURE_2D, mTextures[CubeFaceFront]);
    mMeshes[BOTTOM_MESH].render(CorrectionMesh::WARP_MESH);

    glBindTexture(GL_TEXTURE_2D, mTextures[CubeFaceLeft]);
    mMeshes[LEFT_MESH].render(CorrectionMesh::WARP_MESH);

    glBindTexture(GL_TEXTURE_2D, mTextures[CubeFaceRight]);
    mMeshes[RIGHT_MESH].render(CorrectionMesh::WARP_MESH);

    glBindTexture(GL_TEXTURE_2D, mTextures[CubeFaceTop]);
    mMeshes[TOP_MESH].render(CorrectionMesh::WARP_MESH);

    sgct::ShaderProgram::unbind();

    glDisable(GL_DEPTH_TEST);

    if (alpha)
        glDisable(GL_BLEND);

    //restore depth func
    glDepthFunc(GL_LESS);
}

void sgct_core::SphericalMirrorProjection::renderInternalFixedPipeline()
{
    sgct::Engine::mInstance->enterCurrentViewport();

    sgct::SGCTWindow * winPtr = sgct::Engine::instance()->getCurrentWindowPtr();
    sgct_core::BaseViewport * vpPtr = winPtr->getCurrentViewport();
    
    float aspect = winPtr->getAspectRatio() *
        (vpPtr->getXSize() / vpPtr->getYSize());
    
    glClearColor(mClearColor.r, mClearColor.g, mClearColor.b, mClearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bindShaderProgram();

    glPushAttrib(GL_ALL_ATTRIB_BITS);
    //if for some reson the active texture has been reset
    glActiveTexture(GL_TEXTURE0);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glPushMatrix();
    glOrtho(-static_cast<double>(aspect), static_cast<double>(aspect), -1.0, 1.0, -1.0, 1.0);
    glMatrixMode(GL_MODELVIEW); //restore
    glLoadIdentity();

    glDisable(GL_CULL_FACE);
    bool alpha = winPtr->getAlpha();
    if (alpha)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
        glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);

    glEnable(GL_TEXTURE_2D);
    glUniform1i(mTexLoc, 0);

    glBindTexture(GL_TEXTURE_2D, mTextures[CubeFaceFront]);
    mMeshes[BOTTOM_MESH].render(CorrectionMesh::WARP_MESH);

    glBindTexture(GL_TEXTURE_2D, mTextures[CubeFaceLeft]);
    mMeshes[LEFT_MESH].render(CorrectionMesh::WARP_MESH);

    glBindTexture(GL_TEXTURE_2D, mTextures[CubeFaceRight]);
    mMeshes[RIGHT_MESH].render(CorrectionMesh::WARP_MESH);

    glBindTexture(GL_TEXTURE_2D, mTextures[CubeFaceTop]);
    mMeshes[TOP_MESH].render(CorrectionMesh::WARP_MESH);

    sgct::ShaderProgram::unbind();

    glPopClientAttrib();
    glPopAttrib();

    //exit ortho mode
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

void sgct_core::SphericalMirrorProjection::renderCubemapInternal(std::size_t * subViewPortIndex)
{
    BaseViewport * vp;
    unsigned int faceIndex;
    for (std::size_t i = 0; i < 6; i++)
    {
        vp = &mSubViewports[i];
        *subViewPortIndex = i;
        faceIndex = static_cast<unsigned int>(i);
        TextureIndex ti = static_cast<TextureIndex>(CubeFaceRight + i);

        if (vp->isEnabled())
        {
            //bind & attach buffer
            mCubeMapFBO_Ptr->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
            if (!mCubeMapFBO_Ptr->isMultiSampled())
                attachTextures(ti);

            sgct::Engine::mInstance->getCurrentWindowPtr()->setCurrentViewport(vp);
            drawCubeFace(i);

            //blit MSAA fbo to texture
            if (mCubeMapFBO_Ptr->isMultiSampled())
                blitCubeFace(ti);
        }//end if viewport is enabled
    }//end for
}

void sgct_core::SphericalMirrorProjection::renderCubemapInternalFixedPipeline(std::size_t * subViewPortIndex)
{
    BaseViewport * vp;
    unsigned int faceIndex;
    for (std::size_t i = 0; i < 6; i++)
    {
        vp = &mSubViewports[i];
        *subViewPortIndex = i;
        faceIndex = static_cast<unsigned int>(i);
        TextureIndex ti = static_cast<TextureIndex>(CubeFaceRight + i);

        if (vp->isEnabled())
        {
            //bind & attach buffer
            mCubeMapFBO_Ptr->bind(); //osg seems to unbind FBO when rendering with osg FBO cameras
            if (!mCubeMapFBO_Ptr->isMultiSampled())
                attachTextures(ti);

            sgct::Engine::mInstance->getCurrentWindowPtr()->setCurrentViewport(vp);
            drawCubeFace(i);

            //blit MSAA fbo to texture
            if (mCubeMapFBO_Ptr->isMultiSampled())
                blitCubeFace(ti);
        }//end if viewport is enabled
    }//end for
}