/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
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
			**glsl_version**\n\
			\n\
			layout (location = 0) in vec2 Position;\n\
			layout (location = 1) in vec2 TexCoords;\n\
			layout (location = 2) in vec4 VertColor;\n\
			\n\
			out vec2 UV;\n\
			out vec4 Col;\n\
			\n\
			void main()\n\
			{\n\
			   gl_Position = vec4(Position, 0.0, 1.0);\n\
			   UV = TexCoords;\n\
			   Col = VertColor;\n\
			}\n";

		const std::string Base_Frag_Shader = "\
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

		const std::string Overlay_Vert_Shader = "\
			**glsl_version**\n\
			\n\
			layout (location = 0) in vec2 TexCoords;\n\
			layout (location = 1) in vec3 Position;\n\
			\n\
			out vec2 UV;\n\
			\n\
			void main()\n\
			{\n\
			   gl_Position = vec4(Position, 1.0);\n\
			   UV = TexCoords;\n\
			}\n";

		const std::string Overlay_Frag_Shader = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
			out vec4 Color;\n\
			\n\
			uniform sampler2D Tex;\n\
			\n\
			void main()\n\
			{\n\
				Color = texture(Tex, UV);\n\
			}\n";

		const std::string Anaglyph_Vert_Shader = "\
			**glsl_version**\n\
			\n\
			layout (location = 0) in vec2 Position;\n\
			layout (location = 1) in vec2 TexCoords;\n\
			layout (location = 2) in vec3 VertColor;\n\
			\n\
			out vec2 UV;\n\
			out vec4 Col;\n\
			\n\
			void main()\n\
			{\n\
			   gl_Position = vec4(Position, 0.0, 1.0);\n\
			   UV = TexCoords;\n\
			   Col = vec4(VertColor, 1.0);\n\
			}\n";

		const std::string Anaglyph_Red_Cyan_Stereo_Frag_Shader = "\
			**glsl_version**\n\
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
				vec4 leftVals = texture( LeftTex, UV);\n\
				float leftLum = 0.3 * leftVals.r + 0.59 * leftVals.g + 0.11 * leftVals.b;\n\
				vec4 rightVals = texture( RightTex, UV);\n\
				float rightLum = 0.3 * rightVals.r + 0.59 * rightVals.g + 0.11 * rightVals.b;\n\
				Color.r = Col.r * leftLum;\n\
				Color.g = Col.g * rightLum;\n\
				Color.b = Col.b * rightLum;\n\
				Color.a = Col.a * max(leftVals.a, rightVals.a);\n\
			}\n";

		const std::string Anaglyph_Red_Cyan_Stereo_Frag_Shader_Wimmer = "\
			**glsl_version**\n\
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
				vec4 leftVals = texture( LeftTex, UV);\n\
				vec4 rightVals = texture( RightTex, UV);\n\
				Color.r = Col.r * (0.7*leftVals.g + 0.3*leftVals.b);\n\
				Color.g = Col.g * rightVals.r;\n\
				Color.b = Col.b * rightVals.b;\n\
				Color.a = Col.a * max(leftVals.a, rightVals.a);\n\
			}\n";

		const std::string Anaglyph_Amber_Blue_Stereo_Frag_Shader = "\
			**glsl_version**\n\
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
				vec4 leftVals = texture( LeftTex, UV);\n\
				vec4 rightVals = texture( RightTex, UV);\n\
				vec3 coef = vec3(0.15, 0.15, 0.70);\n\
				float rightMix = dot(rightVals.rbg, coef);\n\
				Color.r = Col.r * leftVals.r;\n\
				Color.g = Col.g * leftVals.g;\n\
				Color.b = Col.b * rightMix;\n\
				Color.a = Col.a * max(leftVals.a, rightVals.a);\n\
			}\n";

		const std::string CheckerBoard_Frag_Shader = "\
			**glsl_version**\n\
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
					Color = Col * texture( RightTex, UV);\n\
				else\n\
					Color = Col * texture( LeftTex, UV);\n\
			}\n";

		const std::string CheckerBoard_Inverted_Frag_Shader = "\
			**glsl_version**\n\
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
					Color = Col * texture( LeftTex, UV);\n\
				else\n\
					Color = Col * texture( RightTex, UV);\n\
			}\n";

		const std::string Vertical_Interlaced_Stereo_Frag_Shader = "\
			**glsl_version**\n\
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
					Color = Col * texture( RightTex, UV);\n\
				else\n\
					Color = Col * texture( LeftTex, UV);\n\
			}\n";

		const std::string Vertical_Interlaced_Inverted_Stereo_Frag_Shader = "\
			**glsl_version**\n\
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
					Color = Col * texture( LeftTex, UV);\n\
				else\n\
					Color = Col * texture( RightTex, UV);\n\
			}\n";

		const std::string SBS_Stereo_Frag_Shader = "\
			**glsl_version**\n\
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
				vec2 mul = vec2(2.0, 1.0); \n\
				if( UV.s < 0.5 ) \n\
					Color = Col * texture( LeftTex, UV.st * mul); \n\
				else \n\
					Color = Col * texture( RightTex, UV.st * mul - vec2(1.0, 0.0)); \n\
			}\n";

		const std::string SBS_Stereo_Inverted_Frag_Shader = "\
			**glsl_version**\n\
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
				vec2 mul = vec2(2.0, 1.0); \n\
				if( UV.s < 0.5 ) \n\
					Color = Col * texture( RightTex, UV.st * mul); \n\
				else \n\
					Color = Col * texture( LeftTex, UV.st * mul - vec2(1.0, 0.0)); \n\
			}\n";

		const std::string TB_Stereo_Frag_Shader = "\
			**glsl_version**\n\
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
				vec2 mul = vec2(1.0, 2.0); \n\
				if( UV.t < 0.5 ) \n\
					Color = Col * texture( RightTex, UV.st * mul); \n\
				else \n\
					Color = Col * texture( LeftTex, UV.st * mul - vec2(0.0, 1.0)); \n\
			}\n";

		const std::string TB_Stereo_Inverted_Frag_Shader = "\
			**glsl_version**\n\
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
				vec2 mul = vec2(1.0, 2.0); \n\
				if( UV.t < 0.5 ) \n\
					Color = Col * texture( LeftTex, UV.st * mul); \n\
				else \n\
					Color = Col * texture( RightTex, UV.st * mul - vec2(0.0, 1.0)); \n\
			}\n";

		const std::string Dummy_Stereo_Frag_Shader = "\
			**glsl_version**\n\
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
				Color = Col * (0.5 * texture( LeftTex, UV) + 0.5 * texture( RightTex, UV));\n\
			}\n";

		const std::string FXAA_Vert_Shader = "\
			**glsl_version**\n\
			\n\
			layout (location = 0) in vec2 TexCoords;\n\
			layout (location = 1) in vec3 Position;\n\
			\n\
			uniform float rt_w;\n\
			uniform float rt_h;\n\
			uniform float FXAA_SUBPIX_OFFSET; \n\
			\n\
			out vec2 UVCoord;\n\
			out vec2 texcoordOffset[4];\n\
			\n\
			void main(void)\n\
			{\n\
				gl_Position = vec4(Position, 1.0);\n\
				UVCoord = TexCoords;\n\
				\n\
				texcoordOffset[0] = UVCoord + FXAA_SUBPIX_OFFSET * vec2(-1.0/rt_w,  -1.0/rt_h);\n\
				texcoordOffset[1] = UVCoord + FXAA_SUBPIX_OFFSET * vec2( 1.0/rt_w,  -1.0/rt_h);\n\
				texcoordOffset[2] = UVCoord + FXAA_SUBPIX_OFFSET * vec2(-1.0/rt_w,  1.0/rt_h);\n\
				texcoordOffset[3] = UVCoord + FXAA_SUBPIX_OFFSET * vec2( 1.0/rt_w,  1.0/rt_h);\n\
			}\n";

		const std::string FXAA_Frag_Shader = "\
			**glsl_version**\n\
			/* \n\
			FXAA_EDGE_THRESHOLD \n\
			The minimum amount of local contrast required to apply algorithm. \n\
			1/3 - too little \n\
			1/4 - low quality \n\
			1/8 - high quality \n\
			1/16 - overkill \n\
			\n\
			FXAA_EDGE_THRESHOLD_MIN \n\
			Trims the algorithm from processing darks. \n\
			1/32 - visible limit \n\
			1/16 - high quality \n\
			1/12 - upper limit (start of visible unfiltered edges) \n\
			*/ \n\
			#define FXAA_EDGE_THRESHOLD_MIN 1.0/16.0 \n\
			#define FXAA_EDGE_THRESHOLD 1.0/8.0 \n\
			#define FXAA_SPAN_MAX 8.0 \n\
			uniform float rt_w;\n\
			uniform float rt_h;\n\
			uniform sampler2D tex;\n\
			/* \n\
				FXAA_SUBPIX_TRIM \n\
				Controls removal of sub-pixel aliasing. \n\
				1/2 - low removal \n\
				1/3 - medium removal \n\
				1/4 - default removal \n\
				1/8 - high removal \n\
				0 - complete removal \n\
			*/ \n\
			uniform float FXAA_SUBPIX_TRIM; //1.0/8.0;\n\
			\n\
			in vec2 texcoordOffset[4];\n\
			in vec2 UVCoord;\n\
			out vec4 Color;\n\
			\n\
			vec3 antialias() \n\
			{ \n\
				float FXAA_REDUCE_MIN = 1.0/128.0; \n\
				vec3 rgbNW = textureLod(tex, texcoordOffset[0], 0.0).xyz; \n\
				vec3 rgbNE = textureLod(tex, texcoordOffset[1], 0.0).xyz; \n\
				vec3 rgbSW = textureLod(tex, texcoordOffset[2], 0.0).xyz; \n\
				vec3 rgbSE = textureLod(tex, texcoordOffset[3], 0.0).xyz; \n\
				vec3 rgbM  = textureLod(tex, UVCoord, 0.0).xyz;\n\
				\n\
				vec3 luma = vec3(0.299, 0.587, 0.114);\n\
				float lumaNW = dot(rgbNW, luma);\n\
				float lumaNE = dot(rgbNE, luma);\n\
				float lumaSW = dot(rgbSW, luma);\n\
				float lumaSE = dot(rgbSE, luma);\n\
				float lumaM  = dot( rgbM, luma);\n\
				\n\
				float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n\
				float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n\
				float range = lumaMax - lumaMin;\n\
				//local contrast check, for not processing homogenius areas \n\
				if( range < max(FXAA_EDGE_THRESHOLD_MIN, lumaMax * FXAA_EDGE_THRESHOLD)) \n\
				{ \n\
					return rgbM; \n\
				} \n\
				\n\
				vec2 dir;\n\
				dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n\
				dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n\
				\n\
				float dirReduce = max((lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_SUBPIX_TRIM), FXAA_REDUCE_MIN);\n\
					\n\
				float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);\n\
				\n\
				dir = min(vec2(FXAA_SPAN_MAX,  FXAA_SPAN_MAX), \n\
					max(vec2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), dir * rcpDirMin)) / vec2(rt_w, rt_h);\n\
					\n\
				vec3 rgbA = 0.5 * (\n\
							textureLod(tex, UVCoord + dir * (1.0/3.0 - 0.5), 0.0).xyz +\n\
							textureLod(tex, UVCoord + dir * (2.0/3.0 - 0.5), 0.0).xyz);\n\
				vec3 rgbB = rgbA * 0.5 + (1.0/4.0) * (\n\
							textureLod(tex, UVCoord + dir * (0.0/3.0 - 0.5), 0.0).xyz +\n\
							textureLod(tex, UVCoord + dir * (3.0/3.0 - 0.5), 0.0).xyz);\n\
				float lumaB = dot(rgbB, luma);\n\
				\n\
				if((lumaB < lumaMin) || (lumaB > lumaMax)) \n\
				{ \n\
					return rgbA; \n\
				} \n\
				else \n\
				{ \n\
					return rgbB; \n\
				} \n\
			}\n\
			\n\
			void main(void) \n\
			{ \n\
				Color = vec4(antialias(), 1.0); \n\
			}";
	}
}
#endif
