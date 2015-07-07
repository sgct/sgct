/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#ifndef _SGCT_INTERNAL_FISHEYE_SHADERS_CUBIC_H_
#define _SGCT_INTERNAL_FISHEYE_SHADERS_CUBIC_H_

#include <string>

namespace sgct_core
{
	/*
		All shaders are in GLSL 1.2 for compability with Mac OS X
	*/

	namespace shaders_fisheye_cubic
	{
		const std::string B_spline_fun = "\
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
            }\n";

		const std::string catmull_rom_fun = "\
            vec4 cubic(float x)\n\
            {\n\
                float x2 = x * x;\n\
                float x3 = x2 * x;\n\
                vec4 w;\n\
                w.x = -x + 2*x2 - x3;\n\
                w.y = 2 - 5*x2 + 3*x3;\n\
                w.z = x + 4*x2 - 3*x3;\n\
                w.w = -x2 + x3;\n\
                return w / 2.0;\n\
            }\n";
        
        const std::string weightedMultisample_f = "\
            float filterf(vec2 texcoord, samplerCube map, vec4 bg)\n\
            {\n\
                float sample0 = getCubeSample(texcoord, map, bg).x;\n\
                float sample1 = getCubeSample(texcoord + vec2(0.5, 0.5) / vec2(**size**, **size**), map, bg).x;\n\
                float sample2 = getCubeSample(texcoord + vec2(-0.5, 0.5) / vec2(**size**, **size**), map, bg).x;\n\
                float sample3 = getCubeSample(texcoord + vec2(-0.5, -0.5) / vec2(**size**, **size**), map, bg).x;\n\
                float sample4 = getCubeSample(texcoord + vec2(0.5, -0.5) / vec2(**size**, **size**), map, bg).x;\n\
                \n\
                return (4.0*sample0 + sample1 + sample2 + sample3 + sample4)/8.0;\n\
            }\n";
        
        const std::string weightedMultisample_4f = "\
            vec4 filter4f(vec2 texcoord, samplerCube map, vec4 bg)\n\
            {\n\
                vec4 sample0 = getCubeSample(texcoord, map, bg);\n\
                vec4 sample1 = getCubeSample(texcoord + vec2(0.5, 0.5) / vec2(**size**, **size**), map, bg);\n\
                vec4 sample2 = getCubeSample(texcoord + vec2(-0.5, 0.5) / vec2(**size**, **size**), map, bg);\n\
                vec4 sample3 = getCubeSample(texcoord + vec2(-0.5, -0.5) / vec2(**size**, **size**), map, bg);\n\
                vec4 sample4 = getCubeSample(texcoord + vec2(0.5, -0.5) / vec2(**size**, **size**), map, bg);\n\
                \n\
                return (4.0*sample0 + sample1 + sample2 + sample3 + sample4)/8.0;\n\
            }\n";

		const std::string interpolate4_f = "\
            float filterf(vec2 texcoord, samplerCube map, vec4 bg)\n\
            {\n\
                vec2 transTex = texcoord * vec2(**size**, **size**);\n\
                float fx = fract(transTex.x);\n\
                float fy = fract(transTex.y);\n\
                transTex.x -= fx;\n\
                transTex.y -= fy;\n\
                \n\
                vec4 xcubic = cubic(fx);\n\
                vec4 ycubic = cubic(fy);\n\
                \n\
                vec4 c = vec4(transTex.x - **step**, transTex.x + **step**, transTex.y - **step**, transTex.y + **step**);\n\
                vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);\n\
                vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;\n\
                \n\
                float sample0 = getCubeSample(vec2(offset.x, offset.z) / vec2(**size**, **size**), map, bg).x;\n\
                float sample1 = getCubeSample(vec2(offset.y, offset.z) / vec2(**size**, **size**), map, bg).x;\n\
                float sample2 = getCubeSample(vec2(offset.x, offset.w) / vec2(**size**, **size**), map, bg).x;\n\
                float sample3 = getCubeSample(vec2(offset.y, offset.w) / vec2(**size**, **size**), map, bg).x;\n\
                \n\
                float sx = s.x / (s.x + s.y);\n\
                float sy = s.z / (s.z + s.w);\n\
                \n\
                return mix(\n\
                       mix(sample3, sample2, sx),\n\
                       mix(sample1, sample0, sx), sy);\n\
            }\n";

