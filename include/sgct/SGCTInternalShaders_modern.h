/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_INTERNAL_SHADERS_MODERN_H_
#define _SGCT_INTERNAL_SHADERS_MODERN_H_

#include <string>

namespace sgct_core
{
	/*
		Contains GLSL 3.3+ shaders
	*/
	namespace shaders_modern
	{
		const std::string TexPos_Vert_Shader = "\
			#version 330 core\n\
			\n\
			layout (location = 0) in vec2 Position;\n\
			layout (location = 1) in vec2 LeftUV;\n\
			layout (location = 2) in vec2 RightUV;\n\
			layout (location = 3) in vec3 Color;\n\
			\n\
			uniform mat4 MVP;\n\
			out vec2 UV;\n\
			out vec3 Col;\n\
			\n\
			void main()\n\
			{\n\
			   gl_Position = MVP * vec4(Position, 0.0, 1.0);\n\
			   UV = LeftUV;\n\
			   Col = Color;\n\
			};\n";

		const std::string TexPos_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec3 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			\n\
			void main()\n\
			{\n\
				Color = texture2D(LeftTex, UV.st) * vec4(Col, 1.0);\n\
			};\n";
	}
}
#endif
