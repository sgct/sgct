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

namespace core_sgct
{
	/*
		All shaders are in GLSL 1.2 for compability with Mac OS X
	*/
	namespace shaders
	{
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
	}
}
#endif