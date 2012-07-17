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

#include <stdio.h>
#include <fstream>
#include <GL/glew.h>
#if __WIN32__
#include <GL/wglew.h>
#elif __LINUX__
#include <GL/glext.h>
#else
#include <OpenGL/glext.h>
#endif
#include <GL/glfw.h>

#include "../include/sgct/CorrectionMesh.h"

bool core_sgct::CorrectionMesh::readAndGenerateMesh(const char * meshPath)
{
	if( meshPath == NULL )
	{
	    return false;
	}

	FILE * meshFile;
	meshFile = fopen(meshPath, "r");
	
	if( meshFile == NULL )
		return false;

	float x,y,s,t;
	unsigned char intensity;
	unsigned int a,b,c;
	unsigned int numOfVerticesRead = 0;
	unsigned int numOfFacesRead = 0;

	char lineBuffer[256];
	while( !feof( meshFile ) )
	{
		if( fgets(lineBuffer, 256, meshFile ) != NULL )
		{
			if( sscanf(lineBuffer, "%f %f %c %f %f", &x, &y, &intensity, &s, &t) == 5 )
			{
				numOfVerticesRead++;
				//fprintf(stderr, "Vertex data: %f %f %c %f %f\n", x, y, intensity, s, t);
			}
			else if( sscanf(lineBuffer, "[ %u %u %u ]", &a, &b, &c) == 3 )
			{
				numOfFacesRead++;
			}
		}

	}

	fprintf(stderr, "Vertices read: %u\n", numOfVerticesRead);
	fprintf(stderr, "Faces read: %u\n", numOfFacesRead);

	fclose( meshFile );

	return true;
}
