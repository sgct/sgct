/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/Viewport.h>

#include <sgct/ClusterManager.h>
#include <sgct/config.h>
#include <sgct/FisheyeProjection.h>
#include <sgct/MessageHandler.h>
#include <sgct/ReadConfig.h>
#include <sgct/ScreenCapture.h>
#include <sgct/SphericalMirrorProjection.h>
#include <sgct/SpoutOutputProjection.h>
#include <sgct/TextureManager.h>
#include <algorithm>
#include <array>
#include <optional>
#include <variant>

namespace {
    // Helper structs for the visitor pattern of the std::variant on projections
    template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
    template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;


} // namespace

namespace sgct::core {

Viewport::Viewport() : Viewport(0.f, 0.f, 0.f, 0.f) {}

Viewport::Viewport(float x, float y, float xSize, float ySize) {
    mPosition = glm::vec2(x, y);
    mSize = glm::vec2(xSize, ySize);
    mProjectionPlane.reset();
}

Viewport::~Viewport() {
    glDeleteTextures(1, &mOverlayTextureIndex);
    glDeleteTextures(1, &mBlendMaskTextureIndex);
    glDeleteTextures(1, &mBlackLevelMaskTextureIndex);
}

void Viewport::applySettings(const config::Viewport& viewport) {
    if (viewport.user) {
        setUserName(*viewport.user);
    }
    if (viewport.name) {
        setName(*viewport.name);
    }
    if (viewport.overlayTexture) {
        setOverlayTexture(*viewport.overlayTexture);
    }
    if (viewport.blendMaskTexture) {
        setBlendMaskTexture(*viewport.blendMaskTexture);
    }
    if (viewport.blendLevelMaskTexture) {
        setBlackLevelMaskTexture(*viewport.blendLevelMaskTexture);
    }
    if (viewport.correctionMeshTexture) {
        setCorrectionMesh(*viewport.correctionMeshTexture);
    }
    if (viewport.meshHint) {
        mMeshHint = *viewport.meshHint;
    }
    if (viewport.isTracked) {
        setTracked(*viewport.isTracked);
    }
    if (viewport.eye) {
        Frustum::Mode e = [](config::Viewport::Eye e) {
            switch (e) {
                default:
                case config::Viewport::Eye::Mono:
                    return Frustum::MonoEye;
                case config::Viewport::Eye::StereoLeft:
                    return Frustum::StereoLeftEye;
                case config::Viewport::Eye::StereoRight:
                    return Frustum::StereoRightEye;
            }
        }(*viewport.eye);
        setEye(e);
    }

    setPos(viewport.position);
    setSize(viewport.size);

    std::visit(overloaded {
        [](const config::NoProjection&) {},
        [this](const config::PlanarProjection& proj) {
            applyPlanarProjection(proj);
        },
        [this](const config::FisheyeProjection& proj) {
            applyFisheyeProjection(proj);
        },
        [this](const config::SphericalMirrorProjection& proj) {
            applySphericalMirrorProjection(proj);
        },
        [this](const config::SpoutOutputProjection& proj) {
            applySpoutOutputProjection(proj);
        },
        [this](const config::ProjectionPlane& proj) {
            mProjectionPlane.setCoordinateLowerLeft(*proj.lowerLeft);
            mUnTransformedViewPlaneCoords.lowerLeft = *proj.lowerLeft;
            mProjectionPlane.setCoordinateUpperLeft(*proj.upperLeft);
            mUnTransformedViewPlaneCoords.upperLeft = *proj.upperLeft;
            mProjectionPlane.setCoordinateUpperRight(*proj.upperRight);
            mUnTransformedViewPlaneCoords.upperRight = *proj.upperRight;
        },
    }, viewport.projection);
}

void Viewport::applySettings(const sgct::config::MpcdiProjection& proj) {
    if (proj.id) {
        setName(*proj.id);
    }
    if (proj.position) {
        setPos(*proj.position);
    }
    if (proj.size) {
        setSize(*proj.size);
    }
    if (proj.frustum) {
        setViewPlaneCoordsUsingFOVs(
            proj.frustum->up,
            proj.frustum->down,
            proj.frustum->left,
            proj.frustum->right,
            *proj.orientation,
            *proj.distance
        );
        mProjectionPlane.offset(*proj.offset);
    }
}

void Viewport::applyPlanarProjection(const config::PlanarProjection& proj) {
    setViewPlaneCoordsUsingFOVs(
        proj.fov.up,
        proj.fov.down,
        proj.fov.left,
        proj.fov.right,
        proj.orientation.value_or(glm::quat()),
        proj.fov.distance.value_or(10.f)
    );
    if (proj.offset) {
        mProjectionPlane.offset(*proj.offset);
    }
}

void Viewport::applyFisheyeProjection(const config::FisheyeProjection& proj) {
    std::unique_ptr<FisheyeProjection> fishProj = std::make_unique<FisheyeProjection>();
    fishProj->setUser(mUser);
    
    if (proj.fov) {
        fishProj->setFOV(*proj.fov);
    }
    if (proj.quality) {
        fishProj->setCubemapResolution(*proj.quality);
    }
    if (proj.method) {
        core::FisheyeProjection::FisheyeMethod m = [](config::FisheyeProjection::Method m)
        {
            switch (m) {
                default:
                case config::FisheyeProjection::Method::FourFace:
                    return core::FisheyeProjection::FisheyeMethod::FourFaceCube;
                case config::FisheyeProjection::Method::FiveFace:
                    return core::FisheyeProjection::FisheyeMethod::FiveFaceCube;
            }
        }(*proj.method);
        fishProj->setRenderingMethod(m);
    }
    if (proj.interpolation) {
        core::NonLinearProjection::InterpolationMode i =
            [](config::FisheyeProjection::Interpolation i) {
                switch (i) {
                    default:
                    case config::FisheyeProjection::Interpolation::Linear:
                        return core::NonLinearProjection::InterpolationMode::Linear;
                    case config::FisheyeProjection::Interpolation::Cubic:
                        return core::NonLinearProjection::InterpolationMode::Cubic;
                }
            }(*proj.interpolation);
        fishProj->setInterpolationMode(i);
    }
    if (proj.tilt) {
        fishProj->setTilt(*proj.tilt);
    }
    if (proj.diameter) {
        fishProj->setDomeDiameter(*proj.diameter);
    }
    if (proj.crop) {
        fishProj->setCropFactors(
            proj.crop->left,
            proj.crop->right,
            proj.crop->bottom,
            proj.crop->top
        );
    }
    if (proj.offset) {
        fishProj->setBaseOffset(*proj.offset);
    }
    if (proj.background) {
        fishProj->setClearColor(*proj.background);
    }
    fishProj->setUseDepthTransformation(true);
    mNonLinearProjection = std::move(fishProj);
}

void Viewport::applySpoutOutputProjection(const config::SpoutOutputProjection& proj) {
#ifndef SGCT_HAS_SPOUT
    (void)proj;
    MessageHandler::instance()->print(
        MessageHandler::Level::Warning,
        "ReadConfig: Spout library not added to SGCT\n"
    );
    return;
#else
   
    std::unique_ptr<SpoutOutputProjection> spoutProj =
        std::make_unique<SpoutOutputProjection>();
    spoutProj->setUser(mUser);
    if (proj.quality) {
        spoutProj->setCubemapResolution(*proj.quality);
    }
    if (proj.mapping) {
        SpoutOutputProjection::Mapping m = [](config::SpoutOutputProjection::Mapping m) {
            switch (m) {
                default:
                case config::SpoutOutputProjection::Mapping::Fisheye:
                    return SpoutOutputProjection::Mapping::Fisheye;
                case config::SpoutOutputProjection::Mapping::Equirectangular:
                    return SpoutOutputProjection::Mapping::Equirectangular;
                case config::SpoutOutputProjection::Mapping::Cubemap:
                    return SpoutOutputProjection::Mapping::Cubemap;
            }
        }(*proj.mapping);
        spoutProj->setSpoutMapping(m);
    }
    spoutProj->setSpoutMappingName(proj.mappingSpoutName);
    if (proj.background) {
        spoutProj->setClearColor(*proj.background);
    }
    if (proj.channels) {
        bool channels[6] = {
            proj.channels->right,
            proj.channels->zLeft,
            proj.channels->bottom,
            proj.channels->top,
            proj.channels->left,
            proj.channels->zRight
        };
        spoutProj->setSpoutChannels(channels);
    }
    if (proj.orientation) {
        spoutProj->setSpoutRigOrientation(*proj.orientation);
    }

    spoutProj->setUseDepthTransformation(true);
    mNonLinearProjection = std::move(spoutProj);
#endif
}

void Viewport::applySphericalMirrorProjection(
                                            const config::SphericalMirrorProjection& proj)
{
    std::unique_ptr<SphericalMirrorProjection> sphericalMirrorProj =
        std::make_unique<SphericalMirrorProjection>();

    sphericalMirrorProj->setUser(mUser);
    if (proj.quality) {
        sphericalMirrorProj->setCubemapResolution(*proj.quality);
    }

    if (proj.tilt) {
        sphericalMirrorProj->setTilt(*proj.tilt);
    }
    if (proj.background) {
        sphericalMirrorProj->setClearColor(*proj.background);
    }

    sphericalMirrorProj->setMeshPaths(
        proj.mesh.bottom,
        proj.mesh.left,
        proj.mesh.right,
        proj.mesh.top
    );
    sphericalMirrorProj->setUseDepthTransformation(false);
    mNonLinearProjection = std::move(sphericalMirrorProj);
}

void Viewport::setOverlayTexture(std::string texturePath) {
    mOverlayFilename = std::move(texturePath);
}

void Viewport::setBlendMaskTexture(std::string texturePath) {
    mBlendMaskFilename = std::move(texturePath);
}

void Viewport::setBlackLevelMaskTexture(std::string texturePath) {
    mBlackLevelMaskFilename = std::move(texturePath);
}

void Viewport::setCorrectionMesh(std::string meshPath) {
    mMeshFilename = std::move(meshPath);
}

void Viewport::setMpcdiWarpMesh(std::vector<unsigned char> data) {
    mMpcdiWarpMesh = std::move(data);
}

void Viewport::setTracked(bool state) {
    mTracked = state;
}

void Viewport::loadData() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "Viewport: loading GPU data for '%s'\n", mName.c_str()
    );
        
    if (!mOverlayFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            mOverlayTextureIndex,
            mOverlayFilename,
            true,
            1
        );
    }

    if (!mBlendMaskFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            mBlendMaskTextureIndex,
            mBlendMaskFilename,
            true,
            1
        );
    }

    if (!mBlackLevelMaskFilename.empty()) {
        TextureManager::instance()->loadUnManagedTexture(
            mBlackLevelMaskTextureIndex,
            mBlackLevelMaskFilename,
            true,
            1
        );
    }

    if (!mMpcdiWarpMesh.empty()) {
        mCorrectionMesh = mCM.readAndGenerateMesh(
            "mesh.mpcdi",
            *this,
            CorrectionMesh::parseHint("mpcdi")
        );
    }
    else {
        // load default if mMeshFilename is empty
        mCorrectionMesh = mCM.readAndGenerateMesh(
            mMeshFilename,
            *this,
            CorrectionMesh::parseHint(mMeshHint)
        );
    }
}

