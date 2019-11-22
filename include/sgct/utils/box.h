/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifndef __SGCT__BOX__H__
#define __SGCT__BOX__H__

namespace sgct::utils {

/// This class creates and renders a textured box.
class Box {
public:
    enum class TextureMappingMode { Regular = 0, CubeMap, SkyBox };

    /// This constructor requires a valid OpenGL context
    Box(float size, TextureMappingMode mode = TextureMappingMode::Regular);

    /// The destructor requires a valid OpenGL context
    ~Box();

     void draw();

private:
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
};

} // namespace sgct::utils

#endif // __SGCT__BOX__H__
