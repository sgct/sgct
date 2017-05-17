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

    if (mMpcdiSubFileContents.subFileBuffer[mMpcdiSubFileContents.mpcdiXml])
        delete mMpcdiSubFileContents.subFileBuffer[mMpcdiSubFileContents.mpcdiXml];
    if (mMpcdiSubFileContents.subFileBuffer[mMpcdiSubFileContents.mpcdiPfm])
        delete mMpcdiSubFileContents.subFileBuffer[mMpcdiSubFileContents.mpcdiPfm];
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
        else if (strcmp("MpcdiProjection", val) == 0)
        {
            mMeshHint.assign(element->Attribute("mpcdi"));
            parseMpcdiConfiguration(subElement);
        }
        else if (strcmp("Viewplane", val) == 0 || strcmp("Projectionplane", val) == 0)
        {
            mProjectionPlane.configure(subElement);
        }

        //iterate
        subElement = subElement->NextSiblingElement();
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
    mIsMeshStoredInFile = true;
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

    //load default if mMeshFilename is empty
    if (mIsMeshStoredInFile) {
        mCorrectionMesh = mCM.readAndGenerateMesh(mMeshFilename, this, CorrectionMesh::parseHint(mMeshHint), true);
    } else {
        mCorrectionMesh = mCM.readAndGenerateMesh(mMeshFilename, this, CorrectionMesh::parseHint("mpcdi"), false);
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

bool sgct_core::Viewport::doesStringHaveSuffix(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool sgct_core::Viewport::parseMpcdiConfiguration(tinyxml2::XMLElement * element)
{
    if (element->Attribute("file") != NULL) {
        std::string cfgFilePath = element->Attribute("file");
        FILE * cfgFile = nullptr;
        unzFile *zipfile = nullptr;

        if (! openZipFile(cfgFile, cfgFilePath, zipfile))
            return false;


        // Get info about the zip file
        unz_global_info global_info;
        if (unzGetGlobalInfo(zipfile, &global_info) != UNZ_OK)
        {
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                "parseMpcdiConfiguration: Unable to get zip archive info from %s\n", cfgFilePath);
            unzClose(zipfile);
            return false;
        }

        //Search for required files inside mpcdi archive file
        for (int i = 0; i < global_info.number_entry; ++i)
        {
            unz_file_info file_info;
            char filename[ MAX_FILENAME ];
            if (unzGetCurrentFileInfo(zipfile, &file_info, filename, MAX_FILENAME,
                NULL, 0, NULL, 0) != UNZ_OK)
            {
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                    "parseMpcdiConfiguration: Unable to get info on compressed file #%d\n", i);
                unzClose(zipfile);
                return false;
            }
            if (! processMpcdiSubFile(filename, zipfile, file_info))
                return false;
        }
        unzClose(zipfile);
        if (!hasFoundFile_xml || !hasFoundFile_pfm)
        {
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                "parseMpcdiConfiguration: mpcdi file %s does not contain xml and/or pfm file\n",
                cfgFilePath);
            return false;
        }
    }
    else
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
            "parseMpcdiConfiguration: Empty config file specified!\n");
        return false;
    }

    return true;
}

bool sgct_core::Viewport::openZipFile(FILE* cfgFile, const std::string cfgFilePath,
                                      unzFile* zipfile)
{
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if (fopen_s(&cfgFile, cfgFilePath.c_str(), "r") != 0 || !cfgFile)
#else
    cfgFile = fopen(cfgFilePath.c_str(), "r");
    if (cfgFile == nullptr)
#endif
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
            "parseMpcdiConfiguration: Failed to open file %s\n", cfgFilePath);
        return false;
    }
    //Open MPCDI file (zip compressed format)
    zipfile = unzOpen(cfgFilePath);
    if (zipfile == nullptr)
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
            "parseMpcdiConfiguration: Failed to open compressed mpcdi file %s\n", cfgFilePath);
        return false;
    }
    return true;
}

bool sgct_core::Viewport::processMpcdiSubFile(std::string filename, unzFile* zipfile,
                                              unz_global_info& file_info)
{
    for (int i = 0; i < mMpcdiSubFileContents.mpcdi_nRequiredFiles; ++i)
    {
        if (doesStringHaveSuffix(filename, mMpcdiSubFileContents.subFileExtension[i]))
        {
            mMpcdiSubFileContents.hasFoundFile[i] = true;
            mMpcdiSubFileContents.subFileSize[i] = file_info.uncompressed_size;
            if (unzOpenCurrentFile(zipfile) != UNZ_OK)
            {
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                    "parseMpcdiConfiguration: Unable to open %s\n", filename);
                unzClose(zipfile);
                return false;
            }
            mMpcdiSubFileContents.subFileBuffer[i] = new char(file_info.uncompressed_size);
            if (mMpcdiSubFileContents.subFileBuffer[i]) {
                error = unzReadCurrentFile(zipfile, mMpcdiSubFileContents.subFileBuffer[i],
                		                   file_info.uncompressed_size);
                unzCloseCurrentFile(zipfile);
                if (error < 0)
                {
                    sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                        "parseMpcdiConfiguration: %s read from %s failed.\n",
                        mMpcdiSubFileContents.subFileExtension[i], filename);
                    unzClose(zipfile);
                    return false;
                }
            } else {
                sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR,
                    "parseMpcdiConfiguration: Unable to allocate memory for %s\n", filename);
                unzClose(zipfile);
                return false;
            }
        }
	}
	return true;
}

