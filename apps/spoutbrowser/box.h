/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifndef __SGCT__BOX__H__
#define __SGCT__BOX__H__

/**
 * This class creates and renders a textured box.
 */
class Box {
public:
    enum class TextureMappingMode { Regular = 0, CubeMap, SkyBox };

    /**
     * This constructor requires a valid OpenGL context.
     */
    Box(float size, TextureMappingMode mode = TextureMappingMode::Regular);

    /**
     * The destructor requires a valid OpenGL context.
     */
    ~Box();

     void draw() const;

private:
    unsigned int _vao = 0;
    unsigned int _vbo = 0;
};

#endif // __SGCT__BOX__H__
