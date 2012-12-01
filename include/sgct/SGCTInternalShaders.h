/*************************************************************************
Copyright (c) 2012 Miroslav Andel, Linköping University.
All rights reserved.

Original Authors:
Miroslav Andel, Alexander Fridlund

For any questions or information about the SGCT project please contact: miroslav.andel@liu.se

This work is licensed under the Creative Commons Attribution-ShareAlike 3.0 Unported License.
To view a copy of this license, visit http://creativecommons.org/licenses/by-sa/3.0/ or send a letter to
Creative Commons, 444 Castro Street, Suite 900, Mountain View, California, 94041, USA.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*************************************************************************/

#ifndef _SGCT_INTERNAL_SHADERS_H_
#define _SGCT_INTERNAL_SHADERS_H_

#include <string>

namespace sgct_core
{
	/*
		All shaders are in GLSL 1.2 for compability with Mac OS X
	*/
	namespace shaders
	{
		/*

		#version 120

		void main()
		{
			gl_TexCoord[0] = gl_MultiTexCoord0;
			gl_TexCoord[1] = gl_MultiTexCoord1;
			gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
		}

		*/
		const std::string Base_Vert_Shader = "\
			#version 120\n\
			\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_TexCoord[1] = gl_MultiTexCoord1;\n\
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
			}\n";

		/*

		#version 120

		uniform samplerCube cubemap;
		uniform float halfFov;
		float quarter_pi = 0.7853981634;

		void main()
		{
			float s = 2.0 * (gl_TexCoord[0].s - 0.5);
			float t = 2.0 * (gl_TexCoord[0].t - 0.5);
			vec4 color;
			if( s*s + t*t <= 1.0 )
			{
				float phi = sqrt(s*s + t*t) * halfFov;
				float theta = atan(s,t);
				float x = sin(phi) * sin(theta);
				float y = -sin(phi) * cos(theta);
				float z = cos(phi);
				vec3 ReflectDir = vec3(x, y, z);
				\\Since we only use four faces the cubemap is rotated 45 degrees
				vec3 rotVec = vec3( cos(quarter_pi)*x + sin(quarter_pi)*z, y, -sin(quarter_pi)*x + cos(quarter_pi)*z);
				color = vec4(textureCube(cubemap, rotVec));
			}
			else
				color = vec4(0.0, 0.0, 0.0, 0.0);
			gl_FragColor = color;
		}

		*/
		const std::string Fisheye_Frag_Shader = "\
			#version 120\n\
			\n\
			uniform samplerCube cubemap;\n\
			uniform float halfFov;\n\
			float quarter_pi = 0.7853981634;\n\
			\n\
			void main()\n\
			{\n\
				float s = 2.0 * (gl_TexCoord[0].s - 0.5);\n\
				float t = 2.0 * (gl_TexCoord[0].t - 0.5);\n\
				vec4 color;\n\
				if( s*s + t*t <= 1.0 )\n\
				{\n\
					float phi = sqrt(s*s + t*t) * halfFov;\n\
					float theta = atan(s,t);\n\
					float x = sin(phi) * sin(theta);\n\
					float y = -sin(phi) * cos(theta);\n\
					float z = cos(phi);\n\
					vec3 ReflectDir = vec3(x, y, z);\n\
					vec3 rotVec = vec3( cos(quarter_pi)*x + sin(quarter_pi)*z, y, -sin(quarter_pi)*x + cos(quarter_pi)*z);\n\
					color = vec4(textureCube(cubemap, rotVec));\n\
				}\n\
				else\n\
					color = vec4(0.0, 0.0, 0.0, 0.0);\n\
				gl_FragColor = color;\n\
			}\n";

