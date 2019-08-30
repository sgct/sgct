/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/CorrectionMesh.h>

#include <sgct/ClusterManager.h>
#include <sgct/Engine.h>
#include <sgct/MessageHandler.h>
#include <sgct/SGCTSettings.h>
#include <sgct/Viewport.h>
#include <sgct/SGCTUser.h>
#include <sgct/helpers/SGCTStringFunctions.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

#if (_MSC_VER >= 1400)
    #define _sscanf sscanf_s
#else
    #define _sscanf sscanf
#endif

//#define CONVERT_SCISS_TO_DOMEPROJECTION
//#define CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC

namespace {
    constexpr const int MaxLineLength = 1024;
    constexpr const int MaxXmlDepth = 16;

    bool readMeshBuffer(float* dest, unsigned int& idx, const unsigned char* src,
                        size_t srcSize_bytes, int readSize_bytes)
    {
        if ((idx + readSize_bytes) > srcSize_bytes) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "CorrectionMesh: Reached EOF in mesh buffer\n"
            );
            return false;
        }
        float val;
        memcpy(&val, &src[idx], readSize_bytes);
        *dest = val;
        idx += readSize_bytes;
        return true;
    }

} // namespace

namespace sgct_core {

struct SCISSTexturedVertex {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float tx = 0.f;
    float ty = 0.f;
    float tz = 0.f;
};

struct SCISSViewData {
    // Rotation quaternion
    float qx = 0.f;
    float qy = 0.f;
    float qz = 0.f;
    float qw = 1.f;

    // Position of view (currently unused in Uniview)
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;

    float fovUp = 20.f;
    float fovDown = 20.f;
    float fovLeft = 20.f;
    float fovRight = 20.f;
};

enum SCISSDistortionType {
    MESHTYPE_PLANAR,
    MESHTYPE_CUBE
};

CorrectionMesh::CorrectionMeshGeometry::~CorrectionMeshGeometry() {
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMeshGeometry: Releasing correction mesh OpenGL data\n"
    );

    if (ClusterManager::instance()->getMeshImplementation() ==
        ClusterManager::MeshImplementation::DisplayList)
    {
        glDeleteLists(mVertexMeshData, 1);
    }
    else {
        glDeleteVertexArrays(1, &mArrayMeshData);
        glDeleteBuffers(1, &mVertexMeshData);
        glDeleteBuffers(1, &mIndexMeshData);
    }
}

bool CorrectionMesh::readAndGenerateMesh(std::string meshPath, Viewport& parent,
                                         MeshHint hint)
{    
    // generate unwarped mask
    setupSimpleMesh(mGeometries.quad, parent);
    createMesh(mGeometries.quad);
    cleanUp();
    
    // generate unwarped mesh for mask
    if (parent.hasBlendMaskTexture() || parent.hasBlackLevelMaskTexture()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Creating mask mesh\n"
        );
        
        bool flipX = false;
        bool flipY = false;

        setupMaskMesh(parent, flipX, flipY);
        createMesh(mGeometries.mask);
        cleanUp();
    }

    // fallback if no mesh is provided
    if (meshPath.empty()) {
        setupSimpleMesh(mGeometries.warp, parent);
        createMesh(mGeometries.warp);
        cleanUp();

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Empty mesh path\n"
        );
        return false;
    }
    
    // transform to lowercase
    std::string path(meshPath);
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);

    bool loadStatus = false;

    // find a suitable format
    if (path.find(".sgc") != std::string::npos) {
        loadStatus = readAndGenerateScissMesh(meshPath, parent);
    }
    else if (path.find(".ol") != std::string::npos) {
        loadStatus = readAndGenerateScalableMesh(meshPath, parent);
    }
    else if (path.find(".skyskan") != std::string::npos) {
    }
    else if (path.find(".txt") != std::string::npos) {
        // default for this suffix
        if (hint == MeshHint::None || hint == MeshHint::SkySkan) {
            loadStatus = readAndGenerateSkySkanMesh(meshPath, parent);
        }
    }
    else if (path.find(".csv") != std::string::npos) {
        // default for this suffix
        if (hint == MeshHint::None || hint == MeshHint::DomeProjection) {
            loadStatus = readAndGenerateDomeProjectionMesh(meshPath, parent);
        }
    }
    else if (path.find(".data") != std::string::npos) {
        // default for this suffix
        if (hint == MeshHint::None || hint == MeshHint::PaulBourke) {
            loadStatus = readAndGeneratePaulBourkeMesh(meshPath, parent);
        }
    }
    else if (path.find(".obj") != std::string::npos) {
        // default for this suffix
        if (hint == MeshHint::None || hint == MeshHint::Obj) {
            loadStatus = readAndGenerateOBJMesh(meshPath, parent);
        }
    }
    else if (path.find(".mpcdi") != std::string::npos) {
        loadStatus = readAndGenerateMpcdiMesh("", parent);
    }
    else if (path.find(".simcad") != std::string::npos) {
        // default for this suffix
        if (hint == MeshHint::None || hint == MeshHint::SimCad) {
            loadStatus = readAndGenerateSimCADMesh(meshPath, parent);
        }
    }
    
    if (!loadStatus) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh error: Loading mesh '%s' failed\n", meshPath.c_str()
        );

        setupSimpleMesh(mGeometries.warp, parent);
        createMesh(mGeometries.warp);
        cleanUp();
        return false;
    }

    if (loadStatus && sgct::SGCTSettings::instance()->getExportWarpingMeshes()) {
        size_t found = meshPath.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = meshPath.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename));
        }
    }
    cleanUp();

    return true;
}

bool CorrectionMesh::readAndGenerateDomeProjectionMesh(const std::string& meshPath,
                                                       Viewport& parent)
{
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "CorrectionMesh: Reading DomeProjection mesh data from '%s'\n", meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(meshPath.c_str(), "r");
    if (meshFile == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    std::vector<CorrectionMeshVertex> vertices;

    unsigned int numberOfCols = 0;
    unsigned int numberOfRows = 0;
    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];
        
        float x;
        float y;
        float u;
        float v;
        unsigned int col;
        unsigned int row;

        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            if (_sscanf(lineBuffer, "%f;%f;%f;%f;%u;%u", &x, &y, &u, &v, &col, &row) == 6)
            {
                // init to max intensity (opaque white)
                CorrectionMeshVertex vertex;
                vertex.r = 1.f;
                vertex.g = 1.f;
                vertex.b = 1.f;
                vertex.a = 1.f;

                // find dimensions of meshdata
                numberOfCols = std::max(numberOfCols, col);
                numberOfRows = std::max(numberOfRows, row);

                glm::clamp(x, 0.f, 1.f);
                glm::clamp(y, 0.f, 1.f);

                const glm::vec2& pos = parent.getPosition();
                const glm::vec2& size = parent.getSize();

                // convert to [-1, 1]
                vertex.x = 2.f * (x * size.x + pos.x) - 1.f;

                // @TODO (abock, 2019-08-30); I'm not sure why the y inversion happens
                // here. It seems like a mistake, but who knows
                vertex.y = 2.f * ((1.f - y) * size.y + pos.y) - 1.f;

                // scale to viewport coordinates
                vertex.s = u * size.x + pos.x;
                vertex.t = (1.f - v) * size.y + pos.y;

                vertices.push_back(std::move(vertex));
            }
        }
    }

    fclose(meshFile);

    // add one to actually store the dimensions instread of largest index
    numberOfCols++;
    numberOfRows++;

    // copy vertices
    unsigned int numberOfVertices = numberOfCols * numberOfRows;
    mTempVertices = new CorrectionMeshVertex[numberOfVertices];
    memcpy(
        mTempVertices,
        vertices.data(),
        numberOfVertices * sizeof(CorrectionMeshVertex)
    );
    mGeometries.warp.mNumberOfVertices = numberOfVertices;
    vertices.clear();

    std::vector<unsigned int> indices;
    for (unsigned int c = 0; c < (numberOfCols - 1); c++) {
        for (unsigned int r = 0; r < (numberOfRows - 1); r++) {
            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

            const unsigned int i0 = r * numberOfCols + c;
            const unsigned int i1 = r * numberOfCols + (c + 1);
            const unsigned int i2 = (r + 1) * numberOfCols + (c + 1);
            const unsigned int i3 = (r + 1) * numberOfCols + c;

            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    mGeometries.warp.mNumberOfIndices = static_cast<unsigned int>(indices.size());
    mTempIndices = new unsigned int[mGeometries.warp.mNumberOfIndices];
    memcpy(
        mTempIndices,
        indices.data(),
        mGeometries.warp.mNumberOfIndices * sizeof(unsigned int)
    );
    indices.clear();

    mGeometries.warp.mGeometryType = GL_TRIANGLES;

    createMesh(mGeometries.warp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        mGeometries.warp.mNumberOfVertices,
        mGeometries.warp.mNumberOfIndices
    );
    
    return true;
}

bool CorrectionMesh::readAndGenerateScalableMesh(const std::string& meshPath,
                                                 Viewport& parent)
{
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "CorrectionMesh: Reading scalable mesh data from '%s'\n", meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(meshPath.c_str(), "r");
    if (meshFile == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    unsigned int numOfVerticesRead = 0;
    unsigned int numOfFacesRead = 0;
    unsigned int numberOfFaces = 0;
    unsigned int numberOfVertices = 0;
    unsigned int numberOfIndices = 0;

    struct {
        double left;
        double right;
        double bottom;
        double top;
    } orthoCoords;
    glm::ivec2 resolution;

    unsigned int a, b, c;
    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];

        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            float x, y, s, t;
            unsigned int intensity;

            if (_sscanf(lineBuffer, "%f %f %u %f %f", &x, &y, &intensity, &s, &t) == 5) {
                if (mTempVertices && resolution.x != 0 && resolution.y != 0) {
                    CorrectionMeshVertex& vertex = mTempVertices[numOfVerticesRead];
                    vertex.x = (x / static_cast<float>(resolution.x)) *
                                    parent.getSize().x + parent.getPosition().x;
                    vertex.y = (y / static_cast<float>(resolution.y)) *
                                    parent.getSize().y + parent.getPosition().y;
                    vertex.r = static_cast<float>(intensity) / 255.f;
                    vertex.g = static_cast<float>(intensity) / 255.f;
                    vertex.b = static_cast<float>(intensity) / 255.f;
                    vertex.a = 1.f;
                    vertex.s = (1.f - t) * parent.getSize().x + parent.getPosition().x;
                    vertex.t = (1.f - s) * parent.getSize().y + parent.getPosition().y;

                    numOfVerticesRead++;
                }
            }
            else if (_sscanf(lineBuffer, "[ %u %u %u ]", &a, &b, &c) == 3) {
                if (mTempIndices) {
                    mTempIndices[numOfFacesRead * 3] = a;
                    mTempIndices[numOfFacesRead * 3 + 1] = b;
                    mTempIndices[numOfFacesRead * 3 + 2] = c;
                }

                numOfFacesRead++;
            }
            else {
                char tmpBuf[16];
                tmpBuf[0] = '\0';
                double tmpD = 0.0;
                unsigned int tmpUI = 0;

                if (_sscanf(lineBuffer, "VERTICES %u", &numberOfVertices) == 1) {
                    mTempVertices = new CorrectionMeshVertex[numberOfVertices];
                    memset(
                        mTempVertices,
                        0,
                        numberOfVertices * sizeof(CorrectionMeshVertex)
                    );
                }

                else if (_sscanf(lineBuffer, "FACES %u", &numberOfFaces) == 1) {
                    numberOfIndices = numberOfFaces * 3;
                    mTempIndices = new unsigned int[numberOfIndices];
                    memset(mTempIndices, 0, numberOfIndices * sizeof(unsigned int));
                }
                else if (_sscanf(lineBuffer, "ORTHO_%s %lf", tmpBuf, 16, &tmpD) == 2) {
                    if (strcmp(tmpBuf, "LEFT") == 0) {
                        orthoCoords.left = tmpD;
                    }
                    else if (strcmp(tmpBuf, "RIGHT") == 0) {
                        orthoCoords.right = tmpD;
                    }
                    else if (strcmp(tmpBuf, "BOTTOM") == 0) {
                        orthoCoords.bottom = tmpD;
                    }
                    else if (strcmp(tmpBuf, "TOP") == 0) {
                        orthoCoords.top = tmpD;
                    }
                }
                else if (_sscanf(lineBuffer, "NATIVEXRES %u", &tmpUI) == 1) {
                    resolution.x = tmpUI;
                }
                else if (_sscanf(lineBuffer, "NATIVEYRES %u", &tmpUI) == 1) {
                    resolution.y = tmpUI;
                }
            }
        }
    }

    if (numberOfVertices != numOfVerticesRead || numberOfFaces != numOfFacesRead) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Incorrect mesh data geometry"
        );
        return false;
    }

    // normalize
    for (unsigned int i = 0; i < numberOfVertices; i++) {
        const float xMin = static_cast<float>(orthoCoords.left);
        const float xMax = static_cast<float>(orthoCoords.right);
        const float yMin = static_cast<float>(orthoCoords.bottom);
        const float yMax = static_cast<float>(orthoCoords.top);

        // normalize between 0.0 and 1.0
        const float xVal = (mTempVertices[i].x - xMin) / (xMax - xMin);
        const float yVal = (mTempVertices[i].y - yMin) / (yMax - yMin);

        // normalize between -1.0 to 1.0
        mTempVertices[i].x = xVal * 2.f - 1.f;
        mTempVertices[i].y = yVal * 2.f - 1.f;
    }

    fclose(meshFile);

    mGeometries.warp.mNumberOfVertices = numberOfVertices;
    mGeometries.warp.mNumberOfIndices = numberOfIndices;
    mGeometries.warp.mGeometryType = GL_TRIANGLES;

    createMesh(mGeometries.warp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Faces=%u\n",
        numOfVerticesRead, numOfFacesRead
    );

    return true;
}

