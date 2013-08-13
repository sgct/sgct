/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _SGCT_INTERNAL_SHADERS_H_
#define _SGCT_INTERNAL_SHADERS_H_

#include <string>

namespace sgct_core
{
	/*!
		All shaders are in GLSL 1.2 for compability with Mac OS X
	*/
	namespace shaders
	{
		const std::string Base_Vert_Shader = "\
			#version 120\n\
			\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
			}\n";

		const std::string Fisheye_Vert_Shader = "\
			#version 120\n\
			\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
			}\n";

		const std::string Fisheye_Frag_Shader = "\
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (gl_TexCoord[0].s - 0.5);\n\
				float t = 2.0 * (gl_TexCoord[0].t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					vec3 rotVec = vec3( angle45Factor*x + angle45Factor*z, y, -angle45Factor*x + angle45Factor*z);\n\
					gl_FragColor = vec4(textureCube(cubemap, rotVec));\n\
				}\n\
				else\n\
					gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth = "\
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (gl_TexCoord[0].s - 0.5);\n\
				float t = 2.0 * (gl_TexCoord[0].t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					vec3 rotVec = vec3( angle45Factor*x + angle45Factor*z, y, -angle45Factor*x + angle45Factor*z);\n\
					gl_FragColor = vec4(textureCube(cubemap, rotVec));\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";
		
		const std::string Fisheye_Frag_Shader_OffAxis = "\
			#version 120\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (gl_TexCoord[0].s - 0.5);\n\
				float t = 2.0 * (gl_TexCoord[0].t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					vec3 rotVec = vec3( angle45Factor*x + angle45Factor*z, y, -angle45Factor*x + angle45Factor*z);\n\
					gl_FragColor = vec4(textureCube(cubemap, rotVec));\n\
				}\n\
				else\n\
					gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth = "\
			#version 120\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (gl_TexCoord[0].s - 0.5);\n\
				float t = 2.0 * (gl_TexCoord[0].t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					vec3 rotVec = vec3( angle45Factor*x + angle45Factor*z, y, -angle45Factor*x + angle45Factor*z);\n\
					gl_FragColor = vec4(textureCube(cubemap, rotVec));\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";
		
		const std::string Anaglyph_Vert_Shader = "\
			#version 120\n\
			\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
				gl_FrontColor = gl_Color;\n\
			}\n";
		
		const std::string Anaglyph_Red_Cyan_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec4 leftVals = texture2D( LeftTex, gl_TexCoord[0].st);\n\
				float leftLum = 0.3 * leftVals.r + 0.59 * leftVals.g + 0.11 * leftVals.b;\n\
				vec4 rightVals = texture2D( RightTex, gl_TexCoord[0].st);\n\
				float rightLum = 0.3 * rightVals.r + 0.59 * rightVals.g + 0.11 * rightVals.b;\n\
				gl_FragColor.r = gl_Color.r * leftLum;\n\
				gl_FragColor.g = gl_Color.g * rightLum;\n\
				gl_FragColor.b = gl_Color.b * rightLum;\n\
				gl_FragColor.a = gl_Color.a * max(leftVals.a, rightVals.a);\n\
			}\n";

		const std::string Anaglyph_Red_Cyan_Frag_Shader_Wimmer = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec4 leftVals = texture2D( LeftTex, gl_TexCoord[0].st);\n\
				vec4 rightVals = texture2D( RightTex, gl_TexCoord[0].st);\n\
				gl_FragColor.r = gl_Color.r * (0.7*leftVals.g + 0.3*leftVals.b);\n\
				gl_FragColor.g = gl_Color.g * rightVals.r;\n\
				gl_FragColor.b = gl_Color.b * rightVals.b;\n\
				gl_FragColor.a = gl_Color.a * max(leftVals.a, rightVals.a);\n\
			}\n";

		const std::string Anaglyph_Amber_Blue_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec4 leftVals = texture2D( LeftTex, gl_TexCoord[0].st);\n\
				vec4 rightVals = texture2D( RightTex, gl_TexCoord[0].st);\n\
				vec3 coef = vec3(0.15, 0.15, 0.70);\n\
				float rightMix = dot(rightVals.rbg, coef);\n\
				gl_FragColor.r = gl_Color.r * leftVals.r;\n\
				gl_FragColor.g = gl_Color.g * leftVals.g;\n\
				gl_FragColor.b = gl_Color.b * rightMix;\n\
				gl_FragColor.a = gl_Color.a * max(leftVals.a, rightVals.a);\n\
			}\n";

		const std::string CheckerBoard_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;\n\
				if( (fval - floor(fval)) == 0.0 )\n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[0].st);\n\
				else\n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st);\n\
			}\n";

		const std::string CheckerBoard_Inverted_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;\n\
				if( (fval - floor(fval)) == 0.0 )\n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st);\n\
				else\n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[0].st);\n\
			}\n";

		//--------> vertical interlaced shaders
		const std::string Vertical_Interlaced_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				float fval = gl_FragCoord.y * 0.5;\n\
				if( (fval - floor(fval)) > 0.5 )\n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[0].st);\n\
				else\n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st);\n\
			}\n";

		//------------> dummy stereo shader
		const std::string Dummy_Stereo_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				gl_FragColor = gl_Color * (0.5 * texture2D( LeftTex, gl_TexCoord[0].st) + 0.5 * texture2D( RightTex, gl_TexCoord[0].st));\n\
			}\n";

		const std::string Vertical_Interlaced_Inverted_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				float fval = gl_FragCoord.y * 0.5;\n\
				if( (fval - floor(fval)) > 0.5 )\n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st);\n\
				else\n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[0].st);\n\
			}\n";

		const std::string FXAA_Vert_Shader = "\
			varying vec4 posPos;\n\
			//#define FXAA_SUBPIX_SHIFT (1.0/4.0)\n\
			uniform float FXAA_SUBPIX_SHIFT; //1.0/4.0;\n\
			\n\
			uniform float rt_w;\n\
			uniform float rt_h;\n\
			\n\
			void main(void)\n\
			{\n\
			  gl_Position = ftransform();\n\
			  gl_TexCoord[0] = gl_MultiTexCoord0;\n\
			  \n\
			  vec2 rcpFrame = vec2(1.0/rt_w, 1.0/rt_h);\n\
			  posPos.xy = gl_MultiTexCoord0.xy;\n\
			  posPos.zw = gl_MultiTexCoord0.xy - (rcpFrame * (0.5 + FXAA_SUBPIX_SHIFT));\n\
			}\n";

		const std::string FXAA_Frag_Shader = "\
			#version 120\n\
			#extension GL_EXT_gpu_shader4 : enable // For NVIDIA cards.\n\
			uniform sampler2D tex0; // 0\n\
			uniform float vx_offset;\n\
			uniform float rt_w;\n\
			uniform float rt_h;\n\
			uniform float FXAA_SPAN_MAX;\n\
			uniform float FXAA_REDUCE_MUL; //1.0/8.0;\n\
			varying vec4 posPos;\n\
			\n\
			#define FxaaInt2 ivec2\n\
			#define FxaaFloat2 vec2\n\
			\n\
			vec3 FxaaPixelShader( \n\
			  vec4 posPos,       // Output of FxaaVertexShader interpolated across screen.\n\
			  sampler2D tex,     // Input texture.\n\
			  vec2 rcpFrame) // Constant {1.0/frameWidth, 1.0/frameHeight}.\n\
			{\n\
			/*--------------------------------------------------------------------------*/\n\
			#define FXAA_REDUCE_MIN   (1.0/128.0)\n\
			//#define FXAA_REDUCE_MUL   (1.0/8.0)\n\
			//#define FXAA_SPAN_MAX     8.0\n\
			/*--------------------------------------------------------------------------*/\n\
			vec3 rgbNW = texture2DLod(tex, posPos.zw, 0.0).xyz;\n\
			vec3 rgbNE = texture2DLodOffset(tex, posPos.zw, 0.0, FxaaInt2(1,0)).xyz;\n\
			vec3 rgbSW = texture2DLodOffset(tex, posPos.zw, 0.0, FxaaInt2(0,1)).xyz;\n\
			vec3 rgbSE = texture2DLodOffset(tex, posPos.zw, 0.0, FxaaInt2(1,1)).xyz;\n\
			vec3 rgbM  = texture2DLod(tex, posPos.xy, 0.0).xyz;\n\
			/*--------------------------------------------------------------------------*/\n\
			vec3 luma = vec3(0.299, 0.587, 0.114);\n\
			float lumaNW = dot(rgbNW, luma);\n\
			float lumaNE = dot(rgbNE, luma);\n\
			float lumaSW = dot(rgbSW, luma);\n\
			float lumaSE = dot(rgbSE, luma);\n\
			float lumaM  = dot(rgbM,  luma);\n\
			/*--------------------------------------------------------------------------*/\n\
			float lumaMin = min(lumaM, min(min(lumaNW, lumaNE), min(lumaSW, lumaSE)));\n\
			float lumaMax = max(lumaM, max(max(lumaNW, lumaNE), max(lumaSW, lumaSE)));\n\
			/*--------------------------------------------------------------------------*/\n\
			vec2 dir;\n\
			dir.x = -((lumaNW + lumaNE) - (lumaSW + lumaSE));\n\
			dir.y =  ((lumaNW + lumaSW) - (lumaNE + lumaSE));\n\
			/*--------------------------------------------------------------------------*/\n\
			float dirReduce = max( \n\
				(lumaNW + lumaNE + lumaSW + lumaSE) * (0.25 * FXAA_REDUCE_MUL),\n\
				FXAA_REDUCE_MIN);\n\
			float rcpDirMin = 1.0/(min(abs(dir.x), abs(dir.y)) + dirReduce);\n\
			dir = min(FxaaFloat2( FXAA_SPAN_MAX,  FXAA_SPAN_MAX), \n\
				max(FxaaFloat2(-FXAA_SPAN_MAX, -FXAA_SPAN_MAX), \n\
				dir * rcpDirMin)) * rcpFrame.xy; \n\
			/*--------------------------------------------------------------------------*/\n\
			vec3 rgbA = (1.0/2.0) * (\n\
				texture2DLod(tex, posPos.xy + dir * (1.0/3.0 - 0.5), 0.0).xyz +\n\
				texture2DLod(tex, posPos.xy + dir * (2.0/3.0 - 0.5), 0.0).xyz);\n\
			vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (\n\
				texture2DLod(tex, posPos.xy + dir * (0.0/3.0 - 0.5), 0.0).xyz + \n\
				texture2DLod(tex, posPos.xy + dir * (3.0/3.0 - 0.5), 0.0).xyz); \n\
			float lumaB = dot(rgbB, luma); \n\
			if((lumaB < lumaMin) || (lumaB > lumaMax)) return rgbA;\n\
				return rgbB; }\n\
			\n\
			vec4 PostFX(sampler2D tex, vec2 uv, float time)\n\
			{\n\
			  vec4 c = vec4(0.0);\n\
			  vec2 rcpFrame = vec2(1.0/rt_w, 1.0/rt_h);\n\
				c.rgb = FxaaPixelShader(posPos, tex, rcpFrame);\n\
				c.a = 1.0;\n\
				return c;\n\
			}\n\
			\n\
			void main()\n\
			{\n\
				vec2 uv = gl_TexCoord[0].st;\n\
				gl_FragColor = PostFX(tex0, uv, 0.0);\n\
			}\n";

	}//end shaders
}
#endif
