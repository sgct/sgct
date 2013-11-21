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
	/*
		All shaders are in GLSL 1.2 for compability with Mac OS X
	*/

	/*! \namespace shaders
	\brief shaders namespace contains legacy opengl shaders
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
			uniform vec4 bgColor;\n\
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
					gl_FragColor = bgColor;\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth = "\
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			uniform vec4 bgColor;\n\
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
					gl_FragColor = bgColor;\n\
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis = "\
			#version 120\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform float halfFov;\n\
			uniform vec4 bgColor;\n\
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
					gl_FragColor = bgColor;\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth = "\
			#version 120\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			uniform vec4 bgColor;\n\
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
					gl_FragColor = bgColor;\n\
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";

		const std::string Fisheye_Depth_Correction_Frag_Shader = "\
			#version 120\n\
			\n\
			uniform sampler2D cTex;\n\
			uniform sampler2D dTex;\n\
			uniform float near;\n\
			uniform float far;\n\
			\n\
			float getDepth(float bufferVal)\n\
			{\n\
				float z_n = 2.0 * bufferVal - 1.0;\n\
				return 2.0 * near * far / (far + near - z_n * (far - near));\n\
			}\n\
			\n\
			float convertBack(float z)\n\
			{\n\
				float za = (2.0 * near * far)/z; \n\
				float zb = (za - (far + near))/(far - near); \n\
				return (1.0 - zb)/2.0; \n\
			}\n\
			\n\
			void main()\n\
			{\n\
				//get angle from -45 to 45 degrees (-pi/4 to +pi/4) \n\
				//float xAngle = 1.57079632679 * (UV.s - 0.5);\n\
				//float yAngle = 1.57079632679 * (UV.t - 0.5);\n\
				float xAngle = 1.45 * (gl_TexCoord[0].s - 0.5);\n\
				float yAngle = 1.45 * (gl_TexCoord[0].t - 0.5);\n\
				\n\
				float z = getDepth(texture2D(dTex, gl_TexCoord[0].st).x); \n\
				float a = tan(xAngle); \n\
				float b = tan(yAngle); \n\
				//use r = sqrt(x*x + y*y + z*z) \n\
				// x = z * tan ( xAngle ) \n\
				// y = z * tan ( yAngle ) \n\
				float r = z * sqrt(a*a + b*b + 1.0); \n\
				\n\
				gl_FragColor = texture2D(cTex, gl_TexCoord[0].st);\n\
				gl_FragDepth = convertBack(r);\n\
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

		const std::string Anaglyph_Red_Cyan_Stereo_Frag_Shader = "\
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

		const std::string Anaglyph_Red_Cyan_Stereo_Frag_Shader_Wimmer = "\
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

		const std::string Anaglyph_Amber_Blue_Stereo_Frag_Shader = "\
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
		const std::string Vertical_Interlaced_Stereo_Frag_Shader = "\
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

		const std::string Vertical_Interlaced_Inverted_Stereo_Frag_Shader = "\
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

		const std::string SBS_Stereo_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec2 mul = vec2(2.0, 1.0); \n\
				if( gl_TexCoord[0].s < 0.5 ) \n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st * mul); \n\
				else \n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[0].st * mul - vec2(1.0, 0.0)); \n\
			}\n";

		const std::string SBS_Stereo_Inverted_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec2 mul = vec2(2.0, 1.0); \n\
				if( gl_TexCoord[0].s < 0.5 ) \n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[0].st * mul); \n\
				else \n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st * mul - vec2(1.0, 0.0)); \n\
			}\n";

		const std::string TB_Stereo_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec2 mul = vec2(1.0, 2.0); \n\
				if( gl_TexCoord[0].t < 0.5 ) \n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[0].st * mul); \n\
				else \n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st * mul - vec2(0.0, 1.0)); \n\
			}\n";

		const std::string TB_Stereo_Inverted_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec2 mul = vec2(1.0, 2.0); \n\
				if( gl_TexCoord[0].t < 0.5 ) \n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st * mul); \n\
				else \n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[0].st * mul - vec2(0.0, 1.0)); \n\
			}\n";

		const std::string FXAA_Vert_Shader = "\
			#version 120\n\
			uniform float rt_w;\n\
			uniform float rt_h;\n\
			uniform float FXAA_SUBPIX_OFFSET; \n\
			\n\
			varying vec2 texcoordOffset[4];\n\
			\n\
			void main(void)\n\
			{\n\
				gl_Position = ftransform();\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				\n\
				texcoordOffset[0] = gl_TexCoord[0].st + FXAA_SUBPIX_OFFSET * vec2(-1.0/rt_w,  -1.0/rt_h);\n\
				texcoordOffset[1] = gl_TexCoord[0].st + FXAA_SUBPIX_OFFSET * vec2( 1.0/rt_w,  -1.0/rt_h);\n\
				texcoordOffset[2] = gl_TexCoord[0].st + FXAA_SUBPIX_OFFSET * vec2(-1.0/rt_w,  1.0/rt_h);\n\
				texcoordOffset[3] = gl_TexCoord[0].st + FXAA_SUBPIX_OFFSET * vec2( 1.0/rt_w,  1.0/rt_h);\n\
			}\n";

		const std::string FXAA_Frag_Shader = "\
			#version 120\n\
			#extension GL_EXT_gpu_shader4 : enable // For NVIDIA cards.\n\
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
			varying vec2 texcoordOffset[4];\n\
			\n\
			vec3 antialias() \n\
			{ \n\
				float FXAA_REDUCE_MIN = 1.0/128.0; \n\
				vec3 rgbNW = texture2DLod(tex, texcoordOffset[0], 0.0).xyz; \n\
				vec3 rgbNE = texture2DLod(tex, texcoordOffset[1], 0.0).xyz; \n\
				vec3 rgbSW = texture2DLod(tex, texcoordOffset[2], 0.0).xyz; \n\
				vec3 rgbSE = texture2DLod(tex, texcoordOffset[3], 0.0).xyz; \n\
				vec3 rgbM  = texture2DLod(tex, gl_TexCoord[0].st, 0.0).xyz;\n\
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
							texture2DLod(tex, gl_TexCoord[0].st + dir * (1.0/3.0 - 0.5), 0.0).xyz +\n\
							texture2DLod(tex, gl_TexCoord[0].st + dir * (2.0/3.0 - 0.5), 0.0).xyz);\n\
				vec3 rgbB = rgbA * 0.5 + (1.0/4.0) * (\n\
							texture2DLod(tex, gl_TexCoord[0].st + dir * (0.0/3.0 - 0.5), 0.0).xyz +\n\
							texture2DLod(tex, gl_TexCoord[0].st + dir * (3.0/3.0 - 0.5), 0.0).xyz);\n\
				float lumaB = dot(rgbB, luma);\n\
				\n\
				if((lumaB < lumaMin) || (lumaB > lumaMax)) \n\
				{ \n\
					return rgbA; \n\
					//return vec3(1.0, 0.0, 0.0); \n\
				} \n\
				else \n\
				{ \n\
					return rgbB; \n\
					//return vec3(0.0, 1.0, 0.0); \n\
				} \n\
			}\n\
			\n\
			void main(void) \n\
			{ \n\
				gl_FragColor = vec4(antialias(), 1.0); \n\
			}";

	}//end shaders
}
#endif
