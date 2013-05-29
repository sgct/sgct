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
		const std::string Base_Vert_Shader = "\
			#version 330 core\n\
			\n\
			layout (location = 0) in vec2 Position;\n\
			layout (location = 1) in vec2 TexCoords;\n\
			layout (location = 2) in vec3 VertColor;\n\
			\n\
			uniform mat4 MVP;\n\
			out vec2 UV;\n\
			out vec4 Col;\n\
			\n\
			void main()\n\
			{\n\
			   gl_Position = MVP * vec4(Position, 0.0, 1.0);\n\
			   UV = TexCoords;\n\
			   Col = vec4(VertColor, 1.0);\n\
			};\n";

		const std::string Base_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			\n\
			void main()\n\
			{\n\
				Color = Col * texture2D(LeftTex, UV.st);\n\
			};\n";

		const std::string Anaglyph_Vert_Shader = "\
			#version 330 core\n\
			\n\
			layout (location = 0) in vec2 Position;\n\
			layout (location = 1) in vec2 TexCoords;\n\
			layout (location = 2) in vec3 VertColor;\n\
			\n\
			uniform mat4 MVP;\n\
			out vec2 UV;\n\
			out vec4 Col;\n\
			\n\
			void main()\n\
			{\n\
			   gl_Position = MVP * vec4(Position, 0.0, 1.0);\n\
			   UV = TexCoords;\n\
			   Col = vec4(VertColor, 1.0);\n\
			};\n";

		const std::string Anaglyph_Red_Cyan_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			\n\
			void main()\n\
			{\n\
				vec4 leftVals = texture2D( LeftTex, UV.st);\n\
				float leftLum = 0.3 * leftVals.r + 0.59 * leftVals.g + 0.11 * leftVals.b;\n\
				vec4 rightVals = texture2D( RightTex, UV.st);\n\
				float rightLum = 0.3 * rightVals.r + 0.59 * rightVals.g + 0.11 * rightVals.b;\n\
				Color.r = Col.r * leftLum;\n\
				Color.g = Col.g * rightLum;\n\
				Color.b = Col.b * rightLum;\n\
				Color.a = Col.a * max(leftVals.a, rightVals.a);\n\
			};\n";

		const std::string Anaglyph_Red_Cyan_Frag_Shader_Wimmer = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			\n\
			void main()\n\
			{\n\
				vec4 leftVals = texture2D( LeftTex, UV.st);\n\
				vec4 rightVals = texture2D( RightTex, UV.st);\n\
				Color.r = Col.r * (0.7*leftVals.g + 0.3*leftVals.b);\n\
				Color.g = Col.g * rightVals.r;\n\
				Color.b = Col.b * rightVals.b;\n\
				Color.a = Col.a * max(leftVals.a, rightVals.a);\n\
			};\n";

		const std::string Anaglyph_Amber_Blue_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			\n\
			void main()\n\
			{\n\
				vec4 leftVals = texture2D( LeftTex, UV.st);\n\
				vec4 rightVals = texture2D( RightTex, UV.st);\n\
				vec3 coef = vec3(0.15, 0.15, 0.70);\n\
				float rightMix = dot(rightVals.rbg, coef);\n\
				Color.r = Col.r * leftVals.r;\n\
				Color.g = Col.g * leftVals.g;\n\
				Color.b = Col.b * rightMix;\n\
				Color.a = Col.a * max(leftVals.a, rightVals.a);\n\
			};\n";

		const std::string CheckerBoard_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			\n\
			void main()\n\
			{\n\
				float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;\n\
				if( (fval - floor(fval)) == 0.0 )\n\
					Color = Col * texture2D( RightTex, UV.st);\n\
				else\n\
					Color = Col * texture2D( LeftTex, UV.st);\n\
			};\n";

		const std::string CheckerBoard_Inverted_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			\n\
			void main()\n\
			{\n\
				float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;\n\
				if( (fval - floor(fval)) == 0.0 )\n\
					Color = Col * texture2D( LeftTex, UV.st);\n\
				else\n\
					Color = Col * texture2D( RightTex, UV.st);\n\
			};\n";

		const std::string Vertical_Interlaced_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			\n\
			void main()\n\
			{\n\
				float fval = gl_FragCoord.y * 0.5;\n\
				if( (fval - floor(fval)) > 0.5 )\n\
					Color = Col * texture2D( RightTex, UV.st);\n\
				else\n\
					Color = Col * texture2D( LeftTex, UV.st);\n\
			};\n";

		const std::string Vertical_Interlaced_Inverted_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			\n\
			void main()\n\
			{\n\
				float fval = gl_FragCoord.y * 0.5;\n\
				if( (fval - floor(fval)) > 0.5 )\n\
					Color = Col * texture2D( LeftTex, UV.st);\n\
				else\n\
					Color = Col * texture2D( RightTex, UV.st);\n\
			};\n";

		const std::string Dummy_Stereo_Frag_Shader = "\
			#version 330 core\n\
			\n\
			in vec2 UV;\n\
			in vec4 Col;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			\n\
			void main()\n\
			{\n\
				Color = Col * (0.5 * texture2D( LeftTex, UV.st) + 0.5 * texture2D( RightTex, UV.st));\n\
			};\n";
	}
}
#endif
