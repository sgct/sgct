/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_INTERNAL_FISHEYE_SHADERS_H_
#define _SGCT_INTERNAL_FISHEYE_SHADERS_H_

#include <string>

namespace sgct_core
{
	/*
		All shaders are in GLSL 1.2 for compability with Mac OS X
	*/

	namespace shaders_fisheye
	{
		const std::string sample_fun = "\
            vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg)\n\
            {\n\
                float s = 2.0 * (texel.s - 0.5);\n\
                float t = 2.0 * (texel.t - 0.5);\n\
                float r2 = s*s + t*t;\n\
                if( r2 <= 1.0 )\n\
                {\n\
                    float phi = sqrt(r2) * halfFov;\n\
                    float theta = atan(s,t);\n\
                    float x = sin(phi) * sin(theta);\n\
                    float y = -sin(phi) * cos(theta);\n\
                    float z = cos(phi);\n\
                    **rotVec**;\n\
                    return textureCube(map, rotVec);\n\
                }\n\
                else\n\
                    return bg;\n\
            }\n";

		const std::string sample_latlon_fun = "\
            vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg)\n\
            {\n\
                float phi = 3.14159265359 * (1.0-texel.t);\n\
				float theta = 6.28318530718 * (texel.s - 0.5);\n\
				float x = sin(phi) * sin(theta);\n\
				float y = sin(phi) * cos(theta);\n\
				float z = cos(phi);\n\
                **rotVec**;\n\
                return textureCube(map, rotVec);\n\
            }\n";

		const std::string sample_offset_fun = "\
            vec4 getCubeSample(vec2 texel, samplerCube map, vec4 bg)\n\
            {\n\
                float s = 2.0 * (texel.s - 0.5);\n\
                float t = 2.0 * (texel.t - 0.5);\n\
                float r2 = s*s + t*t;\n\
                if( r2 <= 1.0 )\n\
                {\n\
                    float phi = sqrt(r2) * halfFov;\n\
                    float theta = atan(s,t);\n\
                    float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
                    **rotVec**;\n\
                    return textureCube(map, rotVec);\n\
                }\n\
                else\n\
                    return bg;\n\
            }\n";
		
		const std::string Base_Vert_Shader = "\
			**glsl_version**\n\
			\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_Position = gl_Vertex;\n\
			}\n";
		
		const std::string Fisheye_Vert_Shader = "\
			**glsl_version**\n\
			\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_Position = gl_Vertex;\n\
			}\n";

		const std::string Fisheye_Frag_Shader = "\
			**glsl_version**\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			**sample_fun**\n\
			\n\
			void main()\n\
			{\n\
				gl_FragColor = getCubeSample(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Normal = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(normalmap, rotVec);\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube positionmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(positionmap, rotVec);\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Normal_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube positionmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(normalmap, rotVec);\n\
					gl_FragData[2] = textureCube(positionmap, rotVec);\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth = "\
			**glsl_version**\n\
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
					**rotVec**;\n\
					gl_FragColor = textureCube(cubemap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragColor = **bgColor**;\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Normal = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform samplerCube normalmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(normalmap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform samplerCube positionmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(positionmap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Normal_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube positionmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(normalmap, rotVec);\n\
					gl_FragData[2] = textureCube(positionmap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			**sample_fun**\n\
			\n\
			void main()\n\
			{\n\
				gl_FragColor = getCubeSample(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Normal = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(normalmap, rotVec);\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube positionmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(positionmap, rotVec);\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Normal_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube positionmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(normalmap, rotVec);\n\
					gl_FragData[2] = textureCube(positionmap, rotVec);\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth = "\
			**glsl_version**\n\
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
					**rotVec**;\n\
					gl_FragColor = textureCube(cubemap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragColor = **bgColor**;\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Normal = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform samplerCube normalmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(normalmap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform samplerCube positionmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(positionmap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Normal_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube positionmap;\n\
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
					**rotVec**;\n\
					gl_FragData[0] = textureCube(cubemap, rotVec);\n\
					gl_FragData[1] = textureCube(normalmap, rotVec);\n\
					gl_FragData[2] = textureCube(positionmap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragData[0] = **bgColor**;\n\
					gl_FragData[1] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragData[2] = vec4(0.0, 0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Depth_Correction_Frag_Shader = "\
			**glsl_version**\n\
			\n\
			uniform sampler2D cTex;\n\
			uniform sampler2D dTex;\n\
			uniform float near;\n\
			uniform float far;\n\
			\n\
			void main()\n\
			{\n\
				float a = far/(far-near);\n\
				float b = far*near/(near-far);\n\
				float z = b/(texture2D(dTex, gl_TexCoord[0].st).x - a);\n\
				\n\
				//get angle from -45 to 45 degrees (-pi/4 to +pi/4) \n\
				float xAngle = 1.57079632679 * (gl_TexCoord[0].s - 0.5);\n\
				float yAngle = 1.57079632679 * (gl_TexCoord[0].t - 0.5);\n\
				\n\
				float x_norm = tan(xAngle); \n\
				float y_norm = tan(yAngle); \n\
				float r = z * sqrt(x_norm*x_norm + y_norm*y_norm + 1.0); \n\
				\n\
				gl_FragColor = texture2D(cTex, gl_TexCoord[0].st);\n\
				gl_FragDepth = a+b/r;\n\
				//No correction\n\
				//gl_FragDepth = texture2D(dTex, gl_TexCoord[0].st).x; \n\
			}\n";

	}//end shaders
}
#endif
