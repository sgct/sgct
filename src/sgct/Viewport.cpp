/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include <sgct/ogl_headers.h>
#include <sgct/Viewport.h>
#include <sgct/TextureManager.h>
#include <sgct/ClusterManager.h>
#include <sgct/MessageHandler.h>
#include <sgct/ReadConfig.h>
#include <sgct/FisheyeProjection.h>
#include <sgct/SphericalMirrorProjection.h>
//#include <glm/gtc/matrix_transform.hpp>
#include "unzip.h"
#include <zip.h>

sgct_core::Viewport::Viewport()
{
    mNonLinearProjection = NULL;
    reset(0.0f, 0.0f, 1.0f, 1.0f);
}

/*!
    Create a viewport coordinates are relative to the window size [0, 1]
*/
sgct_core::Viewport::Viewport(float x, float y, float xSize, float ySize)
{
    mNonLinearProjection = NULL;
    reset(x, y, xSize, ySize);
}

/*!
Destructor that deletes any overlay or mask textures
*/
sgct_core::Viewport::~Viewport()
{
    if (mNonLinearProjection)
    {
        delete mNonLinearProjection;
        mNonLinearProjection = NULL;
    }
    
    if (mOverlayTextureIndex)
        glDeleteTextures(1, &mOverlayTextureIndex);

    if (mBlendMaskTextureIndex)
        glDeleteTextures(1, &mBlendMaskTextureIndex);

    if (mBlackLevelMaskTextureIndex)
        glDeleteTextures(1, &mBlackLevelMaskTextureIndex);

    if (mMpcdiWarpMeshData)
        delete mMpcdiWarpMeshData;
}

void sgct_core::Viewport::configure(tinyxml2::XMLElement * element)
{
    if (element->Attribute("user") != NULL)
        setUserName(element->Attribute("user"));

    if (element->Attribute("name") != NULL)
        setName(element->Attribute("name"));

    if (element->Attribute("overlay") != NULL)
        setOverlayTexture(element->Attribute("overlay"));

    //for backward compability
    if (element->Attribute("mask") != NULL)
        setBlendMaskTexture(element->Attribute("mask"));

    if (element->Attribute("BlendMask") != NULL)
        setBlendMaskTexture(element->Attribute("BlendMask"));

    if (element->Attribute("BlackLevelMask") != NULL)
        setBlackLevelMaskTexture(element->Attribute("BlackLevelMask"));

    if (element->Attribute("mesh") != NULL)
        setCorrectionMesh(element->Attribute("mesh"));

    if (element->Attribute("hint") != NULL)
        mMeshHint.assign(element->Attribute("hint"));

    if (element->Attribute("tracked") != NULL)
        setTracked(strcmp(element->Attribute("tracked"), "true") == 0 ? true : false);

    //get eye if set
    if (element->Attribute("eye") != NULL)
    {
        if (strcmp("center", element->Attribute("eye")) == 0)
        {
            setEye(Frustum::MonoEye);
        }
        else if (strcmp("left", element->Attribute("eye")) == 0)
        {
            setEye(Frustum::StereoLeftEye);
        }
        else if (strcmp("right", element->Attribute("eye")) == 0)
        {
            setEye(Frustum::StereoRightEye);
        }
    }

    const char * val;
    tinyxml2::XMLElement * subElement = element->FirstChildElement();
    while (subElement != NULL)
    {
        val = subElement->Value();
        float fTmp[2];
        fTmp[0] = 0.0f;
        fTmp[1] = 0.0f;

        if (strcmp("Pos", val) == 0)
        {
            if (subElement->QueryFloatAttribute("x", &fTmp[0]) == tinyxml2::XML_NO_ERROR &&
                subElement->QueryFloatAttribute("y", &fTmp[1]) == tinyxml2::XML_NO_ERROR)
                setPos(fTmp[0], fTmp[1]);
            else
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Viewport: Failed to parse position from XML!\n");
        }
        else if (strcmp("Size", val) == 0)
        {
            if (subElement->QueryFloatAttribute("x", &fTmp[0]) == tinyxml2::XML_NO_ERROR &&
                subElement->QueryFloatAttribute("y", &fTmp[1]) == tinyxml2::XML_NO_ERROR)
                setSize(fTmp[0], fTmp[1]);
            else
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Viewport: Failed to parse size from XML!\n");
        }
        else if (strcmp("PlanarProjection", val) == 0)
        {
            parsePlanarProjection(subElement);
        }//end if planar projection
        else if (strcmp("FisheyeProjection", val) == 0)
        {
            parseFisheyeProjection(subElement);
        }
        else if (strcmp("SphericalMirrorProjection", val) == 0)
        {
            parseSphericalMirrorProjection(subElement);
        }
        else if (strcmp("Viewplane", val) == 0 || strcmp("Projectionplane", val) == 0)
        {
            mProjectionPlane.configure(subElement);
        }

        //iterate
        subElement = subElement->NextSiblingElement();
    }
}

