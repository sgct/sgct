#include <vector>
#include <stdio.h>
#include <string>
#include <cstring>

#include <glm/glm.hpp>

#include "objloader.hpp"

// Very, VERY simple OBJ loader.
// Here is a short list of features a real function would provide : 
// - Binary files. Reading a model should be just a few memcpy's away, not parsing a file at runtime. In short : OBJ is not very great.
// - Animations & bones (includes bones weights)
// - Multiple UVs
// - All attributes should be optional, not "forced"
// - More stable. Change a line in the OBJ file and it crashes.
// - More secure. Change another line and you can inject code.
// - Loading from memory, stream, etc

bool loadOBJ(
    const char * path, 
    std::vector<glm::vec3> & out_vertices, 
    std::vector<glm::vec2> & out_uvs,
    std::vector<glm::vec3> & out_normals
){
    printf("Loading OBJ file %s...\n", path);

    std::vector<unsigned int> vertexIndices, uvIndices, normalIndices;
    std::vector<glm::vec3> temp_vertices; 
    std::vector<glm::vec2> temp_uvs;
    std::vector<glm::vec3> temp_normals;

#if (_MSC_VER >= 1400) //visual studio 2005 or later
    errno_t err;
    FILE * file;
    err = fopen_s(&file, path, "r");
    if (err != 0)
#else
    FILE * file = fopen(path, "r");
    if (file == NULL)
#endif
    {
        printf("Impossible to open the file! Are you in the right path?\n");
        return false;
    }

    const int lineheaderStrSize = 128;
    const int faceCoordStrSize = 64;

    char lineHeader[lineheaderStrSize];

    //holders for face coord data
    //supports only triangles (3 values)
    //fix to read 3DsMax obj exported files by Miro
    char faceCoord0[faceCoordStrSize];
    char faceCoord1[faceCoordStrSize];
    char faceCoord2[faceCoordStrSize];

    while( true )
    {
        // read the first word of the line
#if (_MSC_VER >= 1400) //visual studio 2005 or later
        int res = fscanf_s(file, "%s", lineHeader, lineheaderStrSize);
#else
        int res = fscanf(file, "%s", lineHeader);
#endif
        if (res == EOF)
            break; // EOF = End Of File. Quit the loop.

        // else : parse lineHeader
        
        if ( strcmp( lineHeader, "v" ) == 0 )
        {
            glm::vec3 vertex;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
            fscanf_s(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
#else
            fscanf(file, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z );
#endif
            temp_vertices.push_back(vertex);
        }
        else if ( strcmp( lineHeader, "vt" ) == 0 )
        {
            glm::vec2 uv;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
            fscanf_s(file, "%f %f\n", &uv.x, &uv.y );
#else
            fscanf(file, "%f %f\n", &uv.x, &uv.y );
#endif
            temp_uvs.push_back(uv);
        }
        else if ( strcmp( lineHeader, "vn" ) == 0 )
        {
            glm::vec3 normal;
#if (_MSC_VER >= 1400) //visual studio 2005 or later
            fscanf_s(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
#else
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
#endif
            temp_normals.push_back(normal);
        }
        else if ( strcmp( lineHeader, "f" ) == 0 )
        {
            std::string vertex1, vertex2, vertex3;
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            /*int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &vertexIndex[0], &uvIndex[0], &normalIndex[0], &vertexIndex[1], &uvIndex[1], &normalIndex[1], &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
            if (matches != 9){
                printf("File can't be read by our simple parser :-( Try exporting with other options\n");
                return false;
            }*/

#if (_MSC_VER >= 1400) //visual studio 2005 or later
            int matches = fscanf_s(file, "%s %s %s\n", faceCoord0, faceCoordStrSize, faceCoord1, faceCoordStrSize, faceCoord2, faceCoordStrSize);
#else
            int matches = fscanf(file, "%s %s %s\n", faceCoord0, faceCoord1, faceCoord2 );
#endif
            if (matches != 3)
            {
                printf("File can't be read by our simple parser. Only triangle meshes can be read.\n");
                return false;
            }
            else
            {

#if (_MSC_VER >= 1400) //visual studio 2005 or later
                if( sscanf_s( faceCoord0, "%d/%d/%d", &vertexIndex[0], &uvIndex[0], &normalIndex[0] ) == 3 ) //vertex + uv + normal valid
#else
                if (sscanf(faceCoord0, "%d/%d/%d", &vertexIndex[0], &uvIndex[0], &normalIndex[0]) == 3) //vertex + uv + normal valid
#endif
                {

#if (_MSC_VER >= 1400) //visual studio 2005 or later
                    if( sscanf_s( faceCoord1, "%d/%d/%d", &vertexIndex[1], &uvIndex[1], &normalIndex[1] ) != 3 )
#else
                    if (sscanf(faceCoord1, "%d/%d/%d", &vertexIndex[1], &uvIndex[1], &normalIndex[1]) != 3)
#endif
                    {
                        printf("File can't be read by our simple parser. Can't read 2nd face indexes.\n");
                        return false;
                    }
                    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
                    if( sscanf_s( faceCoord2, "%d/%d/%d", &vertexIndex[2], &uvIndex[2], &normalIndex[2] ) != 3 )
#else
                    if (sscanf(faceCoord2, "%d/%d/%d", &vertexIndex[2], &uvIndex[2], &normalIndex[2]) != 3)
#endif
                    {
                        printf("File can't be read by our simple parser. Can't read 3rd face indexes.\n");
                        return false;
                    }

                    vertexIndices.push_back(vertexIndex[0]);
                    vertexIndices.push_back(vertexIndex[1]);
                    vertexIndices.push_back(vertexIndex[2]);
                    uvIndices    .push_back(uvIndex[0]);
                    uvIndices    .push_back(uvIndex[1]);
                    uvIndices    .push_back(uvIndex[2]);
                    normalIndices.push_back(normalIndex[0]);
                    normalIndices.push_back(normalIndex[1]);
                    normalIndices.push_back(normalIndex[2]);
                }
#if (_MSC_VER >= 1400) //visual studio 2005 or later
                else if( sscanf_s( faceCoord0, "%d//%d", &vertexIndex[0], &normalIndex[0] ) == 2 ) //vertex + normal valid
#else
                else if( sscanf( faceCoord0, "%d//%d", &vertexIndex[0], &normalIndex[0] ) == 2 ) //vertex + normal valid
#endif
                {

#if (_MSC_VER >= 1400) //visual studio 2005 or later
                    if( sscanf_s( faceCoord1, "%d//%d", &vertexIndex[1], &normalIndex[1] ) != 2 )
#else
                    if (sscanf(faceCoord1, "%d//%d", &vertexIndex[1], &normalIndex[1]) != 2)
#endif
                    {
                        printf("File can't be read by our simple parser. Can't read 2nd face indexes.\n");
                        return false;
                    }
                    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
                    if( sscanf_s( faceCoord2, "%d//%d", &vertexIndex[2], &normalIndex[2] ) != 2 )
#else
                    if( sscanf( faceCoord2, "%d//%d", &vertexIndex[2], &normalIndex[2] ) != 2 )
#endif
                    {
                        printf("File can't be read by our simple parser. Can't read 3rd face indexes.\n");
                        return false;
                    }

                    vertexIndices.push_back(vertexIndex[0]);
                    vertexIndices.push_back(vertexIndex[1]);
                    vertexIndices.push_back(vertexIndex[2]);
                    normalIndices.push_back(normalIndex[0]);
                    normalIndices.push_back(normalIndex[1]);
                    normalIndices.push_back(normalIndex[2]);
                }

#if (_MSC_VER >= 1400) //visual studio 2005 or later
                else if( sscanf_s( faceCoord0, "%d/%d//", &vertexIndex[0], &uvIndex[0] ) == 2 ) //vertex + uv valid
#else
                else if( sscanf( faceCoord0, "%d/%d//", &vertexIndex[0], &uvIndex[0] ) == 2 ) //vertex + uv valid
#endif
                {
                    
#if (_MSC_VER >= 1400) //visual studio 2005 or later    
                    if( sscanf_s( faceCoord1, "%d/%d//", &vertexIndex[1], &uvIndex[1] ) != 2 )
#else
                    if( sscanf( faceCoord1, "%d/%d//", &vertexIndex[1], &uvIndex[1] ) != 2 )
#endif
                    {
                        printf("File can't be read by our simple parser. Can't read 2nd face indexes.\n");
                        return false;
                    }
                    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
                    if( sscanf_s( faceCoord2, "%d/%d//", &vertexIndex[2], &uvIndex[2] ) != 2 )
#else
                    if( sscanf( faceCoord2, "%d/%d//", &vertexIndex[2], &uvIndex[2] ) != 2 )
#endif
                    {
                        printf("File can't be read by our simple parser. Can't read 3rd face indexes.\n");
                        return false;
                    }

                    vertexIndices.push_back(vertexIndex[0]);
                    vertexIndices.push_back(vertexIndex[1]);
                    vertexIndices.push_back(vertexIndex[2]);
                    uvIndices    .push_back(uvIndex[0]);
                    uvIndices    .push_back(uvIndex[1]);
                    uvIndices    .push_back(uvIndex[2]);
                }

#if (_MSC_VER >= 1400) //visual studio 2005 or later
                else if( sscanf_s( faceCoord0, "%d///", &vertexIndex[0] ) == 1 ) //vertex only
#else
                else if (sscanf(faceCoord0, "%d///", &vertexIndex[0]) == 1) //vertex only
#endif
                {
                    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
                    if( sscanf_s( faceCoord1, "%d///", &vertexIndex[1] ) != 1 )
#else
                    if (sscanf(faceCoord1, "%d///", &vertexIndex[1]) != 1)
#endif
                    {
                        printf("File can't be read by our simple parser. Can't read 2nd face indexes.\n");
                        return false;
                    }
                    
#if (_MSC_VER >= 1400) //visual studio 2005 or later
                    if( sscanf_s( faceCoord2, "%d///", &vertexIndex[2] ) != 1 )
#else
                    if( sscanf( faceCoord2, "%d///", &vertexIndex[2] ) != 1 )
#endif
                    {
                        printf("File can't be read by our simple parser. Can't read 3rd face indexes.\n");
                        return false;
                    }

                    vertexIndices.push_back(vertexIndex[0]);
                    vertexIndices.push_back(vertexIndex[1]);
                    vertexIndices.push_back(vertexIndex[2]);
                }
                else //fail
                {
                    printf("File can't be read by our simple parser. Face format is unknown.\n");
                    return false;
                }
            }
        }else{
            // Probably a comment, eat up the rest of the line
            char stupidBuffer[1000];
            fgets(stupidBuffer, 1000, file);
        }

    }

    // For each vertex of each triangle
    for( unsigned int i=0; i<vertexIndices.size(); i++ )
    {
        // Get the indices of its attributes
        unsigned int vertexIndex = vertexIndices[i];
        unsigned int uvIndex;
        unsigned int normalIndex;

        // Get the attributes thanks to the index
        glm::vec3 vertex = temp_vertices[ vertexIndex-1 ];
        out_vertices.push_back(vertex);

        if( uvIndices.size() > 0 )
        {
            uvIndex = uvIndices[i];
            glm::vec2 uv = temp_uvs[ uvIndex-1 ];
            out_uvs.push_back(uv);
        }

        if( normalIndices.size() > 0 )
        {
            normalIndex = normalIndices[i];
            glm::vec3 normal = temp_normals[ normalIndex-1 ];
            out_normals.push_back(normal);
        }
    }

    return true;
}


#ifdef USE_ASSIMP // don't use this #define, it's only for me (it AssImp fails to compile on your machine, at least all the other tutorials still work)

// Include AssImp
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>           // Output data structure
#include <assimp/postprocess.h>     // Post processing flags

bool loadAssImp(
    const char * path, 
    std::vector<unsigned short> & indices,
    std::vector<glm::vec3> & vertices,
    std::vector<glm::vec2> & uvs,
    std::vector<glm::vec3> & normals
){

    Assimp::Importer importer;

    const aiScene* scene = importer.ReadFile(path, 0/*aiProcess_JoinIdenticalVertices | aiProcess_SortByPType*/);
    if( !scene) {
        fprintf( stderr, importer.GetErrorString());
        return false;
    }
    const aiMesh* mesh = scene->mMeshes[0]; // In this simple example code we always use the 1rst mesh (in OBJ files there is often only one anyway)

    // Fill vertices positions
    vertices.reserve(mesh->mNumVertices);
    for(unsigned int i=0; i<mesh->mNumVertices; i++){
        aiVector3D pos = mesh->mVertices[i];
        vertices.push_back(glm::vec3(pos.x, pos.y, pos.z));
    }

    // Fill vertices texture coordinates
    uvs.reserve(mesh->mNumVertices);
    for(unsigned int i=0; i<mesh->mNumVertices; i++){
        aiVector3D UVW = mesh->mTextureCoords[0][i]; // Assume only 1 set of UV coords; AssImp supports 8 UV sets.
        uvs.push_back(glm::vec2(UVW.x, UVW.y));
    }

    // Fill vertices normals
    normals.reserve(mesh->mNumVertices);
    for(unsigned int i=0; i<mesh->mNumVertices; i++){
        aiVector3D n = mesh->mNormals[i];
        normals.push_back(glm::vec3(n.x, n.y, n.z));
    }


    // Fill face indices
    indices.reserve(3*mesh->mNumFaces);
    for (unsigned int i=0; i<mesh->mNumFaces; i++){
        // Assume the model has only triangles.
        indices.push_back(mesh->mFaces[i].mIndices[0]);
        indices.push_back(mesh->mFaces[i].mIndices[1]);
        indices.push_back(mesh->mFaces[i].mIndices[2]);
    }
    
    // The "scene" pointer will be deleted automatically by "importer"

}

#endif