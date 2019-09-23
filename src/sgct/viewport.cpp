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

    [[nodiscard]] sgct::config::MpcdiProjection parseMpcdi(tinyxml2::XMLElement* element) {
        using namespace tinyxml2;

        sgct::config::MpcdiProjection proj;
        if (element->Attribute("id")) {
            proj.id = element->Attribute("id");
        }

        glm::vec2 vpPosition;
        if (element->QueryFloatAttribute("x", &vpPosition[0]) == XML_NO_ERROR &&
            element->QueryFloatAttribute("y", &vpPosition[1]) == XML_NO_ERROR)
        {
            proj.position = vpPosition;
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Viewport: Failed to parse position from XML\n"
            );
        }

        glm::vec2 vpSize;
        if (element->QueryFloatAttribute("xSize", &vpSize[0]) == XML_NO_ERROR &&
            element->QueryFloatAttribute("ySize", &vpSize[1]) == XML_NO_ERROR)
        {
            proj.size = vpSize;
        }
        else {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "Viewport: Failed to parse size from XML\n"
            );
        }

        //glm::vec2 vpResolution;
        //if (element->QueryFloatAttribute("xResolution", &vpResolution[0]) == XML_NO_ERROR &&
        //    element->QueryFloatAttribute("yResolution", &vpResolution[1]) == XML_NO_ERROR)
        //{
        //    float expectedResolutionX = std::floor(vpResolution.x * winResX);
        //    float expectedResolutionY = std::floor(vpResolution.y * winResY);

        //    if (expectedResolutionX != vpResolution.x ||
        //        expectedResolutionY != vpResolution.y)
        //    {
        //        MessageHandler::instance()->print(
        //            MessageHandler::Level::Warning,
        //            "Viewport: MPCDI region expected resolution does not match window\n"
        //        );
        //    }

        //    // @TODO:  Do something with the resolution
        //}
        //else {
        //    MessageHandler::instance()->print(
        //        MessageHandler::Level::Error,
        //        "Viewport: Failed to parse resolution from XML\n"
        //    );
        //}

        float yaw = 0.f;
        float pitch = 0.f;
        float roll = 0.f;
        tinyxml2::XMLElement* child = element->FirstChildElement();
        while (child) {
            std::string_view val = child->Value();
            if (val == "frustum") {
                bool hasRight = false;
                bool hasLeft = false;
                bool hasUp = false;
                bool hasDown = false;
                bool hasYaw = false;
                bool hasPitch = false;
                bool hasRoll = false;
                sgct::config::MpcdiProjection::Frustum frustum;
                tinyxml2::XMLElement* grandChild = child->FirstChildElement();
                while (grandChild) {
                    try {
                        std::string_view grandChildVal = grandChild->Value();
                        if (grandChildVal == "rightAngle") {
                            frustum.right = std::stof(grandChild->GetText());
                            hasRight = true;
                        }
                        else if (grandChildVal == "leftAngle") {
                            frustum.left = std::stof(grandChild->GetText());
                            hasLeft = true;
                        }
                        else if (grandChildVal == "upAngle") {
                            frustum.up = std::stof(grandChild->GetText());
                            hasUp = true;
                        }
                        else if (grandChildVal == "downAngle") {
                            frustum.down = std::stof(grandChild->GetText());
                            hasDown = true;
                        }
                        else if (grandChildVal == "yaw") {
                            yaw = std::stof(grandChild->GetText());
                            hasYaw = true;
                        }
                        else if (grandChildVal == "pitch") {
                            pitch = std::stof(grandChild->GetText());
                            hasPitch = true;
                        }
                        else if (grandChildVal == "roll") {
                            roll = std::stof(grandChild->GetText());
                            hasRoll = true;
                        }
                    }
                    catch (const std::invalid_argument&) {
                        sgct::MessageHandler::instance()->print(
                            sgct::MessageHandler::Level::Error,
                            "Viewport: Failed to parse frustum element from MPCDI XML\n"
                        );
                    }

                    grandChild = grandChild->NextSiblingElement();
                }


                const bool hasMissingField = !hasDown || !hasUp || !hasLeft || !hasRight ||
                    !hasYaw || !hasPitch || !hasRoll;

                if (hasMissingField) {
                    sgct::MessageHandler::instance()->print(
                        sgct::MessageHandler::Level::Error,
                        "Viewport: Failed to parse mpcdi projection FOV from XML\n"
                    );
                    return {};
                }

                proj.orientation = sgct::core::readconfig::parseMpcdiOrientationNode(yaw, pitch, roll);
            }
            child = child->NextSiblingElement();
        }

        return proj;
    }

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