void sgct_core::Viewport::parseFloatFromAttribute(tinyxml2::XMLElement* element,
                                                  const std::string tag,
                                                  float& target)
{
    if (element->Attribute(tag.c_str()) != NULL)
    {
        try {
            target = std::stof(element->Attribute(tag.c_str()));
        }
        catch (const std::invalid_argument& ia) {
            std::string fullErrorMessage = "Viewport: Failed to parse " + tag
                + " from MPCDI XML!\n";
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                fullErrorMessage.c_str());
        }
    }
    else
    {
        std::string fullErrorMessage = "Viewport: No " + tag
            + " provided in MPCDI XML!\n";
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
            fullErrorMessage.c_str());
    }
}

bool sgct_core::Viewport::parseFrustumElement(frustumData& frustum, frustumData::elemIdx elemIndex,
                                              tinyxml2::XMLElement* elem, const char* frustumTag)
{
    if (strcmp(frustumTag, elem->Value()) == 0)
    {
        try {
            frustum.value[elemIndex] = std::stof(elem->GetText());
            frustum.foundElem[elemIndex] = true;
            return true;
        }
        catch (const std::invalid_argument& ia) {
            std::string fullErrorMessage = "Viewport: Failed to parse frustum element "
                + std::string(frustumTag) + " from MPCDI XML!\n";
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                fullErrorMessage.c_str());
        }
    }
    return false;
}

void sgct_core::Viewport::configureMpcdi(tinyxml2::XMLElement* element[],
                                         const char* val[], int winResX, int winResY)
{
    const int idx_x = 0, idx_y = 1;
    float vpPosition[2] = {0.0, 0.0};
    float vpSize[2] = {0.0, 0.0};
    float vpResolution[2] = {0.0, 0.0};
    float expectedResolution[2];
    frustumData frustumElements;

    if (element[2]->Attribute("id") != NULL)
        setName(element[2]->Attribute("id"));

    parseFloatFromAttribute(element[2], "x", vpPosition[idx_x]);
    parseFloatFromAttribute(element[2], "y", vpPosition[idx_y]);
    setPos(vpPosition[idx_x], vpPosition[idx_y]);
    parseFloatFromAttribute(element[2], "xSize", vpSize[idx_x]);
    parseFloatFromAttribute(element[2], "ySize", vpSize[idx_y]);
    setSize(vpSize[idx_x], vpSize[idx_y]);
    parseFloatFromAttribute(element[2], "xResolution", vpResolution[idx_x]);
    parseFloatFromAttribute(element[2], "yResolution", vpResolution[idx_y]);
    expectedResolution[idx_x] = int(vpSize[idx_x] * (float)winResX);
    expectedResolution[idx_y] = int(vpSize[idx_y] * (float)winResY);

    if(   expectedResolution[idx_x] != vpResolution[idx_x]
       || expectedResolution[idx_y] != vpResolution[idx_y] )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING,
            "Viewport: MPCDI region expected resolution does not match portion of window.\n");
    }

    element[3] = element[2]->FirstChildElement();
    while( element[3] != NULL )
    {
        val[3] = element[3]->Value();
        if( strcmp("frustum", val[3]) == 0 )
        {
            float distance = 10.0f;
            glm::quat rotQuat;
            glm::vec3 offset(0.0f, 0.0f, 0.0f);

            element[4] = element[3]->FirstChildElement();
            while( element[4] != NULL )
            {
                //val[4] = element[4]->Value();
                if (parseFrustumElement(frustumElements, frustumData::elemIdx::right, element[4], "rightAngle"))  ;
                else if (parseFrustumElement(frustumElements, frustumData::elemIdx::left, element[4], "leftAngle"))  ;
                else if (parseFrustumElement(frustumElements, frustumData::elemIdx::up, element[4], "upAngle"))  ;
                else if (parseFrustumElement(frustumElements, frustumData::elemIdx::down, element[4], "downAngle"))  ;
                else if (parseFrustumElement(frustumElements, frustumData::elemIdx::yaw, element[4], "yaw"))  ;
                else if (parseFrustumElement(frustumElements, frustumData::elemIdx::pitch, element[4], "pitch"))  ;
                else if (parseFrustumElement(frustumElements, frustumData::elemIdx::roll, element[4], "roll"))  ;
                element[4] = element[4]->NextSiblingElement();
            }

            for (bool& hasFoundSpecificField : frustumElements.foundElem)
            {
                if (!hasFoundSpecificField)
                {
                    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                        "Viewport: Failed to parse mpcdi projection FOV from XML!\n");
                    return;
                }
            }

            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                "Viewport: Adding mpcdi FOV d=%f l=%f r=%f u=%f y=%f p=%f r=%f\n",
                frustumElements.value[frustumData::elemIdx::down],
                frustumElements.value[frustumData::elemIdx::left],
                frustumElements.value[frustumData::elemIdx::right],
                frustumElements.value[frustumData::elemIdx::up],
                frustumElements.value[frustumData::elemIdx::yaw],
                frustumElements.value[frustumData::elemIdx::pitch],
                frustumElements.value[frustumData::elemIdx::roll]);
            rotQuat = ReadConfig::parseMpcdiOrientationNode(frustumElements.value[frustumData::elemIdx::yaw],
                frustumElements.value[frustumData::elemIdx::pitch],
                frustumElements.value[frustumData::elemIdx::roll]);
            setViewPlaneCoordsUsingFOVs(frustumElements.value[frustumData::elemIdx::up],
                frustumElements.value[frustumData::elemIdx::down],
                frustumElements.value[frustumData::elemIdx::left],
                frustumElements.value[frustumData::elemIdx::right],
                rotQuat,
                distance);
            mProjectionPlane.offset(offset);
        }
        element[3] = element[3]->NextSiblingElement();
    }
}

void sgct_core::Viewport::reset(float x, float y, float xSize, float ySize)
{    
    mX = x;
    mY = y;
    mXSize = xSize;
    mYSize = ySize;
    mEye = Frustum::MonoEye;
    mCorrectionMesh = false;
    mOverlayTextureIndex = GL_FALSE;
    mBlendMaskTextureIndex = GL_FALSE;
    mBlackLevelMaskTextureIndex = GL_FALSE;
    mTracked = false;
    mEnabled = true;
    mName.assign("NoName");
    mUser = ClusterManager::instance()->getDefaultUserPtr();
    mProjectionPlane.reset();
}

