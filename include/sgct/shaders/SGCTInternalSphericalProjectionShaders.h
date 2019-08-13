/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef __SGCT__INTERNAL_SPHERICAL_PROJECTION_SHADERS__H__
#define __SGCT__INTERNAL_SPHERICAL_PROJECTION_SHADERS__H__

namespace sgct_core::shaders {
/*
    All shaders are in GLSL 1.2 for compability with Mac OS X
*/

constexpr const char* Spherical_Projection_Vert_Shader = R"(
    **glsl_version**

    void main() {
        gl_TexCoord[0] = gl_MultiTexCoord0;
        gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
        gl_FrontColor = gl_Color;
    }
)";

constexpr const char* Spherical_Projection_Frag_Shader = R"(
    **glsl_version**

    uniform sampler2D Tex;

    void main() {
        gl_FragColor = gl_Color * texture2D(Tex, gl_TexCoord[0].st);
    }
)";

} // sgct_core::shaders

#endif // __SGCT__INTERNAL_SPHERICAL_PROJECTION_SHADERS__H__