		const std::string interpolate4_4f = "\
            vec4 filter4f(vec2 texcoord, samplerCube map, vec4 bg)\n\
            {\n\
                vec2 transTex = texcoord * vec2(**size**, **size**);\n\
                float fx = fract(transTex.x);\n\
                float fy = fract(transTex.y);\n\
                transTex.x -= fx;\n\
                transTex.y -= fy;\n\
                \n\
                vec4 xcubic = cubic(fx);\n\
                vec4 ycubic = cubic(fy);\n\
                \n\
                vec4 c = vec4(transTex.x - **step**, transTex.x + **step**, transTex.y - **step**, transTex.y + **step**);\n\
                vec4 s = vec4(xcubic.x + xcubic.y, xcubic.z + xcubic.w, ycubic.x + ycubic.y, ycubic.z + ycubic.w);\n\
                vec4 offset = c + vec4(xcubic.y, xcubic.w, ycubic.y, ycubic.w) / s;\n\
                \n\
                vec4 sample0 = getCubeSample(vec2(offset.x, offset.z) / vec2(**size**, **size**), map, bg);\n\
                vec4 sample1 = getCubeSample(vec2(offset.y, offset.z) / vec2(**size**, **size**), map, bg);\n\
                vec4 sample2 = getCubeSample(vec2(offset.x, offset.w) / vec2(**size**, **size**), map, bg);\n\
                vec4 sample3 = getCubeSample(vec2(offset.y, offset.w) / vec2(**size**, **size**), map, bg);\n\
                \n\
                float sx = s.x / (s.x + s.y);\n\
                float sy = s.z / (s.z + s.w);\n\
                \n\
                return mix(\n\
                       mix(sample3, sample2, sx),\n\
                       mix(sample1, sample0, sx), sy);\n\
            }\n";

		const std::string interpolate16_f = "\
            float filterf(vec2 texcoord, samplerCube map, vec4 bg)\n\
            {\n\
                vec2 transTex = texcoord * vec2(**size**, **size**);\n\
                vec2 xy = floor(transTex);\n\
                vec2 normxy = transTex - xy;\n\
                \n\
                vec2 st0 = ((2.0 - normxy) * normxy - 1.0) * normxy;\n\
                vec2 st1 = (3.0 * normxy - 5.0) * normxy * normxy + 2.0;\n\
                vec2 st2 = ((4.0 - 3.0 * normxy) * normxy + 1.0) * normxy;\n\
                vec2 st3 = (normxy - 1.0) * normxy * normxy;\n\
                \n\
                float row0 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, -1.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st1.s * getCubeSample((xy + vec2(0.0, -1.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st2.s * getCubeSample((xy + vec2(1.0, -1.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st3.s * getCubeSample((xy + vec2(2.0, -1.0)) / vec2(**size**, **size**), map, bg).x;\n\
                \n\
                float row1 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 0.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 0.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 0.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 0.0)) / vec2(**size**, **size**), map, bg).x;\n\
                \n\
                float row2 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 1.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 1.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 1.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 1.0)) / vec2(**size**, **size**), map, bg).x;\n\
                \n\
                float row3 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 2.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 2.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 2.0)) / vec2(**size**, **size**), map, bg).x +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 2.0)) / vec2(**size**, **size**), map, bg).x;\n\
                \n\
                return 0.25 * ((st0.t * row0) + (st1.t * row1) + (st2.t * row2) + (st3.t * row3));\n\
            }\n";