void sgct_core::Viewport::parsePlanarProjection(tinyxml2::XMLElement * element)
{
    const char * val;
    
    bool validFOV = false;
    float down, left, right, up;
    float distance = 10.0f;
    glm::quat rotQuat;
    glm::vec3 offset(0.0f, 0.0f, 0.0f);

    tinyxml2::XMLElement * subElement = element->FirstChildElement();
    while (subElement != NULL)
    {
        val = subElement->Value();

        if (strcmp("FOV", val) == 0)
        {
            if (subElement->QueryFloatAttribute("down", &down) == tinyxml2::XML_NO_ERROR &&
                subElement->QueryFloatAttribute("left", &left) == tinyxml2::XML_NO_ERROR &&
                subElement->QueryFloatAttribute("right", &right) == tinyxml2::XML_NO_ERROR &&
                subElement->QueryFloatAttribute("up", &up) == tinyxml2::XML_NO_ERROR)
            {
                validFOV = true;


                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG,
                    "Viewport: Adding planar projection FOV down=%f left=%f right=%f up=%f\n",
                    down, left, right, up);
            }
            else
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Viewport: Failed to parse planar projection FOV from XML!\n");

            float tmpf;
            if (subElement->QueryFloatAttribute("distance", &tmpf) == tinyxml2::XML_NO_ERROR)
            {
                distance = tmpf;
            }
        }
        else if (strcmp("Orientation", val) == 0)
        {
            rotQuat = ReadConfig::parseOrientationNode(subElement);
        }
        else if (strcmp("Offset", val) == 0)
        {
            float x, y, z;

            if (subElement->QueryFloatAttribute("x", &x) == tinyxml2::XML_NO_ERROR)
                offset.x = x;

            if (subElement->QueryFloatAttribute("y", &y) == tinyxml2::XML_NO_ERROR)
                offset.y = y;

            if (subElement->QueryFloatAttribute("z", &z) == tinyxml2::XML_NO_ERROR)
                offset.z = z;
        }

        //iterate
        subElement = subElement->NextSiblingElement();
    }//end while level 3

    if (validFOV)
    {
        setViewPlaneCoordsUsingFOVs(up, -down, -left, right, rotQuat, distance);
        mProjectionPlane.offset(offset);
    }
}

void sgct_core::Viewport::parseFisheyeProjection(tinyxml2::XMLElement * element)
{
    FisheyeProjection * fishProj = new FisheyeProjection();
    for (std::size_t i = 0; i < 6; i++)
        fishProj->getSubViewportPtr(i)->setUser(mUser);
    
    float fov;
    if (element->QueryFloatAttribute("fov", &fov) == tinyxml2::XML_NO_ERROR)
        fishProj->setFOV(fov);

    if (element->Attribute("quality") != NULL)
    {
        fishProj->setCubemapResolution(std::string(element->Attribute("quality")));
    }

    if (element->Attribute("method") != NULL)
        fishProj->setRenderingMethod( 
            strcmp(element->Attribute("method"), "five_face_cube") == 0 ?
            FisheyeProjection::FiveFaceCube : FisheyeProjection::FourFaceCube);

    if (element->Attribute("interpolation") != NULL)
        fishProj->setInterpolationMode(
            strcmp(element->Attribute("interpolation"), "cubic") == 0 ? NonLinearProjection::Cubic : NonLinearProjection::Linear);

    float tilt;
    if (element->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR)
    {
        fishProj->setTilt(tilt);
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Setting fisheye tilt to %f degrees.\n", tilt);
    }

    float diameter;
    if (element->QueryFloatAttribute("diameter", &diameter) == tinyxml2::XML_NO_ERROR)
    {
        fishProj->setDomeDiameter(diameter);
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Setting fisheye diameter to %f meters.\n", diameter);
    }

    tinyxml2::XMLElement * subElement = element->FirstChildElement();
    const char * val;

    while (subElement != NULL)
    {
        val = subElement->Value();

        if (strcmp("Crop", val) == 0)
        {
            float tmpFArr[] = { 0.0f, 0.0f, 0.0f, 0.0f };
            float ftmp;

            if (subElement->QueryFloatAttribute("left", &ftmp) == tinyxml2::XML_NO_ERROR)
                tmpFArr[FisheyeProjection::CropLeft] = ftmp;
            if (subElement->QueryFloatAttribute("right", &ftmp) == tinyxml2::XML_NO_ERROR)
                tmpFArr[FisheyeProjection::CropRight] = ftmp;
            if (subElement->QueryFloatAttribute("bottom", &ftmp) == tinyxml2::XML_NO_ERROR)
                tmpFArr[FisheyeProjection::CropBottom] = ftmp;
            if (subElement->QueryFloatAttribute("top", &ftmp) == tinyxml2::XML_NO_ERROR)
                tmpFArr[FisheyeProjection::CropTop] = ftmp;

            fishProj->setCropFactors(
                tmpFArr[FisheyeProjection::CropLeft],
                tmpFArr[FisheyeProjection::CropRight],
                tmpFArr[FisheyeProjection::CropBottom],
                tmpFArr[FisheyeProjection::CropTop]);
        }
        else if (strcmp("Offset", val) == 0)
        {
            glm::vec3 offset = glm::vec3(0.0f);
            float ftmp;

            if (subElement->QueryFloatAttribute("x", &ftmp) == tinyxml2::XML_NO_ERROR)
                offset.x = ftmp;
            if (subElement->QueryFloatAttribute("y", &ftmp) == tinyxml2::XML_NO_ERROR)
                offset.y = ftmp;
            if (subElement->QueryFloatAttribute("z", &ftmp) == tinyxml2::XML_NO_ERROR)
                offset.z = ftmp;

            fishProj->setBaseOffset(offset);
        }
        if (strcmp("Background", val) == 0)
        {
            glm::vec4 color;
            float ftmp;

            if (subElement->QueryFloatAttribute("r", &ftmp) == tinyxml2::XML_NO_ERROR)
                color.r = ftmp;
            if (subElement->QueryFloatAttribute("g", &ftmp) == tinyxml2::XML_NO_ERROR)
                color.g = ftmp;
            if (subElement->QueryFloatAttribute("b", &ftmp) == tinyxml2::XML_NO_ERROR)
                color.b = ftmp;
            if (subElement->QueryFloatAttribute("a", &ftmp) == tinyxml2::XML_NO_ERROR)
                color.a = ftmp;

            fishProj->setClearColor(color);
        }

        //iterate
        subElement = subElement->NextSiblingElement();
    }

    fishProj->setUseDepthTransformation(true);
    mNonLinearProjection = fishProj;
}