bool CorrectionMesh::readAndGenerateScissMesh(const std::string& meshPath,
                                              Viewport& parent)
{    
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "CorrectionMesh: Reading sciss mesh data from '%s'\n",
        meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, meshPath.c_str(), "rb") != 0 || !meshFile) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(meshPath.c_str(), "rb");
    if (meshFile == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    size_t retval;
    unsigned int numberOfVertices = 0;
    unsigned int numberOfIndices = 0;

    char fileID[3];
#if (_MSC_VER >= 1400)
    retval = fread_s(fileID, sizeof(char) * 3, sizeof(char), 3, meshFile);
#else
    retval = fread(fileID, sizeof(char), 3, meshFile);
#endif

    // check fileID
    if (fileID[0] != 'S' || fileID[1] != 'G' || fileID[2] != 'C' || retval != 3) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Incorrect file id\n"
        );
        fclose(meshFile);
        return false;
    }

    // read file version
    uint8_t fileVersion;
#if (_MSC_VER >= 1400)
    retval = fread_s(&fileVersion, sizeof(uint8_t), sizeof(uint8_t), 1, meshFile);
#else
    retval = fread(&fileVersion, sizeof(uint8_t), 1, meshFile);
#endif
    if (retval != 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: file version %u\n", fileVersion
        );
    }

    // read mapping type
    unsigned int mappingType;
#if (_MSC_VER >= 1400)
    retval = fread_s(&mappingType, sizeof(unsigned int), sizeof(unsigned int), 1, meshFile);
#else
    retval = fread(&mappingType, sizeof(unsigned int), 1, meshFile);
#endif
    if (retval != 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Mapping type = %s (%u)\n",
            mappingType == 0 ? "planar" : "cube", mappingType
        );
    }

    // read viewdata
    SCISSViewData viewData;
    double yaw, pitch, roll;
#if (_MSC_VER >= 1400)
    retval = fread_s(&viewData, sizeof(SCISSViewData), sizeof(SCISSViewData), 1, meshFile);
#else
    retval = fread(&viewData, sizeof(SCISSViewData), 1, meshFile);
#endif
    if (retval != 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }
    else {
        double x = static_cast<double>(viewData.qx);
        double y = static_cast<double>(viewData.qy);
        double z = static_cast<double>(viewData.qz);
        double w = static_cast<double>(viewData.qw);
        
        // @TODO (abock, 2019-08-30): It seems weird that we are mixing the euler angles
        // from x,y,z to y,x,z. Maybe something related to left-handed and right-handed
        // coordinate systems?
        glm::dvec3 angles = glm::degrees(glm::eulerAngles(glm::dquat(w, y, x, z)));
        yaw = -angles.x;
        pitch = angles.y;
        roll = -angles.z;
        
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Rotation quat = [%f %f %f %f]\n"
            "yaw = %lf, pitch = %lf, roll = %lf\n",
            viewData.qx, viewData.qy, viewData.qz, viewData.qw, yaw, pitch, roll);

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Position = [%f %f %f]\n", viewData.x, viewData.y, viewData.z
        );

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: FOV up = %f\n", viewData.fovUp
        );

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: FOV down = %f\n", viewData.fovDown
        );

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: FOV left = %f\n", viewData.fovLeft
        );

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: FOV right = %f\n", viewData.fovRight
        );
    }

    // read number of vertices
    unsigned int size[2];
#if (_MSC_VER >= 1400)
    retval = fread_s(size, sizeof(unsigned int)*2, sizeof(unsigned int), 2, meshFile);
#else
    retval = fread(size, sizeof(unsigned int), 2, meshFile);
#endif
    if (retval != 2) { 
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }

    if (fileVersion == 2) {
        numberOfVertices = size[1];
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Number of vertices = %u\n", numberOfVertices
        );
    }
    else {
        numberOfVertices = size[0] * size[1];
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Number of vertices = %u (%ux%u)\n",
            numberOfVertices, size[0], size[1]
        );
    }
    // read vertices
    std::vector< SCISSTexturedVertex> texturedVertexList(numberOfVertices);
#if (_MSC_VER >= 1400)
    retval = fread_s(
        texturedVertexList.data(),
        sizeof(SCISSTexturedVertex) * numberOfVertices,
        sizeof(SCISSTexturedVertex),
        numberOfVertices,
        meshFile
    );
#else
    retval = fread(texturedVertexList, sizeof(SCISSTexturedVertex), numberOfVertices, meshFile);
#endif
    if (retval != numberOfVertices) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }

    // read number of indices
#if (_MSC_VER >= 1400)
    retval = fread_s(
        &numberOfIndices,
        sizeof(unsigned int),
        sizeof(unsigned int),
        1,
        meshFile
    );
#else
    retval = fread(&numberOfIndices, sizeof(unsigned int), 1, meshFile);
#endif

    if (retval != 1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Number of indices = %u\n", numberOfIndices
        );
    }

    // read faces
    if (numberOfIndices > 0) {
        mTempIndices = new unsigned int[numberOfIndices];
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        retval = fread_s(
            mTempIndices,
            sizeof(unsigned int) * numberOfIndices,
            sizeof(unsigned int),
            numberOfIndices,
            meshFile
        );
#else
        retval = fread(mTempIndices, sizeof(unsigned int), numberOfIndices, meshFile);
#endif
        if (retval != numberOfIndices) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "CorrectionMesh: Error parsing file\n"
            );
            fclose(meshFile);
            return false;
        }
    }

    fclose(meshFile);

    parent.getUser()->setPos(glm::vec3(viewData.x, viewData.y, viewData.z));

    parent.setViewPlaneCoordsUsingFOVs(
        viewData.fovUp,
        viewData.fovDown,
        viewData.fovLeft,
        viewData.fovRight,
        glm::quat(viewData.qw, viewData.qx, viewData.qy, viewData.qz)
    );

    sgct::Engine::instance()->updateFrustums();

    mTempVertices = new CorrectionMeshVertex[numberOfVertices];
    for (unsigned int i = 0; i < numberOfVertices; i++) {
        CorrectionMeshVertex& vertex = mTempVertices[i];
        SCISSTexturedVertex& scissVertex = texturedVertexList[i];

        vertex.r = 1.f;
        vertex.g = 1.f;
        vertex.b = 1.f;
        vertex.a = 1.f;

        scissVertex.x = glm::clamp(scissVertex.x, 0.f, 1.f);
        scissVertex.y = glm::clamp(scissVertex.y, 0.f, 1.f);
        scissVertex.tx = glm::clamp(scissVertex.tx, 0.f, 1.f);
        scissVertex.ty = glm::clamp(scissVertex.ty, 0.f, 1.f);

        const glm::vec2& size = parent.getSize();
        const glm::vec2& pos = parent.getPosition();

        // convert to [-1, 1]
        vertex.x = 2.f * (scissVertex.x * size.x + pos.x) - 1.f;
        vertex.y = 2.f * ((1.f - scissVertex.y) * size.y + pos.y) - 1.f;

        vertex.s = scissVertex.tx * parent.getSize().x + parent.getPosition().x;
        vertex.t = scissVertex.ty * parent.getSize().y + parent.getPosition().y;
    }

