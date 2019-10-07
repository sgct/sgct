/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/correctionmesh.h>

#include <sgct/clustermanager.h>
#include <sgct/engine.h>
#include <sgct/messagehandler.h>
#include <sgct/settings.h>
#include <sgct/viewport.h>
#include <sgct/user.h>
#include <sgct/helpers/stringfunctions.h>
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <tinyxml2.h>

#if (_MSC_VER >= 1400)
    #define _sscanf sscanf_s
#else
    #define _sscanf sscanf
#endif

//#define CONVERT_SCISS_TO_DOMEPROJECTION
//#define CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC

namespace {
    constexpr const int MaxLineLength = 1024;

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

} // namespace

namespace sgct::core {

CorrectionMesh::CorrectionMeshGeometry::~CorrectionMeshGeometry() {
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMeshGeometry: Releasing correction mesh OpenGL data\n"
    );

    if (ClusterManager::instance()->getMeshImplementation() ==
        ClusterManager::MeshImplementation::DisplayList)
    {
        glDeleteLists(vertexData, 1);
    }
    else {
        glDeleteVertexArrays(1, &arrayData);
        glDeleteBuffers(1, &vertexData);
        glDeleteBuffers(1, &indexData);
    }
}

bool CorrectionMesh::readAndGenerateMesh(std::string path, Viewport& parent, Hint hint) {
    // generate unwarped mask
    {
        Buffer buf = setupSimpleMesh(_quadGeometry, parent);
        createMesh(_quadGeometry, buf.vertices, buf.indices);
    }
    
    // generate unwarped mesh for mask
    if (parent.hasBlendMaskTexture() || parent.hasBlackLevelMaskTexture()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Creating mask mesh\n"
        );
        
        bool flipX = false;
        bool flipY = false;
        Buffer buf = setupMaskMesh(parent, flipX, flipY);
        createMesh(_maskGeometry, buf.vertices, buf.indices);
    }

    // fallback if no mesh is provided
    if (path.empty()) {
        Buffer buf = setupSimpleMesh(_warpGeometry, parent);
        createMesh(_warpGeometry, buf.vertices, buf.indices);

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Empty mesh path\n"
        );
        return false;
    }
    
    // transform to lowercase
    std::string pathLower(path);
    std::transform(
        path.begin(),
        path.end(),
        pathLower.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );

    bool loadStatus = false;

    // find a suitable format
    if (pathLower.find(".sgc") != std::string::npos) {
        loadStatus = generateScissMesh(path, parent);
    }
    else if (pathLower.find(".ol") != std::string::npos) {
        loadStatus = generateScalableMesh(path, parent);
    }
    else if (pathLower.find(".skyskan") != std::string::npos) {
        loadStatus = generateSkySkanMesh(path, parent);
    }
    else if (pathLower.find(".txt") != std::string::npos) {
        // default for this suffix
        if (hint == Hint::None || hint == Hint::SkySkan) {
            loadStatus = generateSkySkanMesh(path, parent);
        }
    }
    else if (pathLower.find(".csv") != std::string::npos) {
        // default for this suffix
        if (hint == Hint::None || hint == Hint::DomeProjection) {
            loadStatus = generateDomeProjectionMesh(path, parent);
        }
    }
    else if (pathLower.find(".data") != std::string::npos) {
        // default for this suffix
        if (hint == Hint::None || hint == Hint::PaulBourke) {
            loadStatus = generatePaulBourkeMesh(path, parent);
        }
    }
    else if (pathLower.find(".obj") != std::string::npos) {
        // default for this suffix
        if (hint == Hint::None || hint == Hint::Obj) {
            loadStatus = generateOBJMesh(path);
        }
    }
    else if (pathLower.find(".mpcdi") != std::string::npos) {
        loadStatus = generateMpcdiMesh("", parent);
    }
    else if (pathLower.find(".simcad") != std::string::npos) {
        // default for this suffix
        if (hint == Hint::None || hint == Hint::SimCad) {
            loadStatus = generateSimCADMesh(path, parent);
        }
    }
    
    if (!loadStatus) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh error: Loading mesh '%s' failed\n", path.c_str()
        );

        Buffer buf = setupSimpleMesh(_warpGeometry, parent);
        createMesh(_warpGeometry, buf.vertices, buf.indices);
        return false;
    }

    return true;
}

bool CorrectionMesh::generateDomeProjectionMesh(const std::string& meshPath,
                                                const Viewport& parent)
{
    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Reading DomeProjection mesh data from '%s'\n", meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    Buffer buf;

    unsigned int numberOfCols = 0;
    unsigned int numberOfRows = 0;
    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            float x;
            float y;
            float u;
            float v;
            unsigned int col;
            unsigned int row;

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

                buf.vertices.push_back(std::move(vertex));
            }
        }
    }

    fclose(meshFile);

    // add one to actually store the dimensions instread of largest index
    numberOfCols++;
    numberOfRows++;

    _warpGeometry.nVertices = numberOfCols * numberOfRows;

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

            buf.indices.push_back(i0);
            buf.indices.push_back(i1);
            buf.indices.push_back(i2);

            buf.indices.push_back(i0);
            buf.indices.push_back(i2);
            buf.indices.push_back(i3);
        }
    }

    _warpGeometry.nIndices = static_cast<unsigned int>(buf.indices.size());
    _warpGeometry.geometryType = GL_TRIANGLES;

    createMesh(_warpGeometry, buf.vertices, buf.indices);

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        _warpGeometry.nVertices, _warpGeometry.nIndices
    );
    
    if (Settings::instance()->getExportWarpingMeshes()) {
        const size_t found = meshPath.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = meshPath.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename), buf.vertices, buf.indices);
        }
    }

    return true;
}

bool CorrectionMesh::generateScalableMesh(const std::string& path, const Viewport& parent)
{
    Buffer buf;

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Reading scalable mesh data from '%s'\n", path.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, path.c_str(), "r") != 0 || !meshFile) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
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

    double leftOrtho = 0.0;
    double rightOrtho = 0.0;
    double bottomOrtho = 0.0;
    double topOrtho = 0.0;
    glm::ivec2 resolution;

    unsigned int a, b, c;
    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];

        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            float x, y, s, t;
            unsigned int intensity;

            if (_sscanf(lineBuffer, "%f %f %u %f %f", &x, &y, &intensity, &s, &t) == 5) {
                if (!buf.vertices.empty() && resolution.x != 0 && resolution.y != 0) {
                    CorrectionMeshVertex& vertex = buf.vertices[numOfVerticesRead];
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
                if (!buf.indices.empty()) {
                    buf.indices[numOfFacesRead * 3] = a;
                    buf.indices[numOfFacesRead * 3 + 1] = b;
                    buf.indices[numOfFacesRead * 3 + 2] = c;
                }

                numOfFacesRead++;
            }
            else {
                char tmpBuf[16];
                tmpBuf[0] = '\0';
                double tmpD = 0.0;
                unsigned int tmpUI = 0;

                if (_sscanf(lineBuffer, "VERTICES %u", &numberOfVertices) == 1) {
                    buf.vertices.resize(numberOfVertices);
                    std::fill(
                        buf.vertices.begin(),
                        buf.vertices.end(),
                        CorrectionMeshVertex()
                    );
                }

                else if (_sscanf(lineBuffer, "FACES %u", &numberOfFaces) == 1) {
                    numberOfIndices = numberOfFaces * 3;
                    buf.indices.resize(numberOfIndices);
                    std::fill(buf.indices.begin(), buf.indices.end(), 0);
                }
                else if (_sscanf(lineBuffer, "ORTHO_%s %lf", tmpBuf, 16, &tmpD) == 2) {
                    if (strcmp(tmpBuf, "LEFT") == 0) {
                        leftOrtho = tmpD;
                    }
                    else if (strcmp(tmpBuf, "RIGHT") == 0) {
                        rightOrtho = tmpD;
                    }
                    else if (strcmp(tmpBuf, "BOTTOM") == 0) {
                        bottomOrtho = tmpD;
                    }
                    else if (strcmp(tmpBuf, "TOP") == 0) {
                        topOrtho = tmpD;
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Incorrect mesh data geometry"
        );
        return false;
    }

    // normalize
    for (unsigned int i = 0; i < numberOfVertices; i++) {
        const float xMin = static_cast<float>(leftOrtho);
        const float xMax = static_cast<float>(rightOrtho);
        const float yMin = static_cast<float>(bottomOrtho);
        const float yMax = static_cast<float>(topOrtho);

        // normalize between 0.0 and 1.0
        const float xVal = (buf.vertices[i].x - xMin) / (xMax - xMin);
        const float yVal = (buf.vertices[i].y - yMin) / (yMax - yMin);

        // normalize between -1.0 to 1.0
        buf.vertices[i].x = xVal * 2.f - 1.f;
        buf.vertices[i].y = yVal * 2.f - 1.f;
    }

    fclose(meshFile);

    _warpGeometry.nVertices = numberOfVertices;
    _warpGeometry.nIndices = numberOfIndices;
    _warpGeometry.geometryType = GL_TRIANGLES;

    createMesh(_warpGeometry, buf.vertices, buf.indices);

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Correction mesh read successfully! Vertices=%u, Faces=%u\n",
        numOfVerticesRead, numOfFacesRead
    );

    if (Settings::instance()->getExportWarpingMeshes()) {
        const size_t found = path.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = path.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename), buf.vertices, buf.indices);
        }
    }


    return true;
}