void sgct_core::Viewport::parseSphericalMirrorProjection(tinyxml2::XMLElement * element)
{
    SphericalMirrorProjection * sphericalMirrorProj = new SphericalMirrorProjection();
    for (std::size_t i = 0; i < 6; i++)
        sphericalMirrorProj->getSubViewportPtr(i)->setUser(mUser);

    if (element->Attribute("quality") != NULL)
    {
        sphericalMirrorProj->setCubemapResolution(std::string(element->Attribute("quality")));
    }

    float tilt;
    if (element->QueryFloatAttribute("tilt", &tilt) == tinyxml2::XML_NO_ERROR)
    {
        sphericalMirrorProj->setTilt(tilt);
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "ReadConfig: Setting spherical mirror tilt to %f degrees.\n", tilt);
    }

    tinyxml2::XMLElement * subElement = element->FirstChildElement();
    const char * val;

    while (subElement != NULL)
    {
        val = subElement->Value();

        if (strcmp("Background", val) == 0)
        {
            glm::vec4 color;
            float ftmp;

            if (subElement->QueryFloatAttribute("r", &ftmp) == tinyxml2::XML_NO_ERROR)
                color.r = ftmp;
            if (subElement->QueryFloatAttribute("g", &ftmp) == tinyxml2::XML_NO_ERROR)
                color.g = ftmp;
            if (subElement->QueryFloatAttribute("b", &ftmp) == tinyxml2::XML_NO_ERROR)
                color.b = ftmp;
            if (subElement->QueryFloatAttribute("a", &ftmp) == tinyxml2::XML_NO_ERROR)
                color.a = ftmp;

            sphericalMirrorProj->setClearColor(color);
        }
        else if (strcmp("Geometry", val) == 0)
        {
            if (subElement->Attribute("bottom") != NULL)
            {
                sphericalMirrorProj->setMeshPath(SphericalMirrorProjection::BOTTOM_MESH, subElement->Attribute("bottom"));
            }

            if (subElement->Attribute("left") != NULL)
            {
                sphericalMirrorProj->setMeshPath(SphericalMirrorProjection::LEFT_MESH, subElement->Attribute("left"));
            }

            if (subElement->Attribute("right") != NULL)
            {
                sphericalMirrorProj->setMeshPath(SphericalMirrorProjection::RIGHT_MESH, subElement->Attribute("right"));
            }

            if (subElement->Attribute("top") != NULL)
            {
                sphericalMirrorProj->setMeshPath(SphericalMirrorProjection::TOP_MESH, subElement->Attribute("top"));
            }
        }

        //iterate
        subElement = subElement->NextSiblingElement();
    }

    sphericalMirrorProj->setUseDepthTransformation(false);
    mNonLinearProjection = sphericalMirrorProj;
}

void sgct_core::Viewport::setOverlayTexture(const char * texturePath)
{
    mOverlayFilename.assign(texturePath);
}

void sgct_core::Viewport::setBlendMaskTexture(const char * texturePath)
{
    mBlendMaskFilename.assign(texturePath);
}

void sgct_core::Viewport::setBlackLevelMaskTexture(const char * texturePath)
{
    mBlackLevelMaskFilename.assign(texturePath);
}

void sgct_core::Viewport::setCorrectionMesh(const char * meshPath)
{
    mMeshFilename.assign(meshPath);
}

void sgct_core::Viewport::setMpcdiWarpMesh(const char* meshData, size_t size)
{
    mMpcdiWarpMeshData = new char[size];
    memcpy(mMpcdiWarpMeshData, meshData, size);
    mMpcdiWarpMeshSize = size;
}

void sgct_core::Viewport::setTracked(bool state)
{
    mTracked = state;
}

void sgct_core::Viewport::loadData()
{
    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "Viewport: loading GPU data for '%s'\n", mName.c_str());
        
    if( mOverlayFilename.size() > 0 )
        sgct::TextureManager::instance()->loadUnManagedTexture(mOverlayTextureIndex, mOverlayFilename, true, 1);

    if ( mBlendMaskFilename.size() > 0 )
        sgct::TextureManager::instance()->loadUnManagedTexture(mBlendMaskTextureIndex, mBlendMaskFilename, true, 1);

    if ( mBlackLevelMaskFilename.size() > 0)
        sgct::TextureManager::instance()->loadUnManagedTexture(mBlackLevelMaskTextureIndex, mBlackLevelMaskFilename, true, 1);

    if ( mMpcdiWarpMeshData != nullptr )
    {
        mCorrectionMesh = mCM.readAndGenerateMesh("mesh.mpcdi", this, CorrectionMesh::parseHint("mpcdi"));
    }
    else
    {
        //load default if mMeshFilename is empty
        mCorrectionMesh = mCM.readAndGenerateMesh(mMeshFilename, this, CorrectionMesh::parseHint(mMeshHint));
    }
}

/*!
Render the viewport mesh which the framebuffer texture is attached to
\param type of mesh; quad, warped or mask
*/
void sgct_core::Viewport::renderMesh(sgct_core::CorrectionMesh::MeshType mt)
{
    if( mEnabled )
        mCM.render(mt);
}