bool sgct_core::Viewport::readAndParseXMLString()
{
    if (mMpcdiSubFileContents.subFileBuffer[mpcdiXml] == nullptr)
    	return false;

    tinyxml2::XMLDocument xmlDoc;
    XMLError result = xmlDoc.Parse(mMpcdiSubFileContents.subFileBuffer[mpcdiXml],
                                   mMpcdiSubFileContents.subFileSize[mpcdiXml]);

    if (result != tinyxml2::XML_NO_ERROR)
    {
        std::stringstream ss;
        if (xmlDoc.GetErrorStr1() && xmlDoc.GetErrorStr2())
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr1() << " " << xmlDoc.GetErrorStr2();
        else if (xmlDoc.GetErrorStr1())
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr1();
        else if (xmlDoc.GetErrorStr2())
            ss << "Parsing failed after: " << xmlDoc.GetErrorStr2();
        else
            ss << "File not found";
        mErrorMsg = ss.str();
        assert(false);
        return false;
    }
    else
        return readAndParseXML(xmlDoc);
}

bool sgct_core::Viewport::readAndParseXML(tinyxml2::XMLDocument& xmlDoc)
{
    tinyxml2::XMLElement* XMLroot = xmlDoc.FirstChildElement( "MPCDI" );
    if( XMLroot == NULL )
    {
        mErrorMsg.assign("Cannot find XML root!");
        return false;
    }

    tinyxml2::XMLElement* element[MAX_XML_DEPTH];
    for(unsigned int i=0; i < MAX_XML_DEPTH; i++)
        element[i] = NULL;
    const char * val[MAX_XML_DEPTH];
    element[0] = XMLroot->FirstChildElement();
    while( element[0] != NULL )
    {
        val[0] = element[0]->Value();
        if( strcmp("display", val[0]) == 0 )
        {
            element[1] = element[0]->FirstChildElement();
            while( element[1] != NULL )
            {
                val[1] = element[1]->Value();
                if( strcmp("buffer", val[1]) == 0 )
                {
                    if( element[1]->Attribute("id") != NULL )
                        //TODO

                    if (element[1]->Attribute("xResolution") != NULL)
                        //TODO

                    if (element[1]->Attribute("yResolution") != NULL)
                        //TODO

                    element[2] = element[1]->FirstChildElement();
                    while( element[2] != NULL )
                    {
                        val[2] = element[2]->Value();
                        if( strcmp("region", val[2]) == 0 )
                        {
                            if( element[2]->Attribute("id") != NULL )
                                //TODO

                            if (element[2]->Attribute("x") != NULL)
                                //TODO

                            if (element[2]->Attribute("y") != NULL)
                                //TODO

                            if (element[2]->Attribute("xSize") != NULL)
                                //TODO

                            if (element[2]->Attribute("ySize") != NULL)
                                //TODO

                            if (element[2]->Attribute("xResolution") != NULL)
                                //TODO

                            if (element[2]->Attribute("yResolution") != NULL)
                                //TODO

                            element[3] = element[2]->FirstChildElement();
                            while( element[3] != NULL )
                            {
                                val[3] = element[3]->Value();
                                if( strcmp("frustum", val[3]) == 0 )
                                {
                                    element[4] = element[3]->FirstChildElement();
                                    while( element[4] != NULL )
                                    {
                                        val[4] = element[4]->Value();
                                        if( strcmp("yaw", val[4]) == 0 )
                                            //TODO
                                        else if( strcmp("pitch", val[4]) == 0 )
                                            //TODO
                                        else if( strcmp("roll", val[4]) == 0 )
                                            //TODO
                                        else if( strcmp("rightAngle", val[4]) == 0 )
                                            //TODO
                                        else if( strcmp("leftAngle", val[4]) == 0 )
                                            //TODO
                                        else if( strcmp("upAngle", val[4]) == 0 )
                                            //TODO
                                        else if( strcmp("downAngle", val[4]) == 0 )
                                            //TODO
                                        element[4] = element[4]->NextSiblingElement();
                                    }
                                }
                                element[3] = element[3]->NextSiblingElement();
                            }
                        }
                        //iterate
                        element[2] = element[2]->NextSiblingElement();
                    }
                    tmpNode.addWindow( tmpWin );
                }//end window
                //iterate
                element[1] = element[1]->NextSiblingElement();
            }//end while
            ClusterManager::instance()->addNode(tmpNode);
        }//end display
        else if( strcmp("files", val[0]) == 0 )
        {
            element[1] = element[0]->FirstChildElement();
            while( element[1] != NULL )
            {
                val[1] = element[1]->Value();
                if( strcmp("fileset", val[1]) == 0 )
                {
                    if (element[1]->Attribute("region") != NULL)
                        //TODO
                    val[2] = element[1]->Value();
                    element[2] = element[1]->FirstChildElement();
                    while( element [2] != NULL )
                    {
                        if( strcmp("geometryWarpFile", val[2]) == 0 )
                        {
                            element[3] = element[2]->FirstChildElement();
                            while( element[3] != NULL )
                            {
                                val[3] = element[3]->Value();
                                if( strcmp("path", val[3]) == 0 )
                                    //TODO
                                if( strcmp("interpolation", val[3]) == 0 )
                                    //TODO
                                element[3] = element[3]->NextSiblingElement();
                            }
                        }
                        element[2] = element[2]->NextSiblingElement();
                    }
                }
                element[1] = element[1]->NextSiblingElement();
            }
        }//end 'files'

        //iterate
        element[0] = element[0]->NextSiblingElement();
    }

    return true;
}