bool CorrectionMesh::generateScissMesh(const std::string& path, Viewport& parent) {
    Buffer buf;

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Reading sciss mesh data from '%s'\n", path.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, path.c_str(), "rb") != 0 || !meshFile) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(path.c_str(), "rb");
    if (meshFile == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    size_t ret;
    unsigned int numberOfVertices = 0;
    unsigned int numberOfIndices = 0;

    char fileID[3];
#if (_MSC_VER >= 1400)
    ret = fread_s(fileID, sizeof(char) * 3, sizeof(char), 3, meshFile);
#else
    ret = fread(fileID, sizeof(char), 3, meshFile);
#endif

    // check fileID
    if (fileID[0] != 'S' || fileID[1] != 'G' || fileID[2] != 'C' || ret != 3) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Incorrect file id\n"
        );
        fclose(meshFile);
        return false;
    }

    // read file version
    uint8_t fileVersion;
#if (_MSC_VER >= 1400)
    ret = fread_s(&fileVersion, sizeof(uint8_t), sizeof(uint8_t), 1, meshFile);
#else
    ret = fread(&fileVersion, sizeof(uint8_t), 1, meshFile);
#endif
    if (ret != 1) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: file version %u\n", fileVersion
        );
    }

    // read mapping type
    unsigned int type;
#if (_MSC_VER >= 1400)
    ret = fread_s(&type, sizeof(unsigned int), sizeof(unsigned int), 1, meshFile);
#else
    ret = fread(&type, sizeof(unsigned int), 1, meshFile);
#endif
    if (ret != 1) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Mapping type = %s (%u)\n",
            type == 0 ? "planar" : "cube", type
        );
    }

    // read viewdata
    SCISSViewData viewData;
#if (_MSC_VER >= 1400)
    ret = fread_s(&viewData, sizeof(SCISSViewData), sizeof(SCISSViewData), 1, meshFile);
#else
    ret = fread(&viewData, sizeof(SCISSViewData), 1, meshFile);
#endif
    double yaw, pitch, roll;
    if (ret != 1) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }
    else {
        const double x = static_cast<double>(viewData.qx);
        const double y = static_cast<double>(viewData.qy);
        const double z = static_cast<double>(viewData.qz);
        const double w = static_cast<double>(viewData.qw);
        
        // @TODO (abock, 2019-08-30): It seems weird that we are mixing the euler angles
        // from x,y,z to y,x,z. Maybe something related to left-handed and right-handed
        // coordinate systems?
        glm::dvec3 angles = glm::degrees(glm::eulerAngles(glm::dquat(w, y, x, z)));
        yaw = -angles.x;
        pitch = angles.y;
        roll = -angles.z;
        
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Rotation quat = [%f %f %f %f]\n"
            "yaw = %lf, pitch = %lf, roll = %lf\n",
            viewData.qx, viewData.qy, viewData.qz, viewData.qw, yaw, pitch, roll);

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Position = [%f %f %f]\n", viewData.x, viewData.y, viewData.z
        );

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: FOV up = %f\n", viewData.fovUp
        );

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: FOV down = %f\n", viewData.fovDown
        );

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: FOV left = %f\n", viewData.fovLeft
        );

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: FOV right = %f\n", viewData.fovRight
        );
    }

    // read number of vertices
    unsigned int size[2];
#if (_MSC_VER >= 1400)
    ret = fread_s(size, sizeof(unsigned int) * 2, sizeof(unsigned int), 2, meshFile);
#else
    ret = fread(size, sizeof(unsigned int), 2, meshFile);
#endif
    if (ret != 2) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }

    if (fileVersion == 2) {
        numberOfVertices = size[1];
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Number of vertices = %u\n", numberOfVertices
        );
    }
    else {
        numberOfVertices = size[0] * size[1];
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Number of vertices = %u (%ux%u)\n",
            numberOfVertices, size[0], size[1]
        );
    }
    // read vertices
    std::vector<SCISSTexturedVertex> texturedVertexList(numberOfVertices);
#if (_MSC_VER >= 1400)
    ret = fread_s(
        texturedVertexList.data(),
        sizeof(SCISSTexturedVertex) * numberOfVertices,
        sizeof(SCISSTexturedVertex),
        numberOfVertices,
        meshFile
    );
#else
    ret = fread(
        texturedVertexList.data(),
        sizeof(SCISSTexturedVertex),
        numberOfVertices,
        meshFile
    );
#endif
    if (ret != numberOfVertices) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }

    // read number of indices
#if (_MSC_VER >= 1400)
    ret = fread_s(
        &numberOfIndices,
        sizeof(unsigned int),
        sizeof(unsigned int),
        1,
        meshFile
    );
#else
    ret = fread(&numberOfIndices, sizeof(unsigned int), 1, meshFile);
#endif

    if (ret != 1) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Error parsing file\n"
        );
        fclose(meshFile);
        return false;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Number of indices = %u\n", numberOfIndices
        );
    }

    // read faces
    if (numberOfIndices > 0) {
        buf.indices.resize(numberOfIndices);
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        ret = fread_s(
            buf.indices.data(),
            sizeof(unsigned int) * numberOfIndices,
            sizeof(unsigned int),
            numberOfIndices,
            meshFile
        );
#else
        ret = fread(
            buf.indices.data(),
            sizeof(unsigned int),
            numberOfIndices,
            meshFile
        );
#endif
        if (ret != numberOfIndices) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "CorrectionMesh: Error parsing file\n"
            );
            fclose(meshFile);
            return false;
        }
    }

    fclose(meshFile);

    parent.getUser().setPos(glm::vec3(viewData.x, viewData.y, viewData.z));

    parent.setViewPlaneCoordsUsingFOVs(
        viewData.fovUp,
        viewData.fovDown,
        viewData.fovLeft,
        viewData.fovRight,
        glm::quat(viewData.qw, viewData.qx, viewData.qy, viewData.qz)
    );

    Engine::instance()->updateFrustums();

    buf.vertices.resize(numberOfVertices);
    for (unsigned int i = 0; i < numberOfVertices; i++) {
        CorrectionMeshVertex& vertex = buf.vertices[i];
        SCISSTexturedVertex& scissVertex = texturedVertexList[i];

        vertex.r = 1.f;
        vertex.g = 1.f;
        vertex.b = 1.f;
        vertex.a = 1.f;

        scissVertex.x = glm::clamp(scissVertex.x, 0.f, 1.f);
        scissVertex.y = glm::clamp(scissVertex.y, 0.f, 1.f);
        scissVertex.tx = glm::clamp(scissVertex.tx, 0.f, 1.f);
        scissVertex.ty = glm::clamp(scissVertex.ty, 0.f, 1.f);

        const glm::vec2& s = parent.getSize();
        const glm::vec2& p = parent.getPosition();

        // convert to [-1, 1]
        vertex.x = 2.f * (scissVertex.x * s.x + p.x) - 1.f;
        vertex.y = 2.f * ((1.f - scissVertex.y) * s.y + p.y) - 1.f;

        vertex.s = scissVertex.tx * parent.getSize().x + parent.getPosition().x;
        vertex.t = scissVertex.ty * parent.getSize().y + parent.getPosition().y;
    }

