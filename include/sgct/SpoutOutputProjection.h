/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SPOUTOUTPUT_PROJECTION_H
#define _SPOUTOUTPUT_PROJECTION_H

#include "NonLinearProjection.h"
#include <glm/glm.hpp>

namespace sgct_core
{
    /*!
    This class manages and renders non linear fisheye projections
    */
    class SpoutOutputProjection : public NonLinearProjection
    {
    public:
        SpoutOutputProjection();
        ~SpoutOutputProjection();

        enum Mapping {
            Fisheye,
            Equirectangular,
            Cubemap
        };

        void setSpoutChannels(bool channels[6]);
        void setSpoutMappingName(std::string name);
        void setSpoutMapping(Mapping type);
        void setSpoutRigOrientation(glm::vec3 orientation);
        void update(float width, float height);
        void render();
        void renderCubemap(std::size_t * subViewPortIndex);

        static const size_t spoutTotalFaces = 6;
        static const std::string spoutCubeMapFaceName[spoutTotalFaces];

    private:
        void initTextures();
        void initViewports();
        void initShaders();
        void initFBO();
        void updateGeomerty(const float & width, const float & height);

        void drawCubeFace(const std::size_t & face);
        void blitCubeFace(const int & face);
        void attachTextures(const int & face);
        void renderInternal();
        void renderInternalFixedPipeline();
        void renderCubemapInternal(std::size_t * subViewPortIndex);
        void renderCubemapInternalFixedPipeline(std::size_t * subViewPortIndex);

        void(SpoutOutputProjection::*mInternalRenderFn)(void);
        void(SpoutOutputProjection::*mInternalRenderCubemapFn)(std::size_t *);

        //shader locations
        int mCubemapLoc, mHalfFovLoc, mSwapColorLoc, mSwapDepthLoc, mSwapNearLoc, mSwapFarLoc;

        OffScreenBuffer * mSpoutFBO_Ptr;

        void *handle[spoutTotalFaces];
        GLuint spoutTexture[spoutTotalFaces];
        bool spoutEnabled[spoutTotalFaces];

        void *spoutMappingHandle;
        GLuint spoutMappingTexture;
        Mapping spoutMappingType;
        std::string spoutMappingName;
        glm::vec3 spoutRigOrientation = glm::vec3(0.f);

        int spoutMappingWidth;
        int spoutMappingHeight;
    };

}

#endif