		const std::string interpolate16_4f = "\
            vec4 filter4f(vec2 texcoord, samplerCube map, vec4 bg)\n\
            {\n\
                vec2 transTex = texcoord * vec2(**size**, **size**);\n\
                vec2 xy = floor(transTex);\n\
                vec2 normxy = transTex - xy;\n\
                \n\
                vec2 st0 = ((2.0 - normxy) * normxy - 1.0) * normxy;\n\
                vec2 st1 = (3.0 * normxy - 5.0) * normxy * normxy + 2.0;\n\
                vec2 st2 = ((4.0 - 3.0 * normxy) * normxy + 1.0) * normxy;\n\
                vec2 st3 = (normxy - 1.0) * normxy * normxy;\n\
                \n\
                vec4 row0 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, -1.0)) / vec2(**size**, **size**), map, bg) +\n\
                st1.s * getCubeSample((xy + vec2(0.0, -1.0)) / vec2(**size**, **size**), map, bg) +\n\
                st2.s * getCubeSample((xy + vec2(1.0, -1.0)) / vec2(**size**, **size**), map, bg) +\n\
                st3.s * getCubeSample((xy + vec2(2.0, -1.0)) / vec2(**size**, **size**), map, bg);\n\
                \n\
                vec4 row1 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 0.0)) / vec2(**size**, **size**), map, bg) +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 0.0)) / vec2(**size**, **size**), map, bg) +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 0.0)) / vec2(**size**, **size**), map, bg) +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 0.0)) / vec2(**size**, **size**), map, bg);\n\
                \n\
                vec4 row2 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 1.0)) / vec2(**size**, **size**), map, bg) +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 1.0)) / vec2(**size**, **size**), map, bg) +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 1.0)) / vec2(**size**, **size**), map, bg) +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 1.0)) / vec2(**size**, **size**), map, bg);\n\
                \n\
                vec4 row3 =\n\
                st0.s * getCubeSample((xy + vec2(-1.0, 2.0)) / vec2(**size**, **size**), map, bg) +\n\
                st1.s * getCubeSample((xy + vec2(0.0, 2.0)) / vec2(**size**, **size**), map, bg) +\n\
                st2.s * getCubeSample((xy + vec2(1.0, 2.0)) / vec2(**size**, **size**), map, bg) +\n\
                st3.s * getCubeSample((xy + vec2(2.0, 2.0)) / vec2(**size**, **size**), map, bg);\n\
                \n\
                return 0.25 * ((st0.t * row0) + (st1.t * row1) + (st2.t * row2) + (st3.t * row3));\n\
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
            \n\
            uniform samplerCube cubemap;\n\
            uniform float halfFov;\n\
            float angle45Factor = 0.7071067812;\n\
            \n\
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
            \n\
            void main()\n\
            {\n\
                gl_FragColor = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
            }\n";

		const std::string Fisheye_Frag_Shader_Normal = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube normalmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, normalmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Position = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube positionmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, positionmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, normalmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragData[2] = filter4f(gl_TexCoord[0].st, positionmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
			}\n";

		const std::string Fisheye_Frag_Shader_Depth = "\
			**glsl_version**\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform samplerCube depthmap;\n\
			uniform float halfFov;\n\
			float angle45Factor = 0.7071067812;\n\
			\n\
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
			**interpolatef**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragColor = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragDepth = filterf(gl_TexCoord[0].st, depthmap, vec4(1.0, 1.0, 1.0, 1.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
			**interpolatef**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, normalmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragDepth = filterf(gl_TexCoord[0].st, depthmap, vec4(1.0, 1.0, 1.0, 1.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
			**interpolatef**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, positionmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragDepth = filterf(gl_TexCoord[0].st, depthmap, vec4(1.0, 1.0, 1.0, 1.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
			**interpolatef**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, normalmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragData[2] = filter4f(gl_TexCoord[0].st, positionmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragDepth = filterf(gl_TexCoord[0].st, depthmap, vec4(1.0, 1.0, 1.0, 1.0));\n\
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
            **cubic_fun**\n\
            **interpolate4f**\n\
            \n\
            void main()\n\
            {\n\
                gl_FragColor = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, normalmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, positionmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, normalmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragData[2] = filter4f(gl_TexCoord[0].st, positionmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
			**interpolatef**\n\
            \n\
            void main()\n\
            {\n\
                gl_FragColor = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragDepth = filterf(gl_TexCoord[0].st, depthmap, vec4(1.0, 1.0, 1.0, 1.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
			**interpolatef**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, normalmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragDepth = filterf(gl_TexCoord[0].st, depthmap, vec4(1.0, 1.0, 1.0, 1.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
			**interpolatef**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, positionmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragDepth = filterf(gl_TexCoord[0].st, depthmap, vec4(1.0, 1.0, 1.0, 1.0));\n\
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
			**sample_fun**\n\
            **cubic_fun**\n\
            **interpolate4f**\n\
			**interpolatef**\n\
            \n\
			void main()\n\
			{\n\
                gl_FragData[0] = filter4f(gl_TexCoord[0].st, cubemap, **bgColor**);\n\
				gl_FragData[1] = filter4f(gl_TexCoord[0].st, normalmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragData[2] = filter4f(gl_TexCoord[0].st, positionmap, vec4(0.0, 0.0, 0.0, 0.0));\n\
				gl_FragDepth = filterf(gl_TexCoord[0].st, depthmap, vec4(1.0, 1.0, 1.0, 1.0));\n\
			}\n"; 
	}//end shaders
}
#endif