#ifdef CONVERT_SCISS_TO_DOMEPROJECTION
    // test export to dome projection
    std::string baseOutFilename = path.substr(0, path.find_last_of(".sgc") - 3);
    
    // test export frustum
    std::string outFrustumFilename = baseOutFilename + "_frustum" + std::string(".csv");
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
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
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
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
            outMeshFile << (1.f - texturedVertexList[i].ty) << ";"; // flip v-coord
            outMeshFile << x << ";";
            outMeshFile << y << std::endl;
            ++i;
        }
    }
    outMeshFile.close();
#endif

    _warpGeometry.nVertices = numberOfVertices;
    _warpGeometry.nIndices = numberOfIndices;
    
    if (fileVersion == '2' && size[0] == 4) {
        _warpGeometry.geometryType = GL_TRIANGLES;
    }
    else if (fileVersion == '2' && size[0] == 5) {
        _warpGeometry.geometryType = GL_TRIANGLE_STRIP;
    }
    else { // assume v1
        //GL_QUAD_STRIP removed in OpenGL 3.3+
        //mGeometries[WARP_MESH].geometryType = GL_QUAD_STRIP;
        _warpGeometry.geometryType = GL_TRIANGLE_STRIP;
    }

    texturedVertexList.clear();

    createMesh(_warpGeometry, buf.vertices, buf.indices);

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        numberOfVertices, numberOfIndices
    );
    
    if (Settings::instance()->getExportWarpingMeshes()) {
        const size_t found = path.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = path.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename), buf.vertices, buf.indices);
        }
    }

    return true;
}

bool CorrectionMesh::generateSimCADMesh(const std::string& path, const Viewport& parent) {
    // During projector alignment, a 33x33 matrix is used. This means 33x33 points can be
    // set to define geometry correction. So(x, y) coordinates are defined by the 33x33
    // matrix and the resolution used, defined by the tag. And the corrections to be
    // applied for every point in that 33x33 matrix, are stored in the warp file. This 
    // explains why this file only contains zero’s when no warp is applied.

    Buffer buf;

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Reading simcad warp data from '%s'\n",
        path.c_str()
    );

    tinyxml2::XMLDocument xmlDoc;
    if (xmlDoc.LoadFile(path.c_str()) != tinyxml2::XML_NO_ERROR) {
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

        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "ReadConfig: Error occured while reading config file '%s'\nError: %s\n",
            path.c_str(), str.c_str()
        );
        return false;
    }

    tinyxml2::XMLElement* XMLroot = xmlDoc.FirstChildElement("GeometryFile");
    if (XMLroot == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "ReadConfig: Error occured while reading config file '%s'\n"
            "Error: Cannot find XML root\n", path.c_str()
        );
        return false;
    }

    using namespace tinyxml2;
    XMLElement* element = XMLroot->FirstChildElement();
    if (element == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "ReadConfig: Error occured while reading config file '%s'\n"
            "Error: Cannot find XML root\n", path.c_str()
        );
        return false;
    }

    std::string_view val = element->Value();
    if (val != "GeometryDefinition") {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "ReadConfig: Error occured while reading config file '%s'\n"
            "Error: Missing value 'GeometryDefinition'\n", path.c_str()
        );
        return false;
    }

    float xrange = 1.f;
    float yrange = 1.f;
    std::vector<float> xcorrections, ycorrections;


    XMLElement* child = element->FirstChildElement();
    while (child) {
        std::string_view childVal = child->Value();
                    
        if (childVal == "X-FlatParameters") {
            if (child->QueryFloatAttribute("range", &xrange) == XML_NO_ERROR) {
                std::string xcoordstr(child->GetText());
                std::vector<std::string> xcoords = helpers::split(xcoordstr, ' ');
                for (const std::string& x : xcoords) {
                    xcorrections.push_back(std::stof(x) / xrange);
                } 
            }
        }
        else if (childVal == "Y-FlatParameters") {
            if (child->QueryFloatAttribute("range", &yrange) == XML_NO_ERROR) {
                std::string ycoordstr(child->GetText());
                std::vector<std::string> ycoords = helpers::split(ycoordstr, ' ');
                for (const std::string& y : ycoords) {
                    ycorrections.push_back(std::stof(y) / yrange);
                }
            }
        }

        child = child->NextSiblingElement();
    }

    if (xcorrections.size() != ycorrections.size()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Not the same x coords as y coords\n"
        );
        return false;
    }

    const float numberOfColsf = sqrt(static_cast<float>(xcorrections.size()));
    const float numberOfRowsf = sqrt(static_cast<float>(ycorrections.size()));

    if (ceil(numberOfColsf) != numberOfColsf || ceil(numberOfRowsf) != numberOfRowsf) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Not a valid squared matrix read from SimCAD file\n"
        );
        return false;
    }

    const unsigned int numberOfCols = static_cast<unsigned int>(numberOfColsf);
    const unsigned int numberOfRows = static_cast<unsigned int>(numberOfRowsf);


#ifdef CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC
    // export to dome projection
    std::string baseOutFile = path.substr(0, path.find_last_of(".simcad") - 6);

    // export domeprojection frustum
    std::string outFrustumFilename = baseOutFile + "_frustum" + std::string(".csv");
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
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
    glm::dvec3 angles = glm::degrees(
        glm::eulerAngles(
            glm::dquat(
                static_cast<double>(viewData.qw),
                static_cast<double>(viewData.qy),
                static_cast<double>(viewData.qx),
                static_cast<double>(viewData.qz)
            )
        )
    );
    yaw = -angles.x;
    pitch = angles.y;
    roll = -angles.z;

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Rotation quat = [%f %f %f %f]\n"
        "yaw = %lf, pitch = %lf, roll = %lf\n",
        viewData.qx, viewData.qy, viewData.qz, viewData.qw, yaw, pitch, roll
    );

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Position = [%f %f %f]\n", viewData.x, viewData.y, viewData.z
    );

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: FOV up = %f\n", viewData.fovUp
    );

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: FOV down = %f\n", viewData.fovDown
    );

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: FOV left = %f\n", viewData.fovLeft
    );

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: FOV right = %f\n", viewData.fovRight
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

    // start sgc export
    std::string outSGCFilenameV1 = baseOutFilename + std::string("_u2.sgc");
    std::string outSGCTFileNameV2 = baseOutFilename + std::string("_u3.sgc");
    
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Exporting sgc v1(u2) file \"%s\" and v2(u3) file \"%s\"\n",
        outSGCFilenameV1.c_str(), outSGCFilenameV2.c_str()
    );
    std::ofstream outSGCFileV1(outSGCFilenameV1, std::ios::out | std::ios::binary);
    std::ofstream outSGCFileV2(outSGCFilenameV2, std::ios::out | std::ios::binary);

    outSGCFileV1 << "SGC";
    outSGCFileV2 << "SGC";

    uint8_t SGCversion1 = 1;
    uint8_t SGCversion2 = 2;

    SCISSDistortionType dT = MESHTYPE_PLANAR;
    unsigned int vertexCount = numberOfCols * numberOfRows;
    unsigned int primType = 5; // Triangle list

    viewData.fovLeft = -viewData.fovLeft;
    viewData.fovDown = -viewData.fovDown;
    
    outSGCFileV1.write(reinterpret_cast<const char*>(&SGCversion1), sizeof(uint8_t));
    outSGCFileV1.write(reinterpret_cast<const char*>(&dT), sizeof(SCISSDistortionType));
    outSGCFileV1.write(reinterpret_cast<const char*>(&viewData), sizeof(SCISSViewData));
    outSGCFileV1.write(reinterpret_cast<const char*>(&numberOfCols), sizeof(unsigned int));
    outSGCFileV1.write(reinterpret_cast<const char*>(&numberOfRows), sizeof(unsigned int));

    outSGCFileV2.write(reinterpret_cast<const char*>(&SGCversion2), sizeof(uint8_t));
    outSGCFileV2.write(reinterpret_cast<const char*>(&dT), sizeof(SCISSDistortionType));
    outSGCFileV2.write(reinterpret_cast<const char*>(&viewData), sizeof(SCISSViewData));
    outSGCFileV2.write(reinterpret_cast<const char*>(&primType), sizeof(unsigned int));
    outSGCFileV2.write(reinterpret_cast<const char*>(&vertexCount), sizeof(unsigned int));

    //test export mesh
    std::string outMeshFilename = baseOutFilename + "_mesh" + std::string(".csv");
    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Exporting dome projection mesh file \"%s\"\n",
        outMeshFilename.c_str()
    );
    std::ofstream outMeshFile(outMeshFilename, std::ios::out);
    outMeshFile << "x;y;u;v;column;row" << std::endl;
    outMeshFile << std::fixed;
    outMeshFile << std::setprecision(6);
