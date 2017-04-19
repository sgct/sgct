/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_INTERNAL_SPHERICAL_PROJECTION_SHADERS_H_
#define _SGCT_INTERNAL_SPHERICAL_PROJECTION_SHADERS_H_

#include <string>

namespace sgct_core
{
    /*
        All shaders are in GLSL 1.2 for compability with Mac OS X
    */

    namespace shaders
    {
        const std::string Spherical_Projection_Vert_Shader = "\
            **glsl_version**\n\
            \n\
            void main()\n\
            {\n\
                gl_TexCoord[0] = gl_MultiTexCoord0;\n\
                gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
                gl_FrontColor = gl_Color;\n\
            }\n";

        const std::string Spherical_Projection_Frag_Shader = "\
            **glsl_version**\n\
            \n\
            uniform sampler2D Tex;\n\
            \n\
            void main()\n\
            {\n\
                gl_FragColor = gl_Color * texture2D(Tex, gl_TexCoord[0].st);\n\
            }\n";

    }//end shaders
}
#endif