void Viewport::applySettings(const sgct::config::Viewport& viewport) {
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
        Frustum::Mode e = [](sgct::config::Viewport::Eye e) {
            switch (e) {
                default:
                case sgct::config::Viewport::Eye::Mono:
                    return Frustum::MonoEye;
                case sgct::config::Viewport::Eye::StereoLeft:
                    return Frustum::StereoLeftEye;
                case sgct::config::Viewport::Eye::StereoRight:
                    return Frustum::StereoRightEye;
            }
        }(*viewport.eye);
        setEye(e);
    }

    setPos(viewport.position);
    setSize(viewport.size);

    std::visit(overloaded{
        [](const sgct::config::NoProjection&) {},
        [this](const sgct::config::PlanarProjection& proj) {
            applyPlanarProjection(proj);
        },
        [this](const sgct::config::FisheyeProjection& proj) {
            applyFisheyeProjection(proj);
        },
        [this](const sgct::config::SphericalMirrorProjection& proj) {
            applySphericalMirrorProjection(proj);
        },
        [this](const sgct::config::SpoutOutputProjection& proj) {
            applySpoutOutputProjection(proj);
        },
        [this](const sgct::config::ProjectionPlane& proj) {
            mProjectionPlane.setCoordinateLowerLeft(*proj.lowerLeft);
            mUnTransformedViewPlaneCoords.lowerLeft = *proj.lowerLeft;
            mProjectionPlane.setCoordinateUpperLeft(*proj.upperLeft);
            mUnTransformedViewPlaneCoords.upperLeft = *proj.upperLeft;
            mProjectionPlane.setCoordinateUpperRight(*proj.upperRight);
            mUnTransformedViewPlaneCoords.upperRight = *proj.upperRight;
        },
    }, viewport.projection);
}

void Viewport::configureMpcdi(tinyxml2::XMLElement* element) {
    sgct::config::MpcdiProjection proj = parseMpcdi(element);

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

void Viewport::applyPlanarProjection(const sgct::config::PlanarProjection& proj) {
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

void Viewport::applyFisheyeProjection(const sgct::config::FisheyeProjection& proj) {
    std::unique_ptr<FisheyeProjection> fishProj = std::make_unique<FisheyeProjection>();
    fishProj->setUser(mUser);
    
    if (proj.fov) {
        fishProj->setFOV(*proj.fov);
    }
    if (proj.quality) {
        fishProj->setCubemapResolution(*proj.quality);
    }
    if (proj.method) {
        sgct::core::FisheyeProjection::FisheyeMethod m = [](sgct::config::FisheyeProjection::Method m) {
            switch (m) {
                default:
                case sgct::config::FisheyeProjection::Method::FourFace:
                    return sgct::core::FisheyeProjection::FisheyeMethod::FourFaceCube;
                case sgct::config::FisheyeProjection::Method::FiveFace:
                    return sgct::core::FisheyeProjection::FisheyeMethod::FiveFaceCube;
            }
        }(*proj.method);
        fishProj->setRenderingMethod(m);
    }
    if (proj.interpolation) {
        sgct::core::NonLinearProjection::InterpolationMode i =
            [](sgct::config::FisheyeProjection::Interpolation i) {
                switch (i) {
                    default:
                    case sgct::config::FisheyeProjection::Interpolation::Linear:
                        return sgct::core::NonLinearProjection::InterpolationMode::Linear;
                    case sgct::config::FisheyeProjection::Interpolation::Cubic:
                        return sgct::core::NonLinearProjection::InterpolationMode::Cubic;
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

void Viewport::applySpoutOutputProjection(const sgct::config::SpoutOutputProjection& proj) {
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
        SpoutOutputProjection::Mapping m = [](sgct::config::SpoutOutputProjection::Mapping m) {
            switch (m) {
                default:
                case sgct::config::SpoutOutputProjection::Mapping::Fisheye:
                    return SpoutOutputProjection::Mapping::Fisheye;
                case sgct::config::SpoutOutputProjection::Mapping::Equirectangular:
                    return SpoutOutputProjection::Mapping::Equirectangular;
                case sgct::config::SpoutOutputProjection::Mapping::Cubemap:
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

        bool right = true;
        bool zLeft = true;
        bool bottom = true;
        bool top = true;
        bool left = true;
        bool zRight = true;

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

void Viewport::applySphericalMirrorProjection(const sgct::config::SphericalMirrorProjection& proj) {
    std::unique_ptr<SphericalMirrorProjection> sphericalMirrorProj =
        std::make_unique<SphericalMirrorProjection>();

    sphericalMirrorProj->setUser(mUser);
    sphericalMirrorProj->setCubemapResolution(*proj.quality);

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