#endif // CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC

    CorrectionMeshVertex vertex;

    // init to max intensity (opaque white)
    vertex.r = 1.f;
    vertex.g = 1.f;
    vertex.b = 1.f;
    vertex.a = 1.f;

    size_t i = 0;
    for (unsigned int r = 0; r < numberOfRows; r++) {
        for (unsigned int c = 0; c < numberOfCols; c++) {
            // vertex-mapping
            const float u = (static_cast<float>(c) /
                            (static_cast<float>(numberOfCols) - 1.f));

            // @TODO (abock, 2019-09-01);  Not sure why we are inverting the y coordinate
            // for this, but we do it multiple times in this file and I'm starting to get
            // the impression that there might be a flipping issue going on somewhere
            // deeper
            const float v = 1.f - (static_cast<float>(r) /
                                  (static_cast<float>(numberOfRows) - 1.f));

            const float x = u + xcorrections[i];
            const float y = v - ycorrections[i];

            // convert to [-1, 1]
            vertex.x = 2.f * (x * parent.getSize().x + parent.getPosition().x) - 1.f;
            vertex.y = 2.f * (y * parent.getSize().y + parent.getPosition().y) - 1.f;

            // scale to viewport coordinates
            vertex.s = u * parent.getSize().x + parent.getPosition().x;
            vertex.t = v * parent.getSize().y + parent.getPosition().y;

            buf.vertices.push_back(vertex);
            i++;

#ifdef CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC
            outMeshFile << x << ";";
            outMeshFile << 1.f - y << ";";
            outMeshFile << u << ";";
            outMeshFile << 1.f - v << ";";
            outMeshFile << c << ";";
            outMeshFile << r << std::endl;

            SCISSTexturedVertex scissVertex;
            scissVertex.x = x;
            scissVertex.y = 1.f - y;
            scissVertex.tx = u;
            scissVertex.ty = v;

            outSGCFileV1.write(
                reinterpret_cast<const char*>(&scissVertex),
                sizeof(SCISSTexturedVertex)
            );
            outSGCFileV2.write(
                reinterpret_cast<const char*>(&scissVertex),
                sizeof(SCISSTexturedVertex)
            );
#endif // CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC
        }
    }

    // copy vertices
    unsigned int numberOfVertices = numberOfCols * numberOfRows;
    _warpGeometry.nVertices = numberOfVertices;

    // Make a triangle strip index list
    buf.indices.reserve(4 * numberOfRows * numberOfCols);
    for (unsigned int r = 0; r < numberOfRows - 1; r++) {
        if ((r % 2) == 0) {
            // even rows
            for (unsigned int c = 0; c < numberOfCols; c++) {
                buf.indices.push_back(c + r * numberOfCols);
                buf.indices.push_back(c + (r + 1) * numberOfCols);
            }
        }
        else {
            // odd rows
            for (unsigned int c = numberOfCols - 1; c > 0; c--) {
                buf.indices.push_back(c + (r + 1) * numberOfCols);
                buf.indices.push_back(c - 1 + r * numberOfCols);
            }
        }
    }

#ifdef CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC
    outMeshFile.close();

    unsigned int indicesCount = static_cast<unsigned int>(indicesTrilist.size());
    outSGCFileV1.write(reinterpret_cast<const char*>(&indicesCount), sizeof(unsigned int));
    outSGCFileV2.write(reinterpret_cast<const char*>(&indicesCount), sizeof(unsigned int));
    for (size_t i = 0; i < indicesTrilist.size(); i++) {
        outSGCFileV1.write(
            reinterpret_cast<const char*>(&indicesTrilist[i]),
            sizeof(unsigned int)
        );
        outSGCFileV2.write(
            reinterpret_cast<const char*>(&indicesTrilist[i]),
            sizeof(unsigned int)
        );
    }


    outSGCFileV1.close();
    outSGCFileV2.close();
#endif // CONVERT_SIMCAD_TO_DOMEPROJECTION_AND_SGC

    // allocate and copy indices
    _warpGeometry.nIndices = static_cast<unsigned int>(buf.indices.size());

    _warpGeometry.geometryType = GL_TRIANGLE_STRIP;

    createMesh(_warpGeometry, buf.vertices, buf.indices);

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        _warpGeometry.nVertices, _warpGeometry.nIndices
    );

    if (Settings::instance()->getExportWarpingMeshes()) {
        const size_t found = path.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = path.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename), buf.vertices, buf.indices);
        }
    }

    return true;
}