#ifdef CONVERT_SCISS_TO_DOMEPROJECTION
    // test export to dome projection
    std::string baseOutFilename = meshPath.substr(0, meshPath.find_last_of(".sgc") - 3);
    
    // test export frustum
    std::string outFrustumFilename = baseOutFilename + "_frustum" + std::string(".csv");
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Exporting dome projection frustum file \"%s\"\n",
        outFrustumFilename.c_str()
    );
    std::ofstream outFrustumFile;
    outFrustumFile.open(outFrustumFilename, std::ios::out);
    outFrustumFile << "x;y;z;heading;pitch;bank;left;right;bottom;top;tanLeft;tanRight;"
        "tanBottom;tanTop;width;height" << std::endl;
    outFrustumFile << std::fixed;
    outFrustumFile << std::setprecision(8);
    outFrustumFile << viewData.x << ";" << viewData.y << ";" << viewData.z << ";"; //x y z
    outFrustumFile << yaw << ";" << pitch << ";" << roll << ";";
    outFrustumFile << viewData.fovLeft << ";" << viewData.fovRight << ";" <<
                      viewData.fovDown << ";" << viewData.fovUp << ";";
    float tanLeft = tan(glm::radians(viewData.fovLeft));
    float tanRight = tan(glm::radians(viewData.fovRight));
    float tanBottom = tan(glm::radians(viewData.fovDown));
    float tanTop = tan(glm::radians(viewData.fovUp));
    outFrustumFile << tanLeft << ";" << tanRight << ";"
                   << tanBottom << ";" << tanTop << ";";
    outFrustumFile << tanRight - tanLeft << ";";
    outFrustumFile << tanTop - tanBottom << std::endl;
    outFrustumFile.close();

    // test export mesh
    std::string outMeshFilename = baseOutFilename + "_mesh" + std::string(".csv");
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Exporting dome projection mesh file \"%s\"\n",
        outMeshFilename.c_str()
    );
    std::ofstream outMeshFile;
    outMeshFile.open(outMeshFilename, std::ios::out);
    outMeshFile << "x;y;u;v;column;row" << std::endl;
    outMeshFile << std::fixed;
    outMeshFile << std::setprecision(6);

    unsigned int i = 0;
    for (unsigned int y = 0; y < size[1]; ++y) {
        for (unsigned int x = 0; x < size[0]; ++x) {
            outMeshFile << texturedVertexList[i].x << ";";
            outMeshFile << texturedVertexList[i].y << ";";
            outMeshFile << texturedVertexList[i].tx << ";";
            outMeshFile << (1.0f - texturedVertexList[i].ty) << ";"; //flip v-coord
            outMeshFile << x << ";";
            outMeshFile << y << std::endl;
            ++i;
        }
    }
    outMeshFile.close();
#endif

    mGeometries.warp.mNumberOfVertices = numberOfVertices;
    mGeometries.warp.mNumberOfIndices = numberOfIndices;
    
    if (fileVersion == '2' && size[0] == 4) {
        mGeometries.warp.mGeometryType = GL_TRIANGLES;
    }
    else if (fileVersion == '2' && size[0] == 5) {
        mGeometries.warp.mGeometryType = GL_TRIANGLE_STRIP;
    }
    else { // assume v1
        //GL_QUAD_STRIP removed in OpenGL 3.3+
        //mGeometries[WARP_MESH].mGeometryType = GL_QUAD_STRIP;
        mGeometries.warp.mGeometryType = GL_TRIANGLE_STRIP;
    }

    texturedVertexList.clear();

    createMesh(mGeometries.warp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        numberOfVertices, numberOfIndices
    );
    
    return true;
}

bool CorrectionMesh::readAndGenerateSimCADMesh(const std::string& meshPath,
                                               Viewport& parent)
{
    // During projector alignment, a 33x33 matrix is used. This means 33x33 points can be
    // set to define geometry correction. So(x, y) coordinates are defined by the 33x33
    // matrix and the resolution used, defined by the tag. And the corrections to be
    // applied for every point in that 33x33 matrix, are stored in the warp file. This 
    // explains why this file only contains zero’s when no warp is applied.

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "CorrectionMesh: Reading simcad warp data from '%s'\n",
        meshPath.c_str()
    );

    float xrange = 1.f;
    float yrange = 1.f;
    std::vector<float> xcorrections, ycorrections;

    tinyxml2::XMLDocument xmlDoc;
    if (xmlDoc.LoadFile(meshPath.c_str()) != tinyxml2::XML_NO_ERROR) {
        std::string str = "Parsing failed after: ";
        if (xmlDoc.GetErrorStr1() && xmlDoc.GetErrorStr2()) {
            str += xmlDoc.GetErrorStr1();
            str += ' ';
            str += xmlDoc.GetErrorStr2();
        }
        else if (xmlDoc.GetErrorStr1()) {
            str += xmlDoc.GetErrorStr1();
        }
        else if (xmlDoc.GetErrorStr2()) {
            str += xmlDoc.GetErrorStr2();
        }
        else {
            str = "File not found";
        }

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "ReadConfig: Error occured while reading config file '%s'\nError: %s\n",
            meshPath.c_str(), str.c_str()
        );
        return false;
    }
    else {
        tinyxml2::XMLElement* XMLroot = xmlDoc.FirstChildElement("GeometryFile");
        if (XMLroot == nullptr) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "ReadConfig: Error occured while reading config file '%s'\n"
                "Error: Cannot find XML root\n", meshPath.c_str()
            );
            return false;
        }

        //tinyxml2::XMLElement* element[MaxXmlDepth];
        //const char* val[MaxXmlDepth];
        using namespace tinyxml2;
        XMLElement* element = XMLroot->FirstChildElement();
        if (element) {
            std::string_view val = element->Value();

            if (val == "GeometryDefinition") {
                XMLElement* child = element->FirstChildElement();
                while (child) {
                    std::string_view childVal = child->Value();
                    
                    if (childVal == "X-FlatParameters") {
                        if (child->QueryFloatAttribute("range", &xrange) == XML_NO_ERROR) {
                            std::string xcoordstr(child->GetText());
                            std::vector<std::string> xcoords = sgct_helpers::split(xcoordstr, ' ');
                            for (const std::string& x : xcoords) {
                                xcorrections.push_back(std::stof(x) / xrange);
                            } 
                        }
                    }
                    else if (childVal == "Y-FlatParameters") {
                        if (child->QueryFloatAttribute("range", &yrange) == XML_NO_ERROR) {
                            std::string ycoordstr(child->GetText());
                            std::vector<std::string> ycoords = sgct_helpers::split(ycoordstr, ' ');
                            for (const std::string& y : ycoords) {
                                ycorrections.push_back(std::stof(y) / yrange);
                            }
                        }
                    }

                    child = child->NextSiblingElement();
                }
            }
        }
    }

    if (xcorrections.size() != ycorrections.size()) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Not the same x coords as y coords\n"
        );
        return false;
    }

    float numberOfColsf = sqrtf(static_cast<float>(xcorrections.size()));
    float numberOfRowsf = sqrtf(static_cast<float>(ycorrections.size()));

    if (ceilf(numberOfColsf) != numberOfColsf || ceilf(numberOfRowsf) != numberOfRowsf) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Not a valid squared matrix read from SimCAD file\n"
        );
        return false;
    }

    unsigned int numberOfCols = static_cast<unsigned int>(numberOfColsf);
    unsigned int numberOfRows = static_cast<unsigned int>(numberOfRowsf);


