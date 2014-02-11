/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
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
		const std::string Base_Vert_Shader = "\
			#version 120\n\
			\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_Position = gl_Vertex;\n\
			}\n";
		
		const std::string Fisheye_Vert_Shader = "\
			#version 120\n\
			\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_Position = gl_Vertex;\n\
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
                    **rotVec**;\n\
					//four faces\n\
                    //vec3 rotVec = /vec3( angle45Factor*x + angle45Factor*z, y, -angle45Factor*x + angle45Factor*z);\n\
                    //five faces\n\
                    //vec3 rotVec = vec3( angle45Factor*x - angle45Factor*y, angle45Factor*x + angle45Factor*y, z);\n\
					gl_FragColor = textureCube(cubemap, rotVec);\n\
				}\n\
				else\n\
					gl_FragColor = **bgColor**;\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Normal = "\
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
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
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
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
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
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
					**rotVec**;\n\
					gl_FragColor = textureCube(cubemap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragColor = **bgColor**;\n\
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Normal = "\
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
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
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Position = "\
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
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
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Normal_Position = "\
			#version 120\n\
			//#pragma optionNV(fastmath off) // For NVIDIA cards.\n\
			//#pragma optionNV(fastprecision off) // For NVIDIA cards.\n\
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
					**rotVec**;\n\
					gl_FragColor = textureCube(cubemap, rotVec);\n\
				}\n\
				else\n\
					gl_FragColor = **bgColor**;\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Normal = "\
			#version 120\n\
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
			#version 120\n\
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
			#version 120\n\
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
					**rotVec**;\n\
					gl_FragColor = textureCube(cubemap, rotVec);\n\
					gl_FragDepth = textureCube(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					gl_FragColor = **bgColor**;\n\
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Normal = "\
			#version 120\n\
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
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Position = "\
			#version 120\n\
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
					gl_FragDepth = 0.0f;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Normal_Position = "\
			#version 120\n\
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
				//float xAngle = 1.57079632679 * (gl_TexCoord[0].s - 0.5);//correct artifacts are visible\n\
				//float yAngle = 1.57079632679 * (gl_TexCoord[0].t - 0.5);//correct artifacts are visible\n\
				float xAngle = 1.45 * (gl_TexCoord[0].s - 0.5);//less correct but gives better results\n\
				float yAngle = 1.45 * (gl_TexCoord[0].t - 0.5);//less correct but gives better results\n\
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
				//gl_FragDepth = texture2D(dTex, gl_TexCoord[0].st).x;//no warping\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Cubic_Test = "\
            #version 120\n\
            \n\
            uniform samplerCube cubemap;\n\
            uniform float halfFov;\n\
            uniform vec4 **bgColor**;\n\
            float size = 4096.0;\n\
            float angle45Factor = 0.7071067812;\n\
            \n\
            vec4 getCubeSample(vec2 texel)\n\
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
                    return textureCube(cubemap, rotVec);\n\
                }\n\
                else\n\
                    return **bgColor**;\n\
            }\n\
            \n\
            vec4 cubic(float x)\n\
            {\n\
                float x2 = x * x;\n\
                float x3 = x2 * x;\n\
                vec4 w;\n\
                w.x =   -x3 + 3*x2 - 3*x + 1;\n\
                w.y =  3*x3 - 6*x2       + 4;\n\
                w.z = -3*x3 + 3*x2 + 3*x + 1;\n\
                w.w =  x3;\n\
                return w / 6.0;\n\
            }\n\
            \n\
            vec4 catmull_rom(float x)\n\
            {\n\
                float x2 = x * x;\n\
                float x3 = x2 * x;\n\
                vec4 w;\n\
                w.x = -x + 2*x2 - x3;\n\
                w.y = 2 - 5*x2 + 3*x3;\n\
                w.z = x + 4*x2 - 3*x3;\n\
                w.w = -x2 + x3;\n\
                return w / 2.0;\n\
            }\n\
            \n\
            vec4 filter4(vec2 texcoord)\n\
            {\n\
                float fx = fract(texcoord.x);\n\
                float fy = fract(texcoord.y);\n\
                texcoord.x -= fx;\n\
                texcoord.y -= fy;\n\
                \n\
                vec4 xcubic = catmull_rom(fx);\n\
                vec4 ycubic = catmull_rom(fy);\n\
                \n\
                vec4 c = vec4(texcoord.x - 0.75, texcoord.x + 0.75, texcoord.y - 0.75, texcoord.y + 0.75);\n\
                vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);\n\
                vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;\n\
                \n\
                vec4 sample0 = getCubeSample(vec2(offset.x, offset.z) / vec2(size, size));\n\
                vec4 sample1 = getCubeSample(vec2(offset.y, offset.z) / vec2(size, size));\n\
                vec4 sample2 = getCubeSample(vec2(offset.x, offset.w) / vec2(size, size));\n\
                vec4 sample3 = getCubeSample(vec2(offset.y, offset.w) / vec2(size, size));\n\
                \n\
                float sx = s.x / (s.x + s.y);\n\
                float sy = s.z / (s.z + s.w);\n\
                \n\
                return mix(\n\
                       mix(sample3, sample2, sx),\n\
                       mix(sample1, sample0, sx), sy);\n\
            }\n\
            vec4 filter16(vec2 texcoord)\n\
            {\n\
                vec2 xy = floor(texcoord);\n\
                vec2 normxy = texcoord - xy;\n\
                \n\
                vec2 st0 = ((2.0 - normxy) * normxy - 1.0) * normxy;\n\
                vec2 st1 = (3.0 * normxy - 5.0) * normxy * normxy + 2.0;\n\
                vec2 st2 = ((4.0 - 3.0 * normxy) * normxy + 1.0) * normxy;\n\
                vec2 st3 = (normxy - 1.0) * normxy * normxy;\n\
                \n\
                vec4 row0 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, -1.0)) / vec2(size, size)) +\n\
                st1.s * getCubeSample((xy + vec2(0.0, -1.0)) / vec2(size, size)) +\n\
                st2.s * getCubeSample((xy + vec2(1.0, -1.0)) / vec2(size, size)) +\n\
                st3.s * getCubeSample((xy + vec2(2.0, -1.0)) / vec2(size, size));\n\
                \n\
                vec4 row1 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 0.0)) / vec2(size, size)) +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 0.0)) / vec2(size, size)) +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 0.0)) / vec2(size, size)) +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 0.0)) / vec2(size, size));\n\
                \n\
                vec4 row2 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 1.0)) / vec2(size, size)) +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 1.0)) / vec2(size, size)) +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 1.0)) / vec2(size, size)) +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 1.0)) / vec2(size, size));\n\
                \n\
                vec4 row3 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 2.0)) / vec2(size, size)) +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 2.0)) / vec2(size, size)) +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 2.0)) / vec2(size, size)) +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 2.0)) / vec2(size, size));\n\
                \n\
                return 0.25 * ((st0.t * row0) + (st1.t * row1) + (st2.t * row2) + (st3.t * row3));\n\
            }\n\
            \n\
            void main()\n\
            {\n\
                gl_FragColor = filter4(gl_TexCoord[0].st * vec2(size, size));\n\
                //gl_FragColor = getCubeSample(gl_TexCoord[0].st);\n\
            }\n";

	}//end shaders
}
#endif