bool CorrectionMesh::generateSkySkanMesh(const std::string& meshPath, Viewport& parent) {
    Buffer buf;

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Reading SkySkan mesh data from '%s'\n", meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    float azimuth = 0.f;
    float elevation = 0.f;
    float horizontalFov = 0.f;
    float verticalFov = 0.f;
    glm::vec2 fovTweaks = glm::vec2(1.f);
    glm::vec2 uvTweaks = glm::vec2(1.f);
    bool dimensionsSet = false;
    bool azimuthSet = false;
    bool elevationSet = false;
    bool hFovSet = false;
    bool vFovSet = false;
    float x, y, u, v;

    unsigned int size[2];
    unsigned int counter = 0;

    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {

            if (_sscanf(lineBuffer, "Dome Azimuth=%f", &azimuth) == 1) {
                azimuthSet = true;
            }
            else if (_sscanf(lineBuffer, "Dome Elevation=%f", &elevation) == 1) {
                elevationSet = true;
            }
            else if (_sscanf(lineBuffer, "Horizontal FOV=%f", &horizontalFov) == 1) {
                hFovSet = true;
            }
            else if (_sscanf(lineBuffer, "Vertical FOV=%f", &verticalFov) == 1) {
                vFovSet = true;
            }
            else if (_sscanf(lineBuffer, "Horizontal Tweak=%f", &fovTweaks[0]) == 1) {
                ;
            }
            else if (_sscanf(lineBuffer, "Vertical Tweak=%f", &fovTweaks[1]) == 1) {
                ;
            }
            else if (_sscanf(lineBuffer, "U Tweak=%f", &uvTweaks[0]) == 1) {
                ;
            }
            else if (_sscanf(lineBuffer, "V Tweak=%f", &uvTweaks[1]) == 1) {
                ;
            }
            else if (!dimensionsSet &&
                     _sscanf(lineBuffer, "%u %u", &size[0], &size[1]) == 2)
            {
                dimensionsSet = true;
                buf.vertices.resize(size[0] * size[1]);
                _warpGeometry.nVertices = size[0] * size[1];
            }
            else if (dimensionsSet &&
                     _sscanf(lineBuffer, "%f %f %f %f", &x, &y, &u, &v) == 4)
            {
                if (uvTweaks[0] > -1.f) {
                    u *= uvTweaks[0];
                }

                if (uvTweaks[1] > -1.f) {
                    v *= uvTweaks[1];
                }
                
                buf.vertices[counter].x = x;
                buf.vertices[counter].y = y;
                buf.vertices[counter].s = u;
                buf.vertices[counter].t = 1.f - v;

                buf.vertices[counter].r = 1.f;
                buf.vertices[counter].g = 1.f;
                buf.vertices[counter].b = 1.f;
                buf.vertices[counter].a = 1.f;
                counter++;
            }
        }
    }

    fclose(meshFile);

    if (!dimensionsSet || !azimuthSet || !elevationSet || !hFovSet ||
        horizontalFov <= 0.f)
    {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Data reading error\n"
        );
        return false;
    }

    // create frustums and projection matrices
    if (!vFovSet || verticalFov <= 0.f) {
        // half the width (radius is one unit, cancels it self out)
        const float hw = tan(glm::radians<float>(horizontalFov) / 2.f);
        // half height
        const float hh = (1200.f / 2048.f) * hw;
        
        verticalFov = 2.f * glm::degrees<float>(atan(hh));

        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "HFOV: %f VFOV: %f\n", horizontalFov, verticalFov
        );
    }

    if (fovTweaks[0] > 0.f) {
        horizontalFov *= fovTweaks[0];
    }
    if (fovTweaks[1] > 0.f) {
        verticalFov *= fovTweaks[1];
    }

    glm::quat rotQuat;
    rotQuat = glm::rotate(rotQuat, glm::radians(-azimuth), glm::vec3(0.f, 1.f, 0.f));
    rotQuat = glm::rotate(rotQuat, glm::radians(elevation), glm::vec3(1.f, 0.f, 0.f));

    parent.getUser().setPos(glm::vec3(0.f));
    parent.setViewPlaneCoordsUsingFOVs(
        verticalFov / 2.f,
        -verticalFov / 2.f,
        -horizontalFov / 2.f,
        horizontalFov / 2.f,
        rotQuat
    );
    
    sgct::Engine::instance()->updateFrustums();

    for (unsigned int c = 0; c < (size[0] - 1); c++) {
        for (unsigned int r = 0; r < (size[1] - 1); r++) {
            const unsigned int i0 = r * size[0] + c;
            const unsigned int i1 = r * size[0] + (c + 1);
            const unsigned int i2 = (r + 1) * size[0] + (c + 1);
            const unsigned int i3 = (r + 1) * size[0] + c;

            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

            // triangle 1
            if (buf.vertices[i0].x != -1.f && buf.vertices[i0].y != -1.f &&
                buf.vertices[i1].x != -1.f && buf.vertices[i1].y != -1.f &&
                buf.vertices[i2].x != -1.f && buf.vertices[i2].y != -1.f)
            {
                buf.indices.push_back(i0);
                buf.indices.push_back(i1);
                buf.indices.push_back(i2);
            }

            // triangle 2
            if (buf.vertices[i0].x != -1.f && buf.vertices[i0].y != -1.f &&
                buf.vertices[i2].x != -1.f && buf.vertices[i2].y != -1.f &&
                buf.vertices[i3].x != -1.f && buf.vertices[i3].y != -1.f)
            {
                buf.indices.push_back(i0);
                buf.indices.push_back(i2);
                buf.indices.push_back(i3);
            }
        }
    }

    for (unsigned int i = 0; i < _warpGeometry.nVertices; i++) {
        const glm::vec2& s = parent.getSize();
        const glm::vec2& p = parent.getPosition();

        // convert to [-1, 1]
        buf.vertices[i].x = 2.f * (buf.vertices[i].x * s.x + p.x) - 1.f;
        buf.vertices[i].y = 2.f * ((1.f - buf.vertices[i].y) * s.y + p.y) - 1.f;

        buf.vertices[i].s = buf.vertices[i].s * s.x + p.x;
        buf.vertices[i].t = buf.vertices[i].t * s.y + p.y;
    }

    // allocate and copy indices
    _warpGeometry.nIndices = static_cast<unsigned int>(buf.indices.size());
    _warpGeometry.geometryType = GL_TRIANGLES;

    createMesh(_warpGeometry, buf.vertices, buf.indices);

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        _warpGeometry.nVertices, _warpGeometry.nIndices
    );

    if (Settings::instance()->getExportWarpingMeshes()) {
        const size_t found = meshPath.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = meshPath.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename), buf.vertices, buf.indices);
        }
    }

    return true;
}

bool CorrectionMesh::generatePaulBourkeMesh(const std::string& meshPath,
                                            const Viewport& parent)
{
    Buffer buf;

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Reading Paul Bourke spherical mirror mesh data from '%s'\n",
        meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    char lineBuffer[MaxLineLength];

    // get the fist line containing the mapping type _id
    int mappingType = -1;
    if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
        _sscanf(lineBuffer, "%d", &mappingType);
    }

    // get the mesh dimensions
    glm::ivec2 meshSize = glm::ivec2(-1, -1);
    if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
        if (_sscanf(lineBuffer, "%d %d", &meshSize[0], &meshSize[1]) == 2) {
            buf.vertices.reserve(meshSize.x * meshSize.y);
            _warpGeometry.nVertices =
                static_cast<unsigned int>(meshSize.x * meshSize.y);
        }
    }

    // check if everyting useful is set
    if (mappingType == -1 || meshSize.x == -1 || meshSize.y == -1) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Invalid data"
        );
        return false;
    }

    // get all data
    float x, y, s, t, intensity;
    while (!feof(meshFile)) {
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            if (_sscanf(lineBuffer, "%f %f %f %f %f", &x, &y, &s, &t, &intensity) == 5) {
                CorrectionMeshVertex vertex;
                vertex.x = x;
                vertex.y = y;
                vertex.s = s;
                vertex.t = t;

                vertex.r = intensity;
                vertex.g = intensity;
                vertex.b = intensity;
                vertex.a = 1.f;

                buf.vertices.push_back(vertex);
            }
        }
    }

    // generate indices
    for (int c = 0; c < (meshSize.x - 1); c++) {
        for (int r = 0; r < (meshSize.y - 1); r++) {
            const int i0 = r * meshSize.x + c;
            const int i1 = r * meshSize.x + (c + 1);
            const int i2 = (r + 1) * meshSize.x + (c + 1);
            const int i3 = (r + 1) * meshSize.x + c;

            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

            // triangle 1
            buf.indices.push_back(i0);
            buf.indices.push_back(i1);
            buf.indices.push_back(i2);

            // triangle 2
            buf.indices.push_back(i0);
            buf.indices.push_back(i2);
            buf.indices.push_back(i3);
        }
    }

    float aspect = Engine::instance()->getCurrentWindow().getAspectRatio() *
                   (parent.getSize().x / parent.getSize().y);
    
    for (unsigned int i = 0; i < _warpGeometry.nVertices; i++) {
        const glm::vec2& size = parent.getSize();
        const glm::vec2& pos = parent.getPosition();

        // convert to [0, 1] (normalize)
        buf.vertices[i].x /= aspect;
        buf.vertices[i].x = (buf.vertices[i].x + 1.f) / 2.f;
        buf.vertices[i].y = (buf.vertices[i].y + 1.f) / 2.f;
        
        // scale, re-position and convert to [-1, 1]
        buf.vertices[i].x = (buf.vertices[i].x * size.x + pos.x) * 2.f - 1.f;
        buf.vertices[i].y = (buf.vertices[i].y * size.y + pos.y) * 2.f - 1.f;

        // convert to viewport coordinates
        buf.vertices[i].s = buf.vertices[i].s * size.x + pos.x;
        buf.vertices[i].t = buf.vertices[i].t * size.y + pos.y;
    }

    // allocate and copy indices
    _warpGeometry.nIndices = static_cast<unsigned int>(buf.indices.size());
    _warpGeometry.geometryType = GL_TRIANGLES;
    createMesh(_warpGeometry, buf.vertices, buf.indices);

    // force regeneration of dome render quad
    FisheyeProjection* fishPrj = dynamic_cast<FisheyeProjection*>(
        parent.getNonLinearProjection()
    );
    if (fishPrj) {
        fishPrj->setIgnoreAspectRatio(true);
        fishPrj->update(glm::ivec2(1.f, 1.f));
    }

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        _warpGeometry.nVertices, _warpGeometry.nIndices
    );

    if (Settings::instance()->getExportWarpingMeshes()) {
        const size_t found = meshPath.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = meshPath.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename), buf.vertices, buf.indices);
        }
    }

    return true;
}

bool CorrectionMesh::generateOBJMesh(const std::string& meshPath) {
    Buffer buf;

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Reading Maya Wavefront OBJ mesh data from '%s'\n",
        meshPath.c_str()
    );

    FILE* meshFile = nullptr;