#ifdef CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC
    // export to dome projection
    std::string baseOutFilename = meshPath.substr(0, meshPath.find_last_of(".simcad") - 6);

    // export domeprojection frustum
    std::string outFrustumFilename = baseOutFilename + "_frustum" + std::string(".csv");
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Exporting dome projection frustum file \"%s\"\n",
        outFrustumFilename.c_str()
    );
    std::ofstream outFrustumFile;
    outFrustumFile.open(outFrustumFilename, std::ios::out);
    outFrustumFile << "x;y;z;heading;pitch;bank;left;right;bottom;top;tanLeft;tanRight;"
                      "tanBottom;tanTop;width;height" << std::endl;
    outFrustumFile << std::fixed;
    outFrustumFile << std::setprecision(8);

    // write viewdata
    SCISSViewData viewData;

    glm::quat rotation = parent.getRotation();
    viewData.qw = rotation.w;
    viewData.qx = rotation.x;
    viewData.qy = rotation.y;
    viewData.qz = rotation.z;

    glm::vec3 position = parent.getUser()->getPos();
    viewData.x = position.x;
    viewData.y = position.y;
    viewData.z = position.z;

    glm::vec4 fov = parent.getFOV();
    viewData.fovUp = fov.x;
    viewData.fovDown = -fov.y;
    viewData.fovLeft = -fov.z;
    viewData.fovRight = fov.w;

    double yaw, pitch, roll;
    glm::dvec3 angles = glm::degrees(glm::eulerAngles(glm::dquat(static_cast<double>(viewData.qw), static_cast<double>(viewData.qy), static_cast<double>(viewData.qx), static_cast<double>(viewData.qz))));
    yaw = -angles.x;
    pitch = angles.y;
    roll = -angles.z;

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Rotation quat = [%f %f %f %f]\nyaw = %lf, pitch = %lf, roll = %lf\n",
        viewData.qx, viewData.qy, viewData.qz, viewData.qw, yaw, pitch, roll
    );

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Position = [%f %f %f]\n",
        viewData.x, viewData.y, viewData.z
    );

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: FOV up = %f\n",
        viewData.fovUp
    );

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: FOV down = %f\n",
        viewData.fovDown
    );

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: FOV left = %f\n",
        viewData.fovLeft
    );

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: FOV right = %f\n",
        viewData.fovRight
    );


    outFrustumFile << viewData.x << ";" << viewData.y << ";" << viewData.z << ";"; //x y z
    outFrustumFile << yaw << ";" << pitch << ";" << roll << ";";
    outFrustumFile << viewData.fovLeft << ";" << viewData.fovRight << ";"
                   << viewData.fovDown << ";" << viewData.fovUp << ";";
    float tanLeft = tan(glm::radians(viewData.fovLeft));
    float tanRight = tan(glm::radians(viewData.fovRight));
    float tanBottom = tan(glm::radians(viewData.fovDown));
    float tanTop = tan(glm::radians(viewData.fovUp));
    outFrustumFile << tanLeft << ";" << tanRight << ";"
                   << tanBottom << ";" << tanTop << ";";
    outFrustumFile << tanRight - tanLeft << ";";
    outFrustumFile << tanTop - tanBottom << std::endl;
    outFrustumFile.close();

    //start sgc export
    std::string outSGCFilenames[2] = {
        baseOutFilename + std::string("_u2.sgc"),
        baseOutFilename + std::string("_u3.sgc")
    };
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Exporting sgc v1(u2) file \"%s\" and v2(u3) file \"%s\"\n",
        outSGCFilenames[0].c_str(), outSGCFilenames[1].c_str()
    );
    std::ofstream outSGCFiles[2];
    outSGCFiles[0].open(outSGCFilenames[0], std::ios::out | std::ios::binary);
    outSGCFiles[1].open(outSGCFilenames[1], std::ios::out | std::ios::binary);

    outSGCFiles[0] << "SGC";
    outSGCFiles[1] << "SGC";

    uint8_t SGCversion1 = 1;
    uint8_t SGCversion2 = 2;

    SCISSDistortionType dT = MESHTYPE_PLANAR;
    unsigned int vertexCount = numberOfCols*numberOfRows;
    unsigned int primType = 5; //Triangle list

    viewData.fovLeft = -viewData.fovLeft;
    viewData.fovDown = -viewData.fovDown;
    
    outSGCFiles[0].write(reinterpret_cast<const char*>(&SGCversion1), sizeof(uint8_t));
    outSGCFiles[0].write(reinterpret_cast<const char*>(&dT), sizeof(SCISSDistortionType));
    outSGCFiles[0].write(reinterpret_cast<const char*>(&viewData), sizeof(SCISSViewData));
    outSGCFiles[0].write(reinterpret_cast<const char*>(&numberOfCols), sizeof(unsigned int));
    outSGCFiles[0].write(reinterpret_cast<const char*>(&numberOfRows), sizeof(unsigned int));

    outSGCFiles[1].write(reinterpret_cast<const char*>(&SGCversion2), sizeof(uint8_t));
    outSGCFiles[1].write(reinterpret_cast<const char*>(&dT), sizeof(SCISSDistortionType));
    outSGCFiles[1].write(reinterpret_cast<const char*>(&viewData), sizeof(SCISSViewData));
    outSGCFiles[1].write(reinterpret_cast<const char*>(&primType), sizeof(unsigned int));
    outSGCFiles[1].write(reinterpret_cast<const char*>(&vertexCount), sizeof(unsigned int));

    //test export mesh
    std::string outMeshFilename = baseOutFilename + "_mesh" + std::string(".csv");
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Exporting dome projection mesh file \"%s\"\n",
        outMeshFilename.c_str()
    );
    std::ofstream outMeshFile;
    outMeshFile.open(outMeshFilename, std::ios::out);
    outMeshFile << "x;y;u;v;column;row" << std::endl;
    outMeshFile << std::fixed;
    outMeshFile << std::setprecision(6);
#endif // CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC

    CorrectionMeshVertex vertex;
    std::vector<CorrectionMeshVertex> vertices;
    SCISSTexturedVertex scissVertex;

    // init to max intensity (opaque white)
    vertex.r = 1.0f;
    vertex.g = 1.0f;
    vertex.b = 1.0f;
    vertex.a = 1.0f;

    float x, y, u, v;
    size_t i = 0;

    for (unsigned int r = 0; r < numberOfRows; r++) {
        for (unsigned int c = 0; c < numberOfCols; c++) {
            //vertex-mapping
            u = (static_cast<float>(c) / (static_cast<float>(numberOfCols) - 1.f));
            v = 1.f - (static_cast<float>(r) / (static_cast<float>(numberOfRows) - 1.f));

            x = u + xcorrections[i];
            y = v - ycorrections[i];

            //convert to [-1, 1]
            vertex.x = 2.f * (x * parent.getSize().x + parent.getPosition().x) - 1.f;
            vertex.y = 2.f * (y * parent.getSize().y + parent.getPosition().y) - 1.f;

            //scale to viewport coordinates
            vertex.s = u * parent.getSize().x + parent.getPosition().x;
            vertex.t = v * parent.getSize().y + parent.getPosition().y;

            vertices.push_back(vertex);

            i++;

#ifdef CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC
            outMeshFile << x << ";";
            outMeshFile << 1.f - y << ";";
            outMeshFile << u << ";";
            outMeshFile << 1.f - v << ";";
            outMeshFile << c << ";";
            outMeshFile << r << std::endl;

            scissVertex.x = x;
            scissVertex.y = 1.f - y;
            scissVertex.tx = u;
            scissVertex.ty = v;

            outSGCFiles[0].write(
                reinterpret_cast<const char *>(&scissVertex),
                sizeof(SCISSTexturedVertex)
            );
            outSGCFiles[1].write(
                reinterpret_cast<const char *>(&scissVertex),
                sizeof(SCISSTexturedVertex)
            );
#endif // CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC
        }

    }

    //copy vertices
    unsigned int numberOfVertices = numberOfCols * numberOfRows;
    mTempVertices = new CorrectionMeshVertex[numberOfVertices];
    memcpy(mTempVertices, vertices.data(), numberOfVertices * sizeof(CorrectionMeshVertex));
    mGeometries.warp.mNumberOfVertices = numberOfVertices;
    vertices.clear();

    // Make a triangle strip index list
    std::vector<unsigned int> indices_trilist;
    for (unsigned int r = 0; r<numberOfRows - 1; r++) {
        if ((r & 1) == 0) { // even rows
            for (unsigned int c = 0; c < numberOfCols; c++) {
                indices_trilist.push_back(c + r * numberOfCols);
                indices_trilist.push_back(c + (r + 1) * numberOfCols);
            }
        }
        else { // odd rows
            for (unsigned int c = numberOfCols - 1; c > 0; c--) {
                indices_trilist.push_back(c + (r + 1) * numberOfCols);
                indices_trilist.push_back(c - 1 + r * numberOfCols);
            }
        }
    }

#ifdef CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC
    // Implementation of individual triangle index list
    /*std::vector<unsigned int> indices_tris;
    unsigned int i0, i1, i2, i3;
    for (unsigned int c = 0; c < (numberOfCols - 1); c++)
    {
        for (unsigned int r = 0; r < (numberOfRows - 1); r++)
        {
            i0 = r * numberOfCols + c;
            i1 = r * numberOfCols + (c + 1);
            i2 = (r + 1) * numberOfCols + (c + 1);
            i3 = (r + 1) * numberOfCols + c;

            //triangle 1
            indices_tris.push_back(i0);
            indices_tris.push_back(i1);
            indices_tris.push_back(i2);

            //triangle 2
            indices_tris.push_back(i0);
            indices_tris.push_back(i2);
            indices_tris.push_back(i3);
        }
    }*/

    outMeshFile.close();

    unsigned int indicesCount = static_cast<unsigned int>(indices_trilist.size());
    outSGCFiles[0].write(reinterpret_cast<const char*>(&indicesCount), sizeof(unsigned int));
    outSGCFiles[1].write(reinterpret_cast<const char*>(&indicesCount), sizeof(unsigned int));
    for (size_t i = 0; i < indices_trilist.size(); i++) {
        outSGCFiles[0].write(reinterpret_cast<const char*>(&indices_trilist[i]), sizeof(unsigned int));
        outSGCFiles[1].write(reinterpret_cast<const char*>(&indices_trilist[i]), sizeof(unsigned int));
    }

    /*indicesCount = static_cast<unsigned int>(indices_tris.size());
    outSGCFiles[1].write(reinterpret_cast<const char *>(&indicesCount), sizeof(unsigned int));
    for (size_t i = 0; i < indices_tris.size(); i++) {
        outSGCFiles[1].write(reinterpret_cast<const char *>(&indices_tris[i]), sizeof(unsigned int));
    }
    indices_tris.clear();*/

    outSGCFiles[0].close();
    outSGCFiles[1].close();
#endif // CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC

    //allocate and copy indices
    mGeometries.warp.mNumberOfIndices = static_cast<unsigned int>(indices_trilist.size());
    mTempIndices = new unsigned int[mGeometries.warp.mNumberOfIndices];
    memcpy(
        mTempIndices,
        indices_trilist.data(),
        mGeometries.warp.mNumberOfIndices * sizeof(unsigned int)
    );
    indices_trilist.clear();

    mGeometries.warp.mGeometryType = GL_TRIANGLE_STRIP;

    createMesh(mGeometries.warp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        mGeometries.warp.mNumberOfVertices,
        mGeometries.warp.mNumberOfIndices
    );

    return true;
}

