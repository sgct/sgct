/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_INTERNAL_SPHERICAL_PROJECTION_SHADERS_MODERN_H_
#define _SGCT_INTERNAL_SPHERICAL_PROJECTION_SHADERS_MODERN_H_

#include <string>

namespace sgct_core
{
	/*
		Contains GLSL 3.3+ shaders
	*/

	namespace shaders_modern
	{
		const std::string Spherical_Projection_Vert_Shader = "\
			**glsl_version**\n\
			\n\
			layout (location = 0) in vec2 Position;\n\
			layout (location = 1) in vec2 TexCoords;\n\
			layout (location = 2) in vec4 VertColor;\n\
			\n\
			uniform mat4 MVP;\n\
			\n\
			out vec2 UV;\n\
			out vec4 Col;\n\
			\n\
			void main()\n\
			{\n\
			   gl_Position = MVP * vec4(Position, 0.0, 1.0);\n\
			   UV = TexCoords;\n\
			   Col = VertColor;\n\
			}\n";

		const std::string Spherical_Projection_Frag_Shader = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D Tex;\n\
			\n\
			void main()\n\
			{\n\
				Color = Col * texture(Tex, UV);\n\
			}\n";
	}
}
#endif