void Viewport::renderQuadMesh() const {
    if (mEnabled) {
        mCM.renderQuadMesh();
    }
}

void Viewport::renderWarpMesh() const {
    if (mEnabled) {
        mCM.renderWarpMesh();
    }
}

void Viewport::renderMaskMesh() const {
    if (mEnabled) {
        mCM.renderMaskMesh();
    }
}

bool Viewport::hasOverlayTexture() const {
    return mOverlayTextureIndex != 0;
}

bool Viewport::hasBlendMaskTexture() const {
    return mBlendMaskTextureIndex != 0;
}

bool Viewport::hasBlackLevelMaskTexture() const {
    return mBlackLevelMaskTextureIndex != 0;
}

bool Viewport::hasSubViewports() const {
    return mNonLinearProjection != nullptr;
}

bool Viewport::hasCorrectionMesh() const {
    return mCorrectionMesh;
}

bool Viewport::isTracked() const {
    return mTracked;
}

unsigned int Viewport::getOverlayTextureIndex() const {
    return mOverlayTextureIndex;
}

unsigned int Viewport::getBlendMaskTextureIndex() const {
    return mBlendMaskTextureIndex;
}

unsigned int Viewport::getBlackLevelMaskTextureIndex() const {
    return mBlackLevelMaskTextureIndex;
}

CorrectionMesh& Viewport::getCorrectionMeshPtr() {
    return mCM;
}

NonLinearProjection* Viewport::getNonLinearProjection() const {
    return mNonLinearProjection.get();
}

const std::vector<unsigned char>& Viewport::mpcdiWarpMesh() const {
    return mMpcdiWarpMesh;
}

} // namespace sgct::core