bool CorrectionMesh::readAndGenerateSkySkanMesh(const std::string& meshPath,
                                                Viewport& parent)
{
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "CorrectionMesh: Reading SkySkan mesh data from '%s'\n",
        meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(meshPath.c_str(), "r");
    if (meshFile == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    float azimuth = 0.f;
    float elevation = 0.f;
    float horizontal_fov = 0.f;
    float vertical_fov = 0.f;
    float fovTweaks[2] = { 1.f, 1.f };
    float UVTweaks[2] = { 1.f, 1.f };
    bool dimensionsSet = false;
    bool azimuthSet = false;
    bool elevationSet = false;
    bool hFovSet = false;
    bool vFovSet = false;
    float x, y, u, v;

    unsigned int size[2];
    unsigned int counter = 0;

    char lineBuffer[MaxLineLength];
    while (!feof(meshFile)) {
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {

            if (_sscanf(lineBuffer, "Dome Azimuth=%f", &azimuth) == 1) {
                azimuthSet = true;
            }
            else if (_sscanf(lineBuffer, "Dome Elevation=%f", &elevation) == 1) {
                elevationSet = true;
            }
            else if (_sscanf(lineBuffer, "Horizontal FOV=%f", &horizontal_fov) == 1) {
                hFovSet = true;
            }
            else if (_sscanf(lineBuffer, "Vertical FOV=%f", &vertical_fov) == 1) {
                vFovSet = true;
            }
            else if (_sscanf(lineBuffer, "Horizontal Tweak=%f", &fovTweaks[0]) == 1) {
                ;
            }
            else if (_sscanf(lineBuffer, "Vertical Tweak=%f", &fovTweaks[1]) == 1) {
                ;
            }
            else if (_sscanf(lineBuffer, "U Tweak=%f", &UVTweaks[0]) == 1) {
                ;
            }
            else if (_sscanf(lineBuffer, "V Tweak=%f", &UVTweaks[1]) == 1) {
                ;
            }
            else if (!dimensionsSet && _sscanf(lineBuffer, "%u %u", &size[0], &size[1]) == 2)
            {
                dimensionsSet = true;
                mTempVertices = new CorrectionMeshVertex[size[0] * size[1]];
                mGeometries.warp.mNumberOfVertices = size[0] * size[1];
            }
            else if (dimensionsSet && _sscanf(lineBuffer, "%f %f %f %f", &x, &y, &u, &v) == 4)
            {
                if (UVTweaks[0] > -1.f) {
                    u *= UVTweaks[0];
                }

                if (UVTweaks[1] > -1.f) {
                    v *= UVTweaks[1];
                }
                
                mTempVertices[counter].x = x;
                mTempVertices[counter].y = y;
                mTempVertices[counter].s = u;
                //mTempVertices[counter].t = v;
                //mTempVertices[counter].s = 1.0f - u;
                mTempVertices[counter].t = 1.f - v;

                mTempVertices[counter].r = 1.f;
                mTempVertices[counter].g = 1.f;
                mTempVertices[counter].b = 1.f;
                mTempVertices[counter].a = 1.f;

                //fprintf(stderr, "Adding vertex: %u %.3f %.3f %.3f %.3f\n", counter, x, y, u, v);

                counter++;
            }

            //fprintf(stderr, "Row text: %s", lineBuffer);
        }
    }

    fclose(meshFile);

    if (!dimensionsSet || !azimuthSet || !elevationSet || !hFovSet ||
        horizontal_fov <= 0.f)
    {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Data reading error\n"
        );
        return false;
    }

    //create frustums and projection matrices
    if (!vFovSet || vertical_fov <= 0.f) {
        //half the width (radius is one unit, cancels it self out)
        float hw = tanf(glm::radians<float>(horizontal_fov) / 2.f);
        //half height
        float hh = (1200.f / 2048.f) * hw;
        
        vertical_fov = 2.f * glm::degrees<float>(atanf(hh));

        fprintf(stderr, "HFOV: %f VFOV: %f\n", horizontal_fov, vertical_fov);
        
        //vertical_fov = (1200.0f / 2048.0f) * horizontal_fov;
        //vertical_fov = horizontal_fov;
    }

    //correct fovs ??
    //horizontal_fov *= 0.948f;
    //vertical_fov *= 1.51f;

    if (fovTweaks[0] > 0.f) {
        horizontal_fov *= fovTweaks[0];
    }
    if (fovTweaks[1] > 0.f) {
        vertical_fov *= fovTweaks[1];
    }

    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(-azimuth), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(elevation), glm::vec3(1.f, 0.f, 0.f));

    parent.getUser()->setPos(glm::vec3(0.f));
    parent.setViewPlaneCoordsUsingFOVs(
        vertical_fov / 2.f,
        -vertical_fov / 2.f,
        -horizontal_fov / 2.f,
        horizontal_fov / 2.f,
        rotQuat
    );
    
    sgct::Engine::instance()->updateFrustums();

    std::vector<unsigned int> indices;
    unsigned int i0, i1, i2, i3;
    for (unsigned int c = 0; c < (size[0] - 1); c++) {
        for (unsigned int r = 0; r < (size[1] - 1); r++) {
            i0 = r * size[0] + c;
            i1 = r * size[0] + (c + 1);
            i2 = (r + 1) * size[0] + (c + 1);
            i3 = (r + 1) * size[0] + c;

            //fprintf(stderr, "Indexes: %u %u %u %u\n", i0, i1, i2, i3);

            /*

            3      2
             x____x
             |   /|
             |  / |
             | /  |
             |/   |
             x----x
            0      1

            */

            //triangle 1
            if (mTempVertices[i0].x != -1.f && mTempVertices[i0].y != -1.f &&
                mTempVertices[i1].x != -1.f && mTempVertices[i1].y != -1.f &&
                mTempVertices[i2].x != -1.f && mTempVertices[i2].y != -1.f)
            {
                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);
            }

            //triangle 2
            if (mTempVertices[i0].x != -1.f && mTempVertices[i0].y != -1.f &&
                mTempVertices[i2].x != -1.f && mTempVertices[i2].y != -1.f &&
                mTempVertices[i3].x != -1.f && mTempVertices[i3].y != -1.f)
            {
                indices.push_back(i0);
                indices.push_back(i2);
                indices.push_back(i3);
            }
        }
    }

    for (unsigned int i = 0; i < mGeometries.warp.mNumberOfVertices; i++) {
        //clamp
        /*if (mTempVertices[i].x > 1.0f)
            mTempVertices[i].x = 1.0f;
        else if (mTempVertices[i].x < 0.0f)
            mTempVertices[i].x = 0.0f;

        if (mTempVertices[i].y > 1.0f)
            mTempVertices[i].y = 1.0f;
        else if (mTempVertices[i].y < 0.0f)
            mTempVertices[i].y = 0.0f;

        if (mTempVertices[i].s > 1.0f)
            mTempVertices[i].s = 1.0f;
        else if (mTempVertices[i].s < 0.0f)
            mTempVertices[i].s = 0.0f;

        if (mTempVertices[i].t > 1.0f)
            mTempVertices[i].t = 1.0f;
        else if (mTempVertices[i].t < 0.0f)
            mTempVertices[i].t = 0.0f;*/

        //convert to [-1, 1]
        mTempVertices[i].x = 2.f * (mTempVertices[i].x *
                             parent.getSize().x + parent.getPosition().x) - 1.f;
        //mTempVertices[i].x = 2.0f*((1.0f - mTempVertices[i].x) * parent->getXSize() + parent->getX()) - 1.0f;
        //mTempVertices[i].y = 2.0f*(mTempVertices[i].y * parent->getYSize() + parent->getY()) - 1.0f;
        mTempVertices[i].y = 2.f * ((1.f - mTempVertices[i].y) *
                             parent.getSize().y + parent.getPosition().y) - 1.f;
        //test code
        //mTempVertices[i].x /= 1.5f;
        //mTempVertices[i].y /= 1.5f;

        mTempVertices[i].s = mTempVertices[i].s * parent.getSize().x + parent.getPosition().x;
        mTempVertices[i].t = mTempVertices[i].t * parent.getSize().y + parent.getPosition().y;
    }

    //allocate and copy indices
    mGeometries.warp.mNumberOfIndices = static_cast<unsigned int>(indices.size());
    mTempIndices = new unsigned int[mGeometries.warp.mNumberOfIndices];
    memcpy(
        mTempIndices,
        indices.data(),
        mGeometries.warp.mNumberOfIndices * sizeof(unsigned int)
    );

    mGeometries.warp.mGeometryType = GL_TRIANGLES;

    createMesh(mGeometries.warp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        mGeometries.warp.mNumberOfVertices,
        mGeometries.warp.mNumberOfIndices
    );

    return true;
}

bool CorrectionMesh::readAndGeneratePaulBourkeMesh(const std::string& meshPath,
                                                   Viewport& parent)
{
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "CorrectionMesh: Reading Paul Bourke spherical mirror mesh data from '%s'\n",
        meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(meshPath.c_str(), "r");
    if (meshFile == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    //variables
    int mappingType = -1;
    int size[2] = { -1, -1 };
    unsigned int counter = 0;
    float x, y, s, t, intensity;
    char lineBuffer[MaxLineLength];

    //get the fist line containing the mapping type id
    if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
        int tmpi;
        if (_sscanf(lineBuffer, "%d", &tmpi) == 1) {
            mappingType = tmpi;
        }
    }

    //get the mesh dimensions
    if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
        if (_sscanf(lineBuffer, "%d %d", &size[0], &size[1]) == 2) {
            mTempVertices = new CorrectionMeshVertex[size[0] * size[1]];
            mGeometries.warp.mNumberOfVertices =
                static_cast<unsigned int>(size[0] * size[1]);
        }
    }

    //check if everyting useful is set
    if (mappingType == -1 || size[0] == -1 || size[1] == -1) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Invalid data"
        );
        return false;
    }

    //get all data
    while (!feof(meshFile)) {
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            if (_sscanf(lineBuffer, "%f %f %f %f %f", &x, &y, &s, &t, &intensity) == 5) {
                mTempVertices[counter].x = x;
                mTempVertices[counter].y = y;
                mTempVertices[counter].s = s;
                mTempVertices[counter].t = t;

                mTempVertices[counter].r = intensity;
                mTempVertices[counter].g = intensity;
                mTempVertices[counter].b = intensity;
                mTempVertices[counter].a = 1.f;

                //if(counter <= 100)
                //    fprintf(stderr, "Adding vertex: %u %.3f %.3f %.3f %.3f %.3f\n", counter, x, y, s, t, intensity);

                counter++;
            }
        }
    }

    //generate indices
    std::vector<unsigned int> indices;
    int i0, i1, i2, i3;
    for (int c = 0; c < (size[0] - 1); c++) {
        for (int r = 0; r < (size[1] - 1); r++) {
            i0 = r * size[0] + c;
            i1 = r * size[0] + (c + 1);
            i2 = (r + 1) * size[0] + (c + 1);
            i3 = (r + 1) * size[0] + c;

            //fprintf(stderr, "Indexes: %u %u %u %u\n", i0, i1, i2, i3);

            /*

            3      2
            x____x
            |   /|
            |  / |
            | /  |
            |/   |
            x----x
            0      1

            */

            //triangle 1
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            //triangle 2
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    float aspect = sgct::Engine::instance()->getCurrentWindow().getAspectRatio() *
                   (parent.getSize().x / parent.getSize().y);
    
    for (unsigned int i = 0; i < mGeometries.warp.mNumberOfVertices; i++) {
        //convert to [0, 1] (normalize)
        mTempVertices[i].x /= aspect;
        mTempVertices[i].x = (mTempVertices[i].x + 1.f) / 2.f;
        mTempVertices[i].y = (mTempVertices[i].y + 1.f) / 2.f;
        
        //scale, re-position and convert to [-1, 1]
        mTempVertices[i].x =
            (mTempVertices[i].x * parent.getSize().x + parent.getPosition().x) * 2.f - 1.f;
        mTempVertices[i].y =
            (mTempVertices[i].y * parent.getSize().y + parent.getPosition().y) * 2.f - 1.f;

        //convert to viewport coordinates
        mTempVertices[i].s = mTempVertices[i].s * parent.getSize().x + parent.getPosition().x;
        mTempVertices[i].t = mTempVertices[i].t * parent.getSize().y + parent.getPosition().y;
    }

    //allocate and copy indices
    mGeometries.warp.mNumberOfIndices = static_cast<unsigned int>(indices.size());
    mTempIndices = new unsigned int[mGeometries.warp.mNumberOfIndices];
    memcpy(
        mTempIndices,
        indices.data(),
        mGeometries.warp.mNumberOfIndices * sizeof(unsigned int)
    );

    mGeometries.warp.mGeometryType = GL_TRIANGLES;
    createMesh(mGeometries.warp);

    //force regeneration of dome render quad
    FisheyeProjection* fishPrj = dynamic_cast<FisheyeProjection*>(
        parent.getNonLinearProjection()
    );
    if (fishPrj) {
        fishPrj->setIgnoreAspectRatio(true);
        fishPrj->update(glm::ivec2(1.f, 1.f));
    }

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        mGeometries.warp.mNumberOfVertices, mGeometries.warp.mNumberOfIndices
    );
    return true;
}