#if (_MSC_VER >= 1400)
    if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#else
    meshFile = fopen(path.c_str(), "r");
    if (meshFile == nullptr) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Failed to open warping mesh file\n"
        );
        return false;
    }
#endif

    unsigned int counter = 0;
    while (!feof(meshFile)) {
        char lineBuffer[MaxLineLength];
        if (fgets(lineBuffer, MaxLineLength, meshFile) != nullptr) {
            CorrectionMeshVertex tmpVert;

            int i0;
            int i1;
            int i2;
            if (_sscanf(lineBuffer, "v %f %f %*f", &tmpVert.x, &tmpVert.y) == 2) {
                tmpVert.r = 1.f;
                tmpVert.g = 1.f;
                tmpVert.b = 1.f;
                tmpVert.a = 1.f;

                buf.vertices.push_back(tmpVert);
            }
            else if (_sscanf(lineBuffer, "vt %f %f %*f", &tmpVert.s, &tmpVert.t) == 2) {
                if (counter < buf.vertices.size()) {
                    buf.vertices[counter].s = tmpVert.s;
                    buf.vertices[counter].t = tmpVert.t;
                }
                
                counter++;
            }
            else if (_sscanf(
                        lineBuffer,
                        "f %d/%*d/%*d %d/%*d/%*d %d/%*d/%*d", &i0, &i1, &i2
                    ) == 3)
            {
                // indexes starts at 1 in OBJ
                buf.indices.push_back(i0 - 1);
                buf.indices.push_back(i1 - 1);
                buf.indices.push_back(i2 - 1);
            }
        }
    }

    // sanity check
    if (counter != buf.vertices.size() || buf.vertices.empty()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Vertex count doesn't match number of texture coordinates\n"
        );
        return false;
    }

    // allocate and copy indices
    _warpGeometry.nIndices = static_cast<unsigned int>(buf.indices.size());
    _warpGeometry.nVertices = static_cast<unsigned int>(buf.vertices.size());
    _warpGeometry.geometryType = GL_TRIANGLES;
    createMesh(_warpGeometry, buf.vertices, buf.indices);

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Correction mesh read successfully. Vertices=%u, Indices=%u\n",
        _warpGeometry.nVertices, _warpGeometry.nIndices
    );

    if (Settings::instance()->getExportWarpingMeshes()) {
        const size_t found = meshPath.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = meshPath.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename), buf.vertices, buf.indices);
        }
    }

    return true;
}

bool CorrectionMesh::generateMpcdiMesh(const std::string& meshPath,
                                       const Viewport& parent)
{
    Buffer buf;

    bool isReadingFile = !meshPath.empty();

    FILE* meshFile = nullptr;
    size_t srcSizeBytes = 0;
    const unsigned char* srcBuff = nullptr;
    if (isReadingFile) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "CorrectionMesh: Reading MPCDI mesh (PFM format) data from '%s'\n",
            meshPath.c_str()
        );
#if (_MSC_VER >= 1400)
        if (fopen_s(&meshFile, meshPath.c_str(), "r") != 0 || !meshFile) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "CorrectionMesh: Failed to open warping mesh file\n"
            );
            return false;
        }
#else
        meshFile = fopen(path.c_str(), "rb");
        if (meshFile == nullptr) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "CorrectionMesh: Failed to open warping mesh file\n"
            );
            return false;
        }
#endif
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Info,
            "CorrectionMesh: Reading MPCDI mesh (PFM format) from buffer\n",
            meshPath.c_str()
        );
        srcBuff = parent.mpcdiWarpMesh().data();
        srcSizeBytes = parent.mpcdiWarpMesh().size();
    }

    const int MaxHeaderLineLength = 100;
    char headerBuffer[MaxHeaderLineLength];

    unsigned int srcIdx = 0;
    int index = 0;
    int nNewlines = 0;
    do {
        size_t retval;
        char headerChar = 0;
        if (isReadingFile) {
#if (_MSC_VER >= 1400)
            retval = fread_s(&headerChar, sizeof(char)*1, sizeof(char), 1, meshFile);
#else
            retval = fread(&headerChar, sizeof(char), 1, meshFile);
#endif
        }
        else {
            if (srcIdx == srcSizeBytes) {
                retval = static_cast<size_t>(-1);
            }
            else {
                headerChar = srcBuff[srcIdx++];
                retval = 1;
            }
        }
        if (retval != 1) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "CorrectionMesh: Error reading from file\n"
            );
            if (meshFile) {
                fclose(meshFile);
            }
            return false;
        }

        headerBuffer[index++] = headerChar;
        if (headerChar == '\n') {
            nNewlines++;
        }
    } while (nNewlines < 3);

    char fileFormatHeader[2];
    unsigned int numberOfCols = 0;
    unsigned int numberOfRows = 0;
    float endiannessIndicator = 0;

    // @TODO (abock, 2019-08-30): I do not really understand how any of this works
#ifdef WIN32
    _sscanf(
        headerBuffer,
        "%2c\n",
        &fileFormatHeader,
        static_cast<unsigned int>(sizeof(fileFormatHeader))
    );
    // Read header past the 2 character start
    _sscanf(&headerBuffer[3], "%d %d\n", &numberOfCols, &numberOfRows);

    constexpr auto nDigits = [](unsigned int i) {
        return static_cast<int>(ceil(log(static_cast<double>(i))));;
    };
    const int indexForEndianness = 3 + nDigits(numberOfCols) + nDigits(numberOfRows) + 2;
    _sscanf(&headerBuffer[indexForEndianness], "%f\n", &endiannessIndicator);
#else
    if (_sscanf(headerBuffer, "%2c %d %d %f", fileFormatHeader,
                &numberOfCols, &numberOfRows, &endiannessIndicator) != 4)
    {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
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
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh: Incorrect file type\n"
        );
    }
    const int numCorrectionValues = numberOfCols * numberOfRows;
    std::vector<float> corrGridX(numCorrectionValues);
    std::vector<float> corrGridY(numCorrectionValues);
    float errorPos;

    if (isReadingFile) {
        size_t ret = 0;
        for (int i = 0; i < numCorrectionValues; ++i) {
#ifdef WIN32
            ret = fread_s(&corrGridX[i], numCorrectionValues, sizeof(int), 1, meshFile);
            ret = fread_s(&corrGridY[i], numCorrectionValues, sizeof(int), 1, meshFile);
            // MPCDI uses the PFM format for correction grid. PFM format is designed for
            // 3 RGB values. However MPCDI substitutes Red for X correction, Green for Y
            // correction, and Blue for correction error. This will be NaN for error value
            ret = fread_s(&errorPos, numCorrectionValues, sizeof(int), 1, meshFile);
#else
            ret = fread(&corrGridX[i], sizeof(float), 1, meshFile);
            ret = fread(&corrGridY[i], sizeof(float), 1, meshFile);
            ret = fread(&errorPos, sizeof(float), 1, meshFile);
#endif
        }
        fclose(meshFile);
        if (ret != 4) {
            MessageHandler::instance()->print(
                MessageHandler::Level::Error,
                "CorrectionMesh: Error reading all correction values!\n"
            );
            return false;
        }
    }
    else {
        bool readErr = false;
        for (int i = 0; i < numCorrectionValues; ++i) {
            readErr |= !readMeshBuffer(&corrGridX[i], srcIdx, srcBuff, srcSizeBytes, 4);
            readErr |= !readMeshBuffer(&corrGridY[i], srcIdx, srcBuff, srcSizeBytes, 4);
            readErr |= !readMeshBuffer(&errorPos, srcIdx, srcBuff, srcSizeBytes, 4);

            if (readErr) {
                MessageHandler::instance()->print(
                    MessageHandler::Level::Error,
                    "CorrectionMesh: Error reading mpcdi correction value at index %d\n",
                    i
                );
            }
        }
    }

    std::vector<glm::vec2> smoothPos(numCorrectionValues);
    std::vector<glm::vec2> warpedPos(numCorrectionValues);

    for (int i = 0; i < numCorrectionValues; ++i) {
        const int gridIndex_column = i % numberOfCols;
        const int gridIndex_row = i / numberOfCols;
        // Compute XY positions for each point based on a normalized 0,0 to 1,1 grid,
        // add the correction offsets to each warp point
        smoothPos[i].x = static_cast<float>(gridIndex_column) /
                         static_cast<float>(numberOfCols - 1);
        // Reverse the y position because the values from pfm file are given in raster-scan
        // order, which is left to right but starts at upper-left rather than lower-left.
        smoothPos[i].y = 1.f - (static_cast<float>(gridIndex_row) /
                         static_cast<float>(numberOfRows - 1));
        warpedPos[i].x = smoothPos[i].x + corrGridX[i];
        warpedPos[i].y = smoothPos[i].y + corrGridY[i];
    }

    corrGridX.clear();
    corrGridY.clear();

