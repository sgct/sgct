#include "objloader.h"

#include <vector>
#include <string>
#include <glm/glm.hpp>


// Very, VERY simple OBJ loader.
// Here is a short list of features a real function would provide : 
// - Binary files. Reading a model should be just a few memcpy's away, not parsing a file
//   at runtime. In short : OBJ is not very great.
// - Animations & bones (includes bones weights)
// - Multiple UVs
// - All attributes should be optional, not "forced"
// - More stable. Change a line in the OBJ file and it crashes.
// - More secure. Change another line and you can inject code.
// - Loading from memory, stream, etc

bool loadOBJ(const std::string& path, std::vector<glm::vec3>& vertices, 
             std::vector<glm::vec2>& uvs, std::vector<glm::vec3>& normals)
{
    printf("Loading OBJ file %s", path.c_str());

    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    struct {
        std::vector<glm::vec3> vertices;
        std::vector<glm::vec2> uvs;
        std::vector<glm::vec3> normals;
    } tmp;

#if (_MSC_VER >= 1400)
    FILE* file;
    errno_t err = fopen_s(&file, path.c_str(), "r");
    const bool success = (err == 0);
#else
    FILE* file = fopen(path.c_str(), "r");
    const bool success = file;
#endif
    if (!success) {
        printf("Impossible to open the file! Are you in the right path?\n");
        return false;
    }

    constexpr const int LineHeaderStrSize = 128;
    constexpr const int FaceCoordStrSize = 64;

    char lineHeader[LineHeaderStrSize];

    // holders for face coord data
    // supports only triangles (3 values)
    // fix to read 3DsMax obj exported files by Miro
    char faceCoord0[FaceCoordStrSize];
    char faceCoord1[FaceCoordStrSize];
    char faceCoord2[FaceCoordStrSize];

    while (true) {
        // read the first word of the line
#if (_MSC_VER >= 1400)
        int res = fscanf_s(file, "%s", lineHeader, LineHeaderStrSize);
#else
        int res = fscanf(file, "%s", lineHeader);
#endif
        // EOF = End Of File. Quit the loop.
        if (res == EOF) {
            break; 
        }

        if (strcmp(lineHeader, "v") == 0) {
            glm::vec3 vertex;
#if (_MSC_VER >= 1400)
            fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
#else
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
#endif
            tmp.vertices.push_back(vertex);
        }
        else if (strcmp(lineHeader, "vt") == 0) {
            glm::vec2 uv;
#if (_MSC_VER >= 1400)
            fscanf_s(file, "%f %f\n", &uv.x, &uv.y);
#else
            fscanf(file, "%f %f\n", &uv.x, &uv.y);
#endif
            tmp.uvs.push_back(uv);
        }
        else if (strcmp(lineHeader, "vn") == 0) {
            glm::vec3 normal;
#if (_MSC_VER >= 1400)
            fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
#else
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
#endif
            tmp.normals.push_back(normal);
        }
        else if (strcmp( lineHeader, "f") == 0) {
            std::string vertex1, vertex2, vertex3;
            unsigned int vIdx0;
            unsigned int vIdx1;
            unsigned int vIdx2;

            unsigned int uvIdx0;
            unsigned int uvIdx1;
            unsigned int uvIdx2;

            unsigned int nrmlIdx0;
            unsigned int nrmlIdx1;
            unsigned int nrmlIdx2;

#if (_MSC_VER >= 1400)
            int matches = fscanf_s(
                file,
                "%s %s %s\n",
                faceCoord0,
                FaceCoordStrSize,
                faceCoord1,
                FaceCoordStrSize,
                faceCoord2,
                FaceCoordStrSize
            );
#else
            int matches = fscanf(file, "%s %s %s\n", faceCoord0, faceCoord1, faceCoord2);
#endif
            if (matches != 3) {
                printf("Parser can only read simple triangle meshes are supported\n");
                return false;
            }
            else {
#if (_MSC_VER >= 1400)
                // vertex + uv + normal valid
                if (sscanf_s(faceCoord0, "%d/%d/%d", &vIdx0, &uvIdx0, &nrmlIdx0) == 3)
#else
                // vertex + uv + normal valid
                if (sscanf(faceCoord0, "%d/%d/%d", &vIdx0, &uvIdx0, &nrmlIdx0) == 3)
#endif
                {
#if (_MSC_VER >= 1400)
                    if (sscanf_s(faceCoord1, "%d/%d/%d", &vIdx1, &uvIdx1, &nrmlIdx1) != 3)
#else
                    if (sscanf(faceCoord1, "%d/%d/%d", &vIdx1, &uvIdx1, &nrmlIdx1) != 3)
#endif
                    {
                        printf("Parser can't read 2nd face indexes\n");
                        return false;
                    }
                    
#if (_MSC_VER >= 1400)
                    if (sscanf_s(faceCoord2, "%d/%d/%d", &vIdx2, &uvIdx2, &nrmlIdx2) != 3)
#else
                    if (sscanf(faceCoord2, "%d/%d/%d", &vIdx2, &uvIdx2, &nrmlIdx2) != 3)
#endif
                    {
                        printf("Parser can't read 3rd face indexes\n");
                        return false;
                    }

                    vertexIndices.push_back(vIdx0);
                    vertexIndices.push_back(vIdx1);
                    vertexIndices.push_back(vIdx2);
                    uvIndices    .push_back(uvIdx0);
                    uvIndices    .push_back(uvIdx1);
                    uvIndices    .push_back(uvIdx2);
                    normalIndices.push_back(nrmlIdx0);
                    normalIndices.push_back(nrmlIdx1);
                    normalIndices.push_back(nrmlIdx2);
                }
#if (_MSC_VER >= 1400)
                // vertex + normal valid
                else if (sscanf_s(faceCoord0, "%d//%d", &vIdx0, &nrmlIdx0) == 2)
#else
                // vertex + normal valid
                else if (sscanf(faceCoord0, "%d//%d", &vIdx0, &nrmlIdx0) == 2)
#endif
                {
#if (_MSC_VER >= 1400)
                    if (sscanf_s(faceCoord1, "%d//%d", &vIdx1, &nrmlIdx1) != 2)
#else
                    if (sscanf(faceCoord1, "%d//%d", &vIdx1, &nrmlIdx1) != 2)
#endif
                    {
                        printf("Parser can't read 2nd face indexes\n");
                        return false;
                    }
                    
#if (_MSC_VER >= 1400)
                    if (sscanf_s(faceCoord2, "%d//%d", &vIdx2, &nrmlIdx2) != 2)
#else
                    if (sscanf(faceCoord2, "%d//%d", &vIdx2, &nrmlIdx2) != 2)
#endif
                    {
                        printf("Parser can't read 3rd face indexes\n");
                        return false;
                    }

                    vertexIndices.push_back(vIdx0);
                    vertexIndices.push_back(vIdx1);
                    vertexIndices.push_back(vIdx2);
                    normalIndices.push_back(nrmlIdx0);
                    normalIndices.push_back(nrmlIdx1);
                    normalIndices.push_back(nrmlIdx2);
                }

#if (_MSC_VER >= 1400)
                // vertex + uv valid
                else if (sscanf_s(faceCoord0, "%d/%d//", &vIdx0, &uvIdx0) == 2)
#else
                else if (sscanf(faceCoord0, "%d/%d//", &vIdx0, &uvIdx0) == 2)
#endif
                {
#if (_MSC_VER >= 1400)
                    if (sscanf_s(faceCoord1, "%d/%d//", &vIdx1, &uvIdx1) != 2)
#else
                    if( sscanf( faceCoord1, "%d/%d//", &vIdx1, &uvIdx1) != 2)
#endif
                    {
                        printf("Parser can't read 2nd face indexes\n");
                        return false;
                    }
                    
#if (_MSC_VER >= 1400)
                    if (sscanf_s(faceCoord2, "%d/%d//", &vIdx2, &uvIdx2) != 2)
#else
                    if (sscanf(faceCoord2, "%d/%d//", &vIdx2, &uvIdx2) != 2)
#endif
                    {
                        printf("Parser can't read 3rd face indexes\n");
                        return false;
                    }

                    vertexIndices.push_back(vIdx0);
                    vertexIndices.push_back(vIdx1);
                    vertexIndices.push_back(vIdx2);
                    uvIndices    .push_back(uvIdx0);
                    uvIndices    .push_back(uvIdx1);
                    uvIndices    .push_back(uvIdx2);
                }

#if (_MSC_VER >= 1400)
                // vertex only
                else if (sscanf_s(faceCoord0, "%d///", &vIdx0) == 1)
#else
                // vertex only
                else if (sscanf(faceCoord0, "%d///", &vIdx0) == 1)
#endif
                {
                    
#if (_MSC_VER >= 1400)
                    if (sscanf_s(faceCoord1, "%d///", &vIdx1) != 1)
#else
                    if (sscanf(faceCoord1, "%d///", &vIdx1) != 1)
#endif
                    {
                        printf("Parser can't read 2nd face indexes\n");
                        return false;
                    }
                    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
                    if (sscanf_s(faceCoord2, "%d///", &vIdx2) != 1)
#else
                    if (sscanf(faceCoord2, "%d///", &vIdx2) != 1)
#endif
                    {
                        printf("Parser can't read 3rd face indexes.\n");
                        return false;
                    }

                    vertexIndices.push_back(vIdx0);
                    vertexIndices.push_back(vIdx1);
                    vertexIndices.push_back(vIdx2);
                }
                else  {
                    printf("File can't be read by our parser. Face format is unknown.\n");
                    return false;
                }
            }
        } else {
            // Probably a comment, eat up the rest of the line
            char StupidBuffer[1000];
            fgets(StupidBuffer, 1000, file);
        }

    }

    // For each vertex of each triangle
    for (unsigned int i = 0; i < vertexIndices.size(); i++) {
        // Get the indices of its attributes
        unsigned int vertexIndex = vertexIndices[i];

        // Get the attributes thanks to the index
        glm::vec3 vertex = tmp.vertices[vertexIndex - 1];
        vertices.push_back(vertex);

        if (!uvIndices.empty()) {
            unsigned int uvIndex = uvIndices[i];
            glm::vec2 uv = tmp.uvs[uvIndex - 1];
            uvs.push_back(uv);
        }

        if (!normalIndices.empty()) {
            unsigned int normalIndex = normalIndices[i];
            glm::vec3 normal = tmp.normals[normalIndex - 1];
            normals.push_back(normal);
        }
    }

    return true;
}