bool CorrectionMesh::readAndGenerateOBJMesh(const std::string& meshPath, Viewport& parent)
{
    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Info,
        "CorrectionMesh: Reading Maya Wavefront OBJ mesh data from '%s'\n",
        meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(meshPath.c_str(), "r");
    if (meshFile == nullptr) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    //variables
    int i0, i1, i2;
    unsigned int counter = 0;
    char lineBuffer[MaxLineLength];
    CorrectionMeshVertex tmpVert;
    std::vector<CorrectionMeshVertex> verts;
    std::vector<unsigned int> indices;

    //get all data
    while (!feof(meshFile)) {
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            if (_sscanf(lineBuffer, "v %f %f %*f", &tmpVert.x, &tmpVert.y) == 2) {
                tmpVert.r = 1.f;
                tmpVert.g = 1.f;
                tmpVert.b = 1.f;
                tmpVert.a = 1.f;

                verts.push_back(tmpVert);
            }
            else if (_sscanf(lineBuffer, "vt %f %f %*f", &tmpVert.s, &tmpVert.t) == 2) {
                if (counter < verts.size()) {
                    verts[counter].s = tmpVert.s;
                    verts[counter].t = tmpVert.t;
                }
                
                counter++;
            }
            else if (_sscanf(lineBuffer, "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &i0, &i1, &i2) == 3) {
                //indexes starts at 1 in OBJ
                indices.push_back(i0 - 1);
                indices.push_back(i1 - 1);
                indices.push_back(i2 - 1);
            }
        }
    }

    //sanity check
    if (counter != verts.size() || verts.size() == 0) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Vertex count doesn't match number of texture coordinates\n"
        );
        return false;
    }

    //allocate and copy indices
    mGeometries.warp.mNumberOfIndices = static_cast<unsigned int>(indices.size());
    mTempIndices = new unsigned int[mGeometries.warp.mNumberOfIndices];
    memcpy(
        mTempIndices,
        indices.data(),
        mGeometries.warp.mNumberOfIndices * sizeof(unsigned int)
    );

    mGeometries.warp.mNumberOfVertices = static_cast<unsigned int>(verts.size());
    mTempVertices = new CorrectionMeshVertex[mGeometries.warp.mNumberOfVertices];
    memcpy(
        mTempVertices,
        verts.data(),
        mGeometries.warp.mNumberOfVertices * sizeof(CorrectionMeshVertex)
    );

    mGeometries.warp.mGeometryType = GL_TRIANGLES;
    createMesh(mGeometries.warp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        mGeometries.warp.mNumberOfVertices,
        mGeometries.warp.mNumberOfIndices
    );
    return true;
}

bool CorrectionMesh::readAndGenerateMpcdiMesh(const std::string& meshPath,
                                              Viewport& parent)
{
    bool isReadingFile = meshPath.length() > 0;
    unsigned int srcIdx = 0;
    const unsigned char* srcBuff;
    size_t srcSize_bytes;
    const int MaxHeaderLineLength = 100;
    FILE* meshFile = nullptr;

    if (isReadingFile) {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Info,
            "CorrectionMesh: Reading MPCDI mesh (PFM format) data from '%s'\n",
            meshPath.c_str()
        );
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "CorrectionMesh: Failed to open warping mesh file\n"
            );
            return false;
        }
#else
        meshFile = fopen(meshPath.c_str(), "rb");
        if (meshFile == nullptr) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "CorrectionMesh: Failed to open warping mesh file\n"
            );
            return false;
        }
#endif
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Info,
            "CorrectionMesh: Reading MPCDI mesh (PFM format) from buffer\n",
            meshPath.c_str()
        );
        srcBuff = parent.mpcdiWarpMesh().data();
        srcSize_bytes = parent.mpcdiWarpMesh().size();
    }

    size_t retval;
    char headerChar;
    char headerBuffer[MaxHeaderLineLength];
    int index = 0;
    int nNewlines = 0;
    const int read3lines = 3;

    do {
        if (isReadingFile) {
#if (_MSC_VER >= 1400) //visual studio 2005 or later
            retval = fread_s(&headerChar, sizeof(char)*1, sizeof(char), 1, meshFile);
#else
            retval = fread(&headerChar, sizeof(char), 1, meshFile);
#endif
        }
        else {
            if (srcIdx == srcSize_bytes) {
                retval = -1;
            }
            else {
                headerChar = srcBuff[srcIdx++];
                retval = 1;
            }
        }
        if (retval != 1) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "CorrectionMesh: Error reading from file\n"
            );
            fclose(meshFile);
            return false;
        }
        headerBuffer[index++] = headerChar;
        if (headerChar == '\n') {
            nNewlines++;
        }
    } while (nNewlines < read3lines);

    char fileFormatHeader[2];
    unsigned int numberOfCols = 0;
    unsigned int numberOfRows = 0;
    float endiannessIndicator = 0;

#ifdef __WIN32__
    _sscanf(
        &headerBuffer[0],
        "%2c\n",
        &fileFormatHeader,
        static_cast<unsigned int>(sizeof(fileFormatHeader))
    );
    //Read header past the 2 character start
    _sscanf(&headerBuffer[3], "%d %d\n", &numberOfCols, &numberOfRows);

    constexpr auto nDigits = [](unsigned int i) {
        return static_cast<int>(ceil(log(static_cast<double>(i))));;
    };


    int indexForEndianness = 3 + nDigits(numberOfCols) + nDigits(numberOfRows) + 2;
    _sscanf(&headerBuffer[indexForEndianness], "%f\n", &endiannessIndicator);
#else
    if (_sscanf(headerBuffer, "%2c %d %d %f", fileFormatHeader,
                &numberOfCols, &numberOfRows, &endiannessIndicator) != 4)
    {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Invalid header syntax\n"
        );
        if (isReadingFile) {
            fclose(meshFile);
        }
        return false;
    }
#endif

    if (fileFormatHeader[0] != 'P' || fileFormatHeader[1] != 'F') {
        //The 'Pf' header is invalid because PFM grayscale type is not supported.
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh: Incorrect file type\n"
        );
    }
    int numCorrectionValues = numberOfCols * numberOfRows;
    float* correctionGridX = new float[numCorrectionValues];
    float* correctionGridY = new float[numCorrectionValues];
    float errorPosition;
    const int value32bit = 4;

    if (isReadingFile) {
#if (_MSC_VER >= 1400) //visual studio 2005 or later
 #define SGCT_READ fread_s
#else
 #define SGCT_READ fread
#endif
        for (int i = 0; i < numCorrectionValues; ++i) {
#ifdef __WIN32__
            retval = SGCT_READ(correctionGridX + i, numCorrectionValues, value32bit, 1, meshFile);
            retval = SGCT_READ(correctionGridY + i, numCorrectionValues, value32bit, 1, meshFile);
            //MPCDI uses the PFM format for correction grid. PFM format is designed for 3 RGB
            // values. However MPCDI substitutes Red for X correction, Green for Y
            // correction, and Blue for correction error. This will be NaN for no error value
            retval = SGCT_READ(&errorPosition, numCorrectionValues, value32bit, 1, meshFile);
#else
            retval = SGCT_READ(correctionGridX + i, value32bit, 1, meshFile);
            retval = SGCT_READ(correctionGridY + i, value32bit, 1, meshFile);
            retval = SGCT_READ(&errorPosition,      value32bit, 1, meshFile);
#endif
        }
        fclose(meshFile);
        if (retval != value32bit) {
            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Error,
                "CorrectionMesh: Error reading all correction values!\n"
            );
            return false;
        }
    }
    else {
        bool readErr = false;
        for (int i = 0; i < numCorrectionValues; ++i) {
//#define TEST_FLAT_MESH

#ifdef TEST_FLAT_MESH
            correctionGridX[i] = (float)(i%32) / 32.0;
            correctionGridY[i] = 1.0 - (float)i * 32.0 / 32.0;
#else
            if (!readMeshBuffer(&correctionGridX[i], srcIdx, srcBuff, srcSize_bytes, value32bit)) {
                readErr = true;
            }
            else if (!readMeshBuffer(&correctionGridY[i], srcIdx, srcBuff, srcSize_bytes, value32bit)) {
                readErr = true;
            }
            else if (!readMeshBuffer(&errorPosition, srcIdx, srcBuff, srcSize_bytes, value32bit)) {
                readErr = true;
            }

            if (readErr) {
                sgct::MessageHandler::instance()->print(
                    sgct::MessageHandler::Level::Error,
                    "CorrectionMesh: Error reading mpcdi correction value at index %d\n",
                    i
                );
            }
#endif
        }
    }

    int gridIndex_column, gridIndex_row;
    float* smoothPos_x = new float[numCorrectionValues];
    float* smoothPos_y = new float[numCorrectionValues];
    float* warpedPos_x = new float[numCorrectionValues];
    float* warpedPos_y = new float[numCorrectionValues];

    for (int i = 0; i < numCorrectionValues; ++i) {
        gridIndex_column = i % numberOfCols;
        gridIndex_row = i / numberOfCols;
        //Compute XY positions for each point based on a normalized 0,0 to 1,1 grid,
        // add the correction offsets to each warp point
        smoothPos_x[i] = static_cast<float>(gridIndex_column) /
                         static_cast<float>(numberOfCols - 1);
        //Reverse the y position because the values from pfm file are given in raster-scan
        // order, which is left to right but starts at upper-left rather than lower-left.
        smoothPos_y[i] = 1.f - (static_cast<float>(gridIndex_row) /
                         static_cast<float>(numberOfRows - 1));
        warpedPos_x[i] = smoothPos_x[i] + correctionGridX[i];
        warpedPos_y[i] = smoothPos_y[i] + correctionGridY[i];
    }

