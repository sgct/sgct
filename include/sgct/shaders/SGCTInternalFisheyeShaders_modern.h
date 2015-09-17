/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_INTERNAL_FISHEYE_SHADERS_MODERN_H_
#define _SGCT_INTERNAL_FISHEYE_SHADERS_MODERN_H_

#include <string>

namespace sgct_core
{
	/*
		Contains GLSL 3.3+ shaders
	*/

	namespace shaders_modern_fisheye
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
					return texture(map, rotVec);\n\
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
                return texture(map, rotVec);\n\
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
                    return texture(map, rotVec);\n\
                }\n\
                else\n\
                    return bg;\n\
            }\n";
		
		const std::string Base_Vert_Shader = "\
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
		
		const std::string Fisheye_Vert_Shader = "\
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

		const std::string Fisheye_Frag_Shader = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
			out vec4 diffuse;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			**sample_fun**\n\
			\n\
			void main()\n\
			{\n\
				diffuse = getCubeSample(UV, cubemap, **bgColor**);\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Normal = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 normal;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					normal = texture(normalmap, rotVec).xyz;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					normal = vec3(0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Position = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 position;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube positionmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					position = texture(positionmap, rotVec).xyz;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					position = vec3(0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Normal_Position = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 normal;\n\
            layout(location = 2) out vec3 position;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube positionmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					normal = texture(normalmap, rotVec).xyz;\n\
					position = texture(positionmap, rotVec).xyz;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					normal = vec3(0.0, 0.0, 0.0);\n\
					position = vec3(0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
			out vec4 diffuse;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					gl_FragDepth = texture(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Normal = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 normal;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					normal = texture(normalmap, rotVec).xyz;\n\
					gl_FragDepth = texture(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					normal = vec3(0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Position = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 position;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube positionmap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					position = texture(positionmap, rotVec).xyz;\n\
					gl_FragDepth = texture(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					position = vec3(0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth_Normal_Position = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 normal;\n\
            layout(location = 2) out vec3 position;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube positionmap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					normal = texture(normalmap, rotVec).xyz;\n\
					position = texture(positionmap, rotVec).xyz;\n\
					gl_FragDepth = texture(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					normal = vec3(0.0, 0.0, 0.0);\n\
					position = vec3(0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
			out vec4 diffuse;\n\
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
				diffuse = getCubeSample(UV, cubemap, **bgColor**);\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Normal = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 normal;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					normal = texture(normalmap, rotVec).xyz;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					normal = vec3(0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Position = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 position;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube positionmap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					position = texture(positionmap, rotVec).xyz;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					position = vec3(0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Normal_Position = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 normal;\n\
            layout(location = 2) out vec3 position;\n\
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
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					normal = texture(normalmap, rotVec).xyz;\n\
					position = texture(positionmap, rotVec).xyz;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					normal = vec3(0.0, 0.0, 0.0);\n\
					position = vec3(0.0, 0.0, 0.0);\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
			out vec4 diffuse;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					gl_FragDepth = texture(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Normal = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 normal;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					normal = texture(normalmap, rotVec).xyz;\n\
					gl_FragDepth = texture(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					normal = vec3(0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Position = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 position;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube positionmap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					position = texture(positionmap, rotVec).xyz;\n\
					gl_FragDepth = texture(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					position = vec3(0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Frag_Shader_OffAxis_Depth_Normal_Position = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
            layout(location = 0) out vec4 diffuse;\n\
            layout(location = 1) out vec3 normal;\n\
            layout(location = 2) out vec3 position;\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform samplerCube positionmap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			uniform vec3 offset;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (UV.s - 0.5);\n\
				float t = 2.0 * (UV.t - 0.5);\n\
				float r2 = s*s + t*t;\n\
				if( r2 <= 1.0 )\n\
				{\n\
					float phi = sqrt(r2) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta) - offset.x;\n\
					float y = -sin(phi) * cos(theta) - offset.y;\n\
					float z = cos(phi) - offset.z;\n\
					**rotVec**;\n\
					diffuse = texture(cubemap, rotVec);\n\
					normal = texture(normalmap, rotVec).xyz;\n\
					position = texture(positionmap, rotVec).xyz;\n\
					gl_FragDepth = texture(depthmap, rotVec).x;\n\
				}\n\
				else\n\
				{\n\
					diffuse = **bgColor**;\n\
					normal = vec3(0.0, 0.0, 0.0);\n\
					position = vec3(0.0, 0.0, 0.0);\n\
					gl_FragDepth = 1.0;\n\
				}\n\
			}\n";

		const std::string Fisheye_Depth_Correction_Frag_Shader = "\
			**glsl_version**\n\
			\n\
			in vec2 UV;\n\
			out vec4 Color;\n\
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
				float z = b/(texture(dTex, UV).x - a);\n\
				\n\
				//get angle from -45 to 45 degrees (-pi/4 to +pi/4) \n\
				float xAngle = 1.57079632679 * (UV.s - 0.5);\n\
				float yAngle = 1.57079632679 * (UV.t - 0.5);\n\
				\n\
				float x_norm = tan(xAngle); \n\
				float y_norm = tan(yAngle); \n\
				float r = z * sqrt(x_norm*x_norm + y_norm*y_norm + 1.0); \n\
				\n\
				Color = texture(cTex, UV);\n\
				gl_FragDepth = a+b/r;\n\
				//No correction\n\
				//gl_FragDepth = texture(dTex, UV).x; \n\
			}\n";

		const std::string Fisheye_Frag_Shader_Cubic_Test = "\
            **glsl_version**\n\
            \n\
            in vec2 UV;\n\
            out vec4 diffuse;\n\
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
                    return texture(cubemap, rotVec);\n\
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
                diffuse = filter4(UV * vec2(size, size));\n\
                //diffuse = getCubeSample(UV);\n\
			}\n";
	}
}
#endif