		/*

		#version 120
		uniform sampler2D LeftTex;
		uniform sampler2D RightTex;

		void main()
		{
			gl_TexCoord[0] = gl_MultiTexCoord0;
			gl_TexCoord[1] = gl_MultiTexCoord1;

			gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
			gl_FrontColor = gl_Color;
		}

		*/
		const std::string Anaglyph_Vert_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				gl_TexCoord[0] = gl_MultiTexCoord0;\n\
				gl_TexCoord[1] = gl_MultiTexCoord1;\n\
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
				gl_FrontColor = gl_Color;\n\
			}\n";

		/*

		#version 120

		uniform sampler2D LeftTex;
		uniform sampler2D RightTex;

		void main()
		{
			vec4 leftVals = texture2D( LeftTex, gl_TexCoord[0].st);
			float leftLum = 0.3 * leftVals.r + 0.59 * leftVals.g + 0.11 * leftVals.b;

			vec4 rightVals = texture2D( RightTex, gl_TexCoord[1].st);
			float rightLum = 0.3 * rightVals.r + 0.59 * rightVals.g + 0.11 * rightVals.b;

			gl_FragColor.r = gl_Color.r * leftLum;
			gl_FragColor.g = gl_Color.g * rightLum;
			gl_FragColor.b = gl_Color.b * rightLum;
			gl_FragColor.a = gl_Color.a * (leftVals.a*0.5 + rightVals.a*0.5);
		}

		*/
		const std::string Anaglyph_Red_Cyan_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec4 leftVals = texture2D( LeftTex, gl_TexCoord[0].st);\n\
				float leftLum = 0.3 * leftVals.r + 0.59 * leftVals.g + 0.11 * leftVals.b;\n\
				vec4 rightVals = texture2D( RightTex, gl_TexCoord[1].st);\n\
				float rightLum = 0.3 * rightVals.r + 0.59 * rightVals.g + 0.11 * rightVals.b;\n\
				gl_FragColor.r = gl_Color.r * leftLum;\n\
				gl_FragColor.g = gl_Color.g * rightLum;\n\
				gl_FragColor.b = gl_Color.b * rightLum;\n\
				gl_FragColor.a = gl_Color.a * (leftVals.a*0.5 + rightVals.a*0.5);\n\
			}\n";

		/*

		#version 120

		uniform sampler2D LeftTex;
		uniform sampler2D RightTex;

		void main()
		{
			vec4 leftVals = texture2D( LeftTex, gl_TexCoord[0].st);
			vec4 rightVals = texture2D( RightTex, gl_TexCoord[1].st);

			vec3 coef = vec3(0.15, 0.15, 0.70);

			float rightMix = dot(rightVals.rbg, coef);
			gl_FragColor.r = gl_Color.r * leftVals.r;
			gl_FragColor.g = gl_Color.g * leftVals.g;
			gl_FragColor.b = gl_Color.b * rightMix;
			gl_FragColor.a = gl_Color.a * (leftVals.a*0.5 + rightVals.a*0.5);
		}

		*/
		const std::string Anaglyph_Amber_Blue_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				vec4 leftVals = texture2D( LeftTex, gl_TexCoord[0].st);\n\
				vec4 rightVals = texture2D( RightTex, gl_TexCoord[1].st);\n\
				vec3 coef = vec3(0.15, 0.15, 0.70);\n\
				float rightMix = dot(rightVals.rbg, coef);\n\
				gl_FragColor.r = gl_Color.r * leftVals.r;\n\
				gl_FragColor.g = gl_Color.g * leftVals.g;\n\
				gl_FragColor.b = gl_Color.b * rightMix;\n\
				gl_FragColor.a = gl_Color.a * (leftVals.a*0.5 + rightVals.a*0.5);\n\
			}\n";

		/*

		#version 120

		uniform sampler2D LeftTex;
		uniform sampler2D RightTex;

		void main()
		{
			//slow
			//float fval = gl_FragCoord.y + gl_FragCoord.x;
			//if( mod(fval,2.0) == 0.0 )

			//fast
			float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;
			if( (fval - floor(fval)) == 0.0 )
				gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[1].st);
			else
				gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st);
		}

		*/

		const std::string CheckerBoard_Frag_Shader = "\
			#version 120\n\
			uniform sampler2D LeftTex;\n\
			uniform sampler2D RightTex;\n\
			void main()\n\
			{\n\
				float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;\n\
				if( (fval - floor(fval)) == 0.0 )\n\
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[1].st);\n\
				else\n\
					gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st);\n\
			}\n";

		/*

		#version 120
		uniform sampler2D LeftTex;
		uniform sampler2D RightTex;

		void main()
		{
			//slow
			//float fval = gl_FragCoord.y + gl_FragCoord.x;
			//if( mod(fval,2.0) == 0.0 )

			//faster
			float fval = (gl_FragCoord.x + gl_FragCoord.y) * 0.5;
			if( (fval - floor(fval)) == 0.0 )
				gl_FragColor = gl_Color * texture2D( LeftTex, gl_TexCoord[0].st);
			else
				gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[1].st);
		}

		*/

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
					gl_FragColor = gl_Color * texture2D( RightTex, gl_TexCoord[1].st);\n\
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

		const std::string FXAA_FRAG_Shader = "\
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
			#define FxaaTexLod0(t, p) texture2DLod(t, p, 0.0)\n\
			#define FxaaTexOff(t, p, o, r) texture2DLodOffset(t, p, 0.0, o)\n\
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
			vec3 rgbNW = FxaaTexLod0(tex, posPos.zw).xyz;\n\
			vec3 rgbNE = FxaaTexOff(tex, posPos.zw, FxaaInt2(1,0), rcpFrame.xy).xyz;\n\
			vec3 rgbSW = FxaaTexOff(tex, posPos.zw, FxaaInt2(0,1), rcpFrame.xy).xyz;\n\
			vec3 rgbSE = FxaaTexOff(tex, posPos.zw, FxaaInt2(1,1), rcpFrame.xy).xyz;\n\
			vec3 rgbM  = FxaaTexLod0(tex, posPos.xy).xyz;\n\
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
				FxaaTexLod0(tex, posPos.xy + dir * (1.0/3.0 - 0.5)).xyz +\n\
				FxaaTexLod0(tex, posPos.xy + dir * (2.0/3.0 - 0.5)).xyz);\n\
			vec3 rgbB = rgbA * (1.0/2.0) + (1.0/4.0) * (\n\
				FxaaTexLod0(tex, posPos.xy + dir * (0.0/3.0 - 0.5)).xyz + \n\
				FxaaTexLod0(tex, posPos.xy + dir * (3.0/3.0 - 0.5)).xyz); \n\
			float lumaB = dot(rgbB, luma); \n\
			if((lumaB < lumaMin) || (lumaB > lumaMax)) return rgbA;\n\
				return rgbB; }\n\
			\n\
			vec4 PostFX(sampler2D tex, vec2 uv, float time)\n\
			{\n\
			  vec4 c = vec4(0.0);\n\
			  vec2 rcpFrame = vec2(1.0/rt_w, 1.0/rt_h);\n\
				c.rgb = FxaaPixelShader(posPos, tex, rcpFrame);\n\
				//c.rgb = 1.0 - texture2D(tex, posPos.xy).rgb;\n\
				c.a = 1.0;\n\
				return c;\n\
			}\n\
			\n\
			void main()\n\
			{\n\
				/*\n\
				vec2 uv = gl_TexCoord[0].xy;\n\
				vec3 tc = vec3(1.0, 0.0, 0.0);\n\
				if (uv.x < (vx_offset-0.005))\n\
				{\n\
					tc = PostFX(tex0, uv, 0.0).xyz;\n\
				}\n\
				else if (uv.x>=(vx_offset+0.005))\n\
				{\n\
					tc = texture2D(tex0, uv).rgb;\n\
				}\n\
				gl_FragColor = vec4(tc, 1.0);\n\
				*/\n\
				\n\
				/*\n\
				vec2 uv = gl_TexCoord[0].st;\n\
				if (uv.y > 0.5)\n\
				{\n\
					gl_FragColor = PostFX(tex0, uv, 0.0);\n\
				}\n\
				else\n\
				{\n\
					uv.y += 0.5;\n\
					vec4 c1 = texture2D(tex0, uv);\n\
					gl_FragColor = c1;\n\
				}\n\
				*/\n\
				vec2 uv = gl_TexCoord[0].st;\n\
				gl_FragColor = PostFX(tex0, uv, 0.0);\n\
			}\n";

	}//end shaders
}
#endif