#ifdef NORMALIZE_CORRECTION_MESH
    glm::vec2 max = *std::max_element(warpedPos, warpedPos + numCorrectionValues);
    glm::vec2 min = *std::min_element(warpedPos, warpedPos + numCorrectionValues);
    float scaleRangeX = max.x - min.x;
    float scaleRangeY = max.y - min.y;
    float scaleFactor = std::max(scaleRangeX, scaleRangeY);
    // Scale all positions to fit within 0,0 to 1,1
    for (int i = 0; i < numCorrectionValues; ++i) {
        warpedPos[i].x = (warpedPos[i].x - minX) / scaleFactor;
        warpedPos[i].y = (warpedPos[i].y - minY) / scaleFactor;
    }
#endif // NORMALIZE_CORRECTION_MESH

    buf.vertices.reserve(numCorrectionValues);
    for (int i = 0; i < numCorrectionValues; ++i) {
        CorrectionMeshVertex vertex;
        // init to max intensity (opaque white)
        vertex.r = 1.f;
        vertex.g = 1.f;
        vertex.b = 1.f;
        vertex.a = 1.f;

        vertex.s = smoothPos[i].x;
        vertex.t = smoothPos[i].y;

        // scale to viewport coordinates
        vertex.x = 2.f * warpedPos[i].x - 1.f;
        vertex.y = 2.f * warpedPos[i].y - 1.f;
        buf.vertices.push_back(vertex);
    }
    warpedPos.clear();
    smoothPos.clear();

    _warpGeometry.nVertices = numberOfCols * numberOfRows;

    buf.indices.reserve(numberOfCols * numberOfRows * 6);
    for (unsigned int c = 0; c < (numberOfCols - 1); c++) {
        for (unsigned int r = 0; r < (numberOfRows - 1); r++) {
            const unsigned int i0 = r * numberOfCols + c;
            const unsigned int i1 = r * numberOfCols + (c + 1);
            const unsigned int i2 = (r + 1) * numberOfCols + (c + 1);
            const unsigned int i3 = (r + 1) * numberOfCols + c;

            // 3      2
            //  x____x
            //  |   /|
            //  |  / |
            //  | /  |
            //  |/   |
            //  x----x
            // 0      1

            // triangle 1
            buf.indices.push_back(i0);
            buf.indices.push_back(i1);
            buf.indices.push_back(i2);

            // triangle 2
            buf.indices.push_back(i0);
            buf.indices.push_back(i2);
            buf.indices.push_back(i3);
        }
    }

    // allocate and copy indices
    _warpGeometry.nIndices = static_cast<unsigned int>(buf.indices.size());
    _warpGeometry.geometryType = GL_TRIANGLES;

    createMesh(_warpGeometry, buf.vertices, buf.indices);

    MessageHandler::instance()->print(
        MessageHandler::Level::Debug,
        "CorrectionMesh: Mpcdi Correction mesh read successfully. "
        "Vertices=%u, Indices=%u\n",
        _warpGeometry.nVertices, _warpGeometry.nIndices
    );

    if (Settings::instance()->getExportWarpingMeshes()) {
        const size_t found = meshPath.find_last_of(".");
        if (found != std::string::npos) {
            std::string filename = meshPath.substr(0, found) + "_export.obj";
            exportMesh(std::move(filename), buf.vertices, buf.indices);
        }
    }

    return true;
}

