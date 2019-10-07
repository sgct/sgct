/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef __SGCT__BOX__H__
#define __SGCT__BOX__H__

/**
 * \namespace sgct::utils
 * \brief SGCT utils namespace contains basic utilities for geometry rendering
 */
namespace sgct::utils {

/**
 * This class creates and renders a textured box.
 */
class Box {
public:
    enum class TextureMappingMode { Regular = 0, CubeMap, SkyBox };

    /// This constructor requires a valid openGL context
    Box(float size, TextureMappingMode mode = TextureMappingMode::Regular);
    ~Box();

    /**
     * If openGL 3.3+ is used:
     *   layout 0 contains texture coordinates (vec2)
     *   layout 1 contains vertex normals (vec3)
     *   layout 2 contains vertex positions (vec3).
     */
     void draw();

private:
    void drawVBO();
    void drawVAO();
    void createVBO(float size, TextureMappingMode tmm);

    unsigned int _vao = 0;
    unsigned int _vbo = 0;
};

} // namespace sgct_utils

#endif // __SGCT__BOX__H__