#ifdef NORMALIZE_CORRECTION_MESH
    float maxX = *std::max_element(warpedPos_x, warpedPos_x + numCorrectionValues);
    float minX = *std::min_element(warpedPos_x, warpedPos_x + numCorrectionValues);
    float scaleRangeX = maxX - minX;
    float maxY = *std::max_element(warpedPos_y, warpedPos_y + numCorrectionValues);
    float minY = *std::min_element(warpedPos_y, warpedPos_y + numCorrectionValues);
    float scaleRangeY = maxY - minY;
    float scaleFactor = (scaleRangeX >= scaleRangeY) ? scaleRangeX : scaleRangeY;
    //Scale all positions to fit within 0,0 to 1,1
    for (int i = 0; i < numCorrectionValues; ++i) {
        warpedPos_x[i] = (warpedPos_x[i] - minX) / scaleFactor;
        warpedPos_y[i] = (warpedPos_y[i] - minY) / scaleFactor;
    }
#endif //NORMALIZE_CORRECTION_MESH

    CorrectionMeshVertex vertex;
    std::vector<CorrectionMeshVertex> vertices;
    //init to max intensity (opaque white)
    vertex.r = 1.f;
    vertex.g = 1.f;
    vertex.b = 1.f;
    vertex.a = 1.f;
    for (int i = 0; i < numCorrectionValues; ++i) {
        vertex.s = smoothPos_x[i];
        vertex.t = smoothPos_y[i];
        //scale to viewport coordinates
        vertex.x = 2.f * warpedPos_x[i] - 1.f;
        vertex.y = 2.f * warpedPos_y[i] - 1.f;
        vertices.push_back(vertex);
    }
    delete[] warpedPos_x;
    delete[] warpedPos_y;
    delete[] smoothPos_x;
    delete[] smoothPos_y;

    //copy vertices
    unsigned int numberOfVertices = numberOfCols * numberOfRows;
    mTempVertices = new CorrectionMeshVertex[numberOfVertices];
    memcpy(
        mTempVertices,
        vertices.data(),
        numberOfVertices * sizeof(CorrectionMeshVertex)
    );
    mGeometries.warp.mNumberOfVertices = numberOfVertices;
    vertices.clear();

    std::vector<unsigned int> indices;
    unsigned int i0, i1, i2, i3;
    for (unsigned int c = 0; c < (numberOfCols - 1); c++) {
        for (unsigned int r = 0; r < (numberOfRows - 1); r++) {
            i0 = r * numberOfCols + c;
            i1 = r * numberOfCols + (c + 1);
            i2 = (r + 1) * numberOfCols + (c + 1);
            i3 = (r + 1) * numberOfCols + c;

            //fprintf(stderr, "Indexes: %u %u %u %u\n", i0, i1, i2, i3);

            /*

            3      2
             x____x
             |   /|
             |  / |
             | /  |
             |/   |
             x----x
            0      1

            */

            //triangle 1
            indices.push_back(i0);
            indices.push_back(i1);
            indices.push_back(i2);

            //triangle 2
            indices.push_back(i0);
            indices.push_back(i2);
            indices.push_back(i3);
        }
    }

    //allocate and copy indices
    mGeometries.warp.mNumberOfIndices = static_cast<unsigned int>(indices.size());
    mTempIndices = new unsigned int[mGeometries.warp.mNumberOfIndices];
    memcpy(
        mTempIndices,
        indices.data(),
        mGeometries.warp.mNumberOfIndices * sizeof(unsigned int)
    );
    indices.clear();

    mGeometries.warp.mGeometryType = GL_TRIANGLES;

    createMesh(mGeometries.warp);

    sgct::MessageHandler::instance()->print(
        sgct::MessageHandler::Level::Debug,
        "CorrectionMesh: Mpcdi Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        mGeometries.warp.mNumberOfVertices,
        mGeometries.warp.mNumberOfIndices
    );

    return true;
}

void CorrectionMesh::setupSimpleMesh(CorrectionMeshGeometry& geomPtr, Viewport& parent) {
    unsigned int numberOfVertices = 4;
    unsigned int numberOfIndices = 4;
    
    geomPtr.mNumberOfVertices = numberOfVertices;
    geomPtr.mNumberOfIndices = numberOfIndices;
    geomPtr.mGeometryType = GL_TRIANGLE_STRIP;

    mTempVertices = new CorrectionMeshVertex[ numberOfVertices ];
    memset(mTempVertices, 0, numberOfVertices * sizeof(CorrectionMeshVertex));

    mTempIndices = new unsigned int[numberOfIndices];
    memset(mTempIndices, 0, numberOfIndices * sizeof(unsigned int));

    mTempIndices[0] = 0;
    mTempIndices[1] = 3;
    mTempIndices[2] = 1;
    mTempIndices[3] = 2;

    mTempVertices[0].r = 1.f;
    mTempVertices[0].g = 1.f;
    mTempVertices[0].b = 1.f;
    mTempVertices[0].a = 1.f;
    mTempVertices[0].s = 0.f * parent.getSize().x + parent.getPosition().x;
    mTempVertices[0].t = 0.f * parent.getSize().y + parent.getPosition().y;
    mTempVertices[0].x = 2.f * (0.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    mTempVertices[0].y = 2.f * (0.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    mTempVertices[1].r = 1.f;
    mTempVertices[1].g = 1.f;
    mTempVertices[1].b = 1.f;
    mTempVertices[1].a = 1.f;
    mTempVertices[1].s = 1.f * parent.getSize().x + parent.getPosition().x;
    mTempVertices[1].t = 0.f * parent.getSize().y + parent.getPosition().y;
    mTempVertices[1].x = 2.f * (1.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    mTempVertices[1].y = 2.f * (0.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    mTempVertices[2].r = 1.f;
    mTempVertices[2].g = 1.f;
    mTempVertices[2].b = 1.f;
    mTempVertices[2].a = 1.f;
    mTempVertices[2].s = 1.f * parent.getSize().x + parent.getPosition().x;
    mTempVertices[2].t = 1.f * parent.getSize().y + parent.getPosition().y;
    mTempVertices[2].x = 2.f * (1.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    mTempVertices[2].y = 2.f * (1.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    mTempVertices[3].r = 1.f;
    mTempVertices[3].g = 1.f;
    mTempVertices[3].b = 1.f;
    mTempVertices[3].a = 1.f;
    mTempVertices[3].s = 0.f * parent.getSize().x + parent.getPosition().x;
    mTempVertices[3].t = 1.f * parent.getSize().y + parent.getPosition().y;
    mTempVertices[3].x = 2.f * (0.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    mTempVertices[3].y = 2.f * (1.f * parent.getSize().y + parent.getPosition().y) - 1.f;
}

void CorrectionMesh::setupMaskMesh(Viewport& parent, bool flipX, bool flipY) {
    unsigned int numberOfVertices = 4;
    unsigned int numberOfIndices = 4;

    mGeometries.mask.mNumberOfVertices = numberOfVertices;
    mGeometries.mask.mNumberOfIndices = numberOfIndices;
    mGeometries.mask.mGeometryType = GL_TRIANGLE_STRIP;

    mTempVertices = new CorrectionMeshVertex[numberOfVertices];
    memset(mTempVertices, 0, numberOfVertices * sizeof(CorrectionMeshVertex));

    mTempIndices = new unsigned int[numberOfIndices];
    memset(mTempIndices, 0, numberOfIndices * sizeof(unsigned int));

    mTempIndices[0] = 0;
    mTempIndices[1] = 3;
    mTempIndices[2] = 1;
    mTempIndices[3] = 2;

    mTempVertices[0].r = 1.f;
    mTempVertices[0].g = 1.f;
    mTempVertices[0].b = 1.f;
    mTempVertices[0].a = 1.f;
    mTempVertices[0].s = flipX ? 1.f : 0.f;
    mTempVertices[0].t = flipY ? 1.f : 0.f;
    mTempVertices[0].x = 2.f * (0.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    mTempVertices[0].y = 2.f * (0.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    mTempVertices[1].r = 1.f;
    mTempVertices[1].g = 1.f;
    mTempVertices[1].b = 1.f;
    mTempVertices[1].a = 1.f;
    mTempVertices[1].s = flipX ? 0.f : 1.f;
    mTempVertices[1].t = flipY ? 1.F : 0.f;
    mTempVertices[1].x = 2.f * (1.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    mTempVertices[1].y = 2.f * (0.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    mTempVertices[2].r = 1.f;
    mTempVertices[2].g = 1.f;
    mTempVertices[2].b = 1.f;
    mTempVertices[2].a = 1.f;
    mTempVertices[2].s = flipX ? 0.f : 1.f;
    mTempVertices[2].t = flipY ? 0.f : 1.f;
    mTempVertices[2].x = 2.f * (1.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    mTempVertices[2].y = 2.f * (1.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    mTempVertices[3].r = 1.f;
    mTempVertices[3].g = 1.f;
    mTempVertices[3].b = 1.f;
    mTempVertices[3].a = 1.f;
    mTempVertices[3].s = flipX ? 1.f : 0.f;
    mTempVertices[3].t = flipY ? 0.f : 1.f;
    mTempVertices[3].x = 2.f * (0.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    mTempVertices[3].y = 2.f * (1.f * parent.getSize().y + parent.getPosition().y) - 1.f;
}

void CorrectionMesh::createMesh(CorrectionMeshGeometry& geomPtr) {
    if (ClusterManager::instance()->getMeshImplementation() ==
        ClusterManager::MeshImplementation::BufferObjects)
    {
        if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
            glGenVertexArrays(1, &(geomPtr.mArrayMeshData));
            glBindVertexArray(geomPtr.mArrayMeshData);

            sgct::MessageHandler::instance()->print(
                sgct::MessageHandler::Level::Debug,
                "CorrectionMesh: Generating VAO: %d\n",
                geomPtr.mArrayMeshData
            );
        }

        glGenBuffers(1, &(geomPtr.mVertexMeshData));
        glGenBuffers(1, &(geomPtr.mIndexMeshData));
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Generating VBOs: %d %d\n",
            geomPtr.mVertexMeshData, geomPtr.mIndexMeshData
        );

        glBindBuffer(GL_ARRAY_BUFFER, geomPtr.mVertexMeshData);
        glBufferData(
            GL_ARRAY_BUFFER,
            geomPtr.mNumberOfVertices * sizeof(CorrectionMeshVertex),
            &mTempVertices[0],
            GL_STATIC_DRAW
        );

        if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(
                0, // The attribute we want to configure
                2,                           // size
                GL_FLOAT,                    // type
                GL_FALSE,                    // normalized?
                sizeof(CorrectionMeshVertex),// stride
                reinterpret_cast<void*>(0)   // array buffer offset
            );

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(
                1, // The attribute we want to configure
                2,                           // size
                GL_FLOAT,                    // type
                GL_FALSE,                    // normalized?
                sizeof(CorrectionMeshVertex),// stride
                reinterpret_cast<void*>(8)   // array buffer offset
            );

            glEnableVertexAttribArray(2);
            glVertexAttribPointer(
                2, // The attribute we want to configure
                4,                           // size
                GL_FLOAT,                    // type
                GL_FALSE,                    // normalized?
                sizeof(CorrectionMeshVertex),// stride
                reinterpret_cast<void*>(16)  // array buffer offset
            );
        }

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geomPtr.mIndexMeshData);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            geomPtr.mNumberOfIndices * sizeof(unsigned int),
            &mTempIndices[0],
            GL_STATIC_DRAW
        );

        //unbind
        if (!sgct::Engine::instance()->isOGLPipelineFixed()) {
            glBindVertexArray(0);
        }
        
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    else {
        //display lists
        geomPtr.mVertexMeshData = glGenLists(1);
        glNewList(geomPtr.mVertexMeshData, GL_COMPILE);

        glBegin(geomPtr.mGeometryType);
        CorrectionMeshVertex vertex;
        
        for (unsigned int i = 0; i < geomPtr.mNumberOfIndices; i++) {
            vertex = mTempVertices[mTempIndices[i]];

            glColor4f(vertex.r, vertex.g, vertex.b, vertex.a);
            glTexCoord2f(vertex.s, vertex.t);
            glVertex2f(vertex.x, vertex.y);
        }
        glEnd();

        glEndList();
        
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Debug,
            "CorrectionMesh: Generating display list: %d\n", geomPtr.mVertexMeshData
        );
    }
}

void CorrectionMesh::exportMesh(const std::string& exportMeshPath) {
    if (mGeometries.warp.mGeometryType != GL_TRIANGLES &&
        mGeometries.warp.mGeometryType != GL_TRIANGLE_STRIP)
    {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh error: Failed to export '%s'. Geometry type is not supported!\n",
            exportMeshPath.c_str()
        );
        return;
    }
    
    std::ofstream file;
    file.open(exportMeshPath, std::ios::out);
    if (file.is_open()) {
        file << std::fixed;
        file << std::setprecision(6);
        
        file << "# SGCT warping mesh\n";
        file << "# Number of vertices: " << mGeometries.warp.mNumberOfVertices << "\n";
        
        //export vertices
        for (unsigned int i = 0; i < mGeometries.warp.mNumberOfVertices; i++) {
            file << "v " << mTempVertices[i].x << " " << mTempVertices[i].y << " 0\n";
        }

        //export texture coords
        for (unsigned int i = 0; i < mGeometries.warp.mNumberOfVertices; i++) {
            file << "vt " << mTempVertices[i].s << " " << mTempVertices[i].t << " 0\n";
        }

        //export generated normals
        for (unsigned int i = 0; i < mGeometries.warp.mNumberOfVertices; i++) {
            file << "vn 0 0 1\n";
        }

        file << "# Number of faces: " << mGeometries.warp.mNumberOfIndices/3 << "\n";

        //export face indices
        if (mGeometries.warp.mGeometryType == GL_TRIANGLES) {
            for (unsigned int i = 0; i < mGeometries.warp.mNumberOfIndices; i += 3) {
                file << "f " << mTempIndices[i] + 1 << "/" << mTempIndices[i] + 1 <<
                        "/" << mTempIndices[i] + 1 << " ";
                file << mTempIndices[i + 1] + 1 << "/" << mTempIndices[i + 1] + 1 <<
                        "/" << mTempIndices[i + 1] + 1 << " ";
                file << mTempIndices[i + 2] + 1 << "/" << mTempIndices[i + 2] + 1 <<
                        "/" << mTempIndices[i + 2] + 1 << "\n";
            }
        }
        else {
            //trangle strip

            //first base triangle
            file << "f " << mTempIndices[0] + 1 << "/" << mTempIndices[0] + 1 <<
                    "/" << mTempIndices[0] + 1 << " ";
            file << mTempIndices[1] + 1 << "/" << mTempIndices[1] + 1 <<
                    "/" << mTempIndices[1] + 1 << " ";
            file << mTempIndices[2] + 1 << "/" << mTempIndices[2] + 1 <<
                    "/" << mTempIndices[2] + 1 << "\n";

            for (unsigned int i = 2; i < mGeometries.warp.mNumberOfIndices; i++) {
                file << "f " << mTempIndices[i] + 1 << "/" << mTempIndices[i] + 1 <<
                        "/" << mTempIndices[i] + 1 << " ";
                file << mTempIndices[i - 1] + 1 << "/" << mTempIndices[i - 1] + 1 <<
                        "/" << mTempIndices[i - 1] + 1 << " ";
                file << mTempIndices[i - 2] + 1 << "/" << mTempIndices[i - 2] + 1 <<
                        "/" << mTempIndices[i - 2] + 1 << "\n";
            }
        }

        file.close();

        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Info,
            "CorrectionMesh: Mesh '%s' exported successfully\n",
            exportMeshPath.c_str()
        );
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Error,
            "CorrectionMesh error: Failed to export '%s'\n",
            exportMeshPath.c_str()
        );
    }
}

void CorrectionMesh::cleanUp() {
    delete[] mTempVertices;
    mTempVertices = nullptr;
    delete[] mTempIndices;
    mTempIndices = nullptr;
}

void CorrectionMesh::render(const CorrectionMeshGeometry& mt) const {
    if (sgct::SGCTSettings::instance()->getShowWarpingWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (ClusterManager::instance()->getMeshImplementation() ==
        ClusterManager::MeshImplementation::BufferObjects)
    {
        if (sgct::Engine::instance()->isOGLPipelineFixed()) {
            glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

            glEnableClientState(GL_VERTEX_ARRAY);
            glClientActiveTexture(GL_TEXTURE0);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            glBindBuffer(GL_ARRAY_BUFFER, mt.mVertexMeshData);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mt.mIndexMeshData);

            glVertexPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(0));
            glTexCoordPointer(2, GL_FLOAT, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(8));
            glColorPointer(4, GL_FLOAT, sizeof(CorrectionMeshVertex), reinterpret_cast<void*>(16));

            glDrawElements(mt.mGeometryType, mt.mNumberOfIndices, GL_UNSIGNED_INT, NULL);

            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            glPopClientAttrib();
        }
        else {
            glBindVertexArray(mt.mArrayMeshData);
            glDrawElements(
                mt.mGeometryType,
                mt.mNumberOfIndices,
                GL_UNSIGNED_INT,
                nullptr
            );
            glBindVertexArray(0);
        }
    }
    else {
        glCallList(mt.mVertexMeshData);
    }

    if (sgct::SGCTSettings::instance()->getShowWarpingWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void CorrectionMesh::renderQuadMesh() const {
    render(mGeometries.quad);
}

void CorrectionMesh::renderWarpMesh() const {
    render(mGeometries.warp);
}

void CorrectionMesh::renderMaskMesh() const {
    render(mGeometries.mask);
}

CorrectionMesh::MeshHint CorrectionMesh::parseHint(const std::string& hintStr) {
    if (hintStr.empty()) {
        return MeshHint::None;
    }
    
    //transform to lowercase
    std::string str(hintStr);
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    sgct_core::CorrectionMesh::MeshHint hint = MeshHint::None;
    if (str == "domeprojection") {
        hint = MeshHint::DomeProjection;
    }
    else if (str == "scalable") {
        hint = MeshHint::Scaleable;
    }
    else if (str == "sciss") {
        hint = MeshHint::Sciss;
    }
    else if (str == "simcad") {
        hint = MeshHint::SimCad;
    }
    else if (str == "skyskan") {
        hint = MeshHint::SkySkan;
    }
    else if (str == "mpcdi") {
        hint = MeshHint::Mpcdi;
    }
    else {
        sgct::MessageHandler::instance()->print(
            sgct::MessageHandler::Level::Warning,
            "CorrectionMesh: hint '%s' is invalid\n",
            hintStr.c_str()
        );
    }

    return hint;
}

} // namespace sgct_core