CorrectionMesh::Buffer CorrectionMesh::setupSimpleMesh(CorrectionMeshGeometry& geomPtr,
                                                       const Viewport& parent)
{
    geomPtr.nVertices = 4;
    geomPtr.nIndices = 4;
    geomPtr.geometryType = GL_TRIANGLE_STRIP;

    Buffer buff;
    buff.indices = { 0, 3, 1, 2 };

    buff.vertices.resize(4, CorrectionMeshVertex());
    buff.vertices[0].r = 1.f;
    buff.vertices[0].g = 1.f;
    buff.vertices[0].b = 1.f;
    buff.vertices[0].a = 1.f;
    buff.vertices[0].s = 0.f * parent.getSize().x + parent.getPosition().x;
    buff.vertices[0].t = 0.f * parent.getSize().y + parent.getPosition().y;
    buff.vertices[0].x = 2.f * (0.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    buff.vertices[0].y = 2.f * (0.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    buff.vertices[1].r = 1.f;
    buff.vertices[1].g = 1.f;
    buff.vertices[1].b = 1.f;
    buff.vertices[1].a = 1.f;
    buff.vertices[1].s = 1.f * parent.getSize().x + parent.getPosition().x;
    buff.vertices[1].t = 0.f * parent.getSize().y + parent.getPosition().y;
    buff.vertices[1].x = 2.f * (1.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    buff.vertices[1].y = 2.f * (0.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    buff.vertices[2].r = 1.f;
    buff.vertices[2].g = 1.f;
    buff.vertices[2].b = 1.f;
    buff.vertices[2].a = 1.f;
    buff.vertices[2].s = 1.f * parent.getSize().x + parent.getPosition().x;
    buff.vertices[2].t = 1.f * parent.getSize().y + parent.getPosition().y;
    buff.vertices[2].x = 2.f * (1.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    buff.vertices[2].y = 2.f * (1.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    buff.vertices[3].r = 1.f;
    buff.vertices[3].g = 1.f;
    buff.vertices[3].b = 1.f;
    buff.vertices[3].a = 1.f;
    buff.vertices[3].s = 0.f * parent.getSize().x + parent.getPosition().x;
    buff.vertices[3].t = 1.f * parent.getSize().y + parent.getPosition().y;
    buff.vertices[3].x = 2.f * (0.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    buff.vertices[3].y = 2.f * (1.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    return buff;
}

CorrectionMesh::Buffer CorrectionMesh::setupMaskMesh(const Viewport& parent,
                                                     bool flipX, bool flipY)
{
    _maskGeometry.nVertices = 4;
    _maskGeometry.nIndices = 4;
    _maskGeometry.geometryType = GL_TRIANGLE_STRIP;

    Buffer buff;
    buff.indices = { 0, 3, 1, 2 };

    buff.vertices.resize(4);
    buff.vertices[0].r = 1.f;
    buff.vertices[0].g = 1.f;
    buff.vertices[0].b = 1.f;
    buff.vertices[0].a = 1.f;
    buff.vertices[0].s = flipX ? 1.f : 0.f;
    buff.vertices[0].t = flipY ? 1.f : 0.f;
    buff.vertices[0].x = 2.f * (0.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    buff.vertices[0].y = 2.f * (0.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    buff.vertices[1].r = 1.f;
    buff.vertices[1].g = 1.f;
    buff.vertices[1].b = 1.f;
    buff.vertices[1].a = 1.f;
    buff.vertices[1].s = flipX ? 0.f : 1.f;
    buff.vertices[1].t = flipY ? 1.F : 0.f;
    buff.vertices[1].x = 2.f * (1.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    buff.vertices[1].y = 2.f * (0.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    buff.vertices[2].r = 1.f;
    buff.vertices[2].g = 1.f;
    buff.vertices[2].b = 1.f;
    buff.vertices[2].a = 1.f;
    buff.vertices[2].s = flipX ? 0.f : 1.f;
    buff.vertices[2].t = flipY ? 0.f : 1.f;
    buff.vertices[2].x = 2.f * (1.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    buff.vertices[2].y = 2.f * (1.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    buff.vertices[3].r = 1.f;
    buff.vertices[3].g = 1.f;
    buff.vertices[3].b = 1.f;
    buff.vertices[3].a = 1.f;
    buff.vertices[3].s = flipX ? 1.f : 0.f;
    buff.vertices[3].t = flipY ? 0.f : 1.f;
    buff.vertices[3].x = 2.f * (0.f * parent.getSize().x + parent.getPosition().x) - 1.f;
    buff.vertices[3].y = 2.f * (1.f * parent.getSize().y + parent.getPosition().y) - 1.f;

    return buff;
}

void CorrectionMesh::createMesh(CorrectionMeshGeometry& geom,
                                const std::vector<CorrectionMeshVertex>& vertices,
                                const std::vector<unsigned int>& indices)
{
    if (ClusterManager::instance()->getMeshImplementation() ==
        ClusterManager::MeshImplementation::BufferObjects)
    {
        glGenVertexArrays(1, &(geom.arrayData));
        glBindVertexArray(geom.arrayData);

        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Generating VAO: %d\n", geom.arrayData
        );

        glGenBuffers(1, &geom.vertexData);
        glGenBuffers(1, &geom.indexData);
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Generating VBOs: %d %d\n",
            geom.vertexData, geom.indexData
        );

        glBindBuffer(GL_ARRAY_BUFFER, geom.vertexData);
        glBufferData(
            GL_ARRAY_BUFFER,
            geom.nVertices * sizeof(CorrectionMeshVertex),
            vertices.data(),
            GL_STATIC_DRAW
        );

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(CorrectionMeshVertex),
            reinterpret_cast<void*>(0)
        );

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(
            1,
            2,
            GL_FLOAT,
            GL_FALSE,
            sizeof(CorrectionMeshVertex),
            reinterpret_cast<void*>(8)
        );

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(
            2,
            4,
            GL_FLOAT,
            GL_FALSE,
            sizeof(CorrectionMeshVertex),
            reinterpret_cast<void*>(16)
        );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, geom.indexData);
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            geom.nIndices * sizeof(unsigned int),
            indices.data(),
            GL_STATIC_DRAW
        );

        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }
    else {
        // display lists
        geom.vertexData = glGenLists(1);
        glNewList(geom.vertexData, GL_COMPILE);
        glBegin(geom.geometryType);
        
        for (unsigned int i = 0; i < geom.nIndices; i++) {
            const CorrectionMeshVertex& vertex = vertices[indices[i]];

            glColor4f(vertex.r, vertex.g, vertex.b, vertex.a);
            glTexCoord2f(vertex.s, vertex.t);
            glVertex2f(vertex.x, vertex.y);
        }
        glEnd();
        glEndList();
        
        MessageHandler::instance()->print(
            MessageHandler::Level::Debug,
            "CorrectionMesh: Generating display list: %d\n", geom.vertexData
        );
    }
}

void CorrectionMesh::exportMesh(const std::string& exportMeshPath,
                                const std::vector<CorrectionMeshVertex>& vertices,
                                const std::vector<unsigned int>& indices)
{
    if (_warpGeometry.geometryType != GL_TRIANGLES &&
        _warpGeometry.geometryType != GL_TRIANGLE_STRIP)
    {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh error: Failed to export '%s'. "
            "Geometry type is not supported!\n", exportMeshPath.c_str()
        );
        return;
    }
    
    std::ofstream file(exportMeshPath, std::ios::out);
    if (!file.is_open()) {
        MessageHandler::instance()->print(
            MessageHandler::Level::Error,
            "CorrectionMesh error: Failed to export '%s'\n", exportMeshPath.c_str()
        );
        return;
    }

    file << std::fixed;
    file << std::setprecision(6);
        
    file << "# SGCT warping mesh\n";
    file << "# Number of vertices: " << _warpGeometry.nVertices << "\n";
        
    // export vertices
    for (unsigned int i = 0; i < _warpGeometry.nVertices; i++) {
        file << "v " << vertices[i].x << ' ' << vertices[i].y << " 0\n";
    }

    // export texture coords
    for (unsigned int i = 0; i < _warpGeometry.nVertices; i++) {
        file << "vt " << vertices[i].s << ' ' << vertices[i].t << " 0\n";
    }

    // export generated normals
    for (unsigned int i = 0; i < _warpGeometry.nVertices; i++) {
        file << "vn 0 0 1\n";
    }

    file << "# Number of faces: " << _warpGeometry.nIndices / 3 << '\n';

    // export face indices
    if (_warpGeometry.geometryType == GL_TRIANGLES) {
        for (unsigned int i = 0; i < _warpGeometry.nIndices; i += 3) {
            file << "f " << indices[i] + 1 << '/' << indices[i] + 1 << '/'
                 << indices[i] + 1 << ' ';
            file << indices[i + 1] + 1 << '/' << indices[i + 1] + 1 << '/'
                 << indices[i + 1] + 1 << ' ';
            file << indices[i + 2] + 1 << '/' << indices[i + 2] + 1 << '/'
                 << indices[i + 2] + 1 << '\n';
        }
    }
    else {
        // triangle strip

        // first base triangle
        file << "f " << indices[0] + 1 << '/' << indices[0] + 1 << '/'
             << indices[0] + 1 << ' ';
        file << indices[1] + 1 << '/' << indices[1] + 1 << '/'
             << indices[1] + 1 << ' ';
        file << indices[2] + 1 << '/' << indices[2] + 1 << '/'
             << indices[2] + 1 << '\n';

        for (unsigned int i = 2; i < _warpGeometry.nIndices; i++) {
            file << "f " << indices[i] + 1 << '/' << indices[i] + 1 << '/'
                 << indices[i] + 1 << ' ';
            file << indices[i - 1] + 1 << '/' << indices[i - 1] + 1 << '/'
                 << indices[i - 1] + 1 << ' ';
            file << indices[i - 2] + 1 << '/' << indices[i - 2] + 1 << '/'
                 << indices[i - 2] + 1 << '\n';
        }
    }

    file.close();

    MessageHandler::instance()->print(
        MessageHandler::Level::Info,
        "CorrectionMesh: Mesh '%s' exported successfully\n", exportMeshPath.c_str()
    );
}

void CorrectionMesh::render(const CorrectionMeshGeometry& mt) const {
    if (Settings::instance()->getShowWarpingWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    if (ClusterManager::instance()->getMeshImplementation() ==
        ClusterManager::MeshImplementation::BufferObjects)
    {
        glBindVertexArray(mt.arrayData);
        glDrawElements(
            mt.geometryType,
            mt.nIndices,
            GL_UNSIGNED_INT,
            nullptr
        );
        glBindVertexArray(0);
    }
    else {
        glCallList(mt.vertexData);
    }

    if (Settings::instance()->getShowWarpingWireframe()) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
}

void CorrectionMesh::renderQuadMesh() const {
    render(_quadGeometry);
}

void CorrectionMesh::renderWarpMesh() const {
    render(_warpGeometry);
}

void CorrectionMesh::renderMaskMesh() const {
    render(_maskGeometry);
}

CorrectionMesh::Hint CorrectionMesh::parseHint(const std::string& hintStr) {
    if (hintStr.empty()) {
        return Hint::None;
    }
    
    // transform to lowercase
    std::string str(hintStr);
    std::transform(
        str.begin(),
        str.end(),
        str.begin(),
        [](char c) { return static_cast<char>(::tolower(c)); }
    );

    CorrectionMesh::Hint hint = Hint::None;
    if (str == "domeprojection") {
        hint = Hint::DomeProjection;
    }
    else if (str == "scalable") {
        hint = Hint::Scaleable;
    }
    else if (str == "sciss") {
        hint = Hint::Sciss;
    }
    else if (str == "simcad") {
        hint = Hint::SimCad;
    }
    else if (str == "skyskan") {
        hint = Hint::SkySkan;
    }
    else if (str == "mpcdi") {
        hint = Hint::Mpcdi;
    }
    else {
        MessageHandler::instance()->print(
            MessageHandler::Level::Warning,
            "CorrectionMesh: hint '%s' is invalid\n", hintStr.c_str()
        );
    }

    return hint;
}

} // namespace sgct::core
