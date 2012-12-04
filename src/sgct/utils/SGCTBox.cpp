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

#include "../include/sgct/utils/SGCTBox.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Engine.h"

sgct_utils::SGCTBox::SGCTBox(float size, TextureMappingMode tmm)
{
	//init
	mVBO = 0;
	mVerts = NULL;

	mVerts = new sgct_helpers::SGCTVertexData[36];
	memset(mVerts, 0, 36 * sizeof(sgct_helpers::SGCTVertexData));

	//populate the array
	if(tmm == Regular)
	{
		//A (front/+z)
		mVerts[0].set(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -size, size, size);
		mVerts[1].set(0.0f, 0.0f, 0.0f, 0.0f, 1.0f, -size, -size, size);
		mVerts[2].set(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, size, -size, size);
		mVerts[3].set(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -size, size, size);
		mVerts[4].set(1.0f, 0.0f, 0.0f, 0.0f, 1.0f, size, -size, size);
		mVerts[5].set(1.0f, 1.0f, 0.0f, 0.0f, 1.0f, size, size, size);

		//B (right/+x)
		mVerts[6].set(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, size, size, size);
		mVerts[7].set(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, size, -size, size);
		mVerts[8].set(1.0f, 0.0f, 1.0f, 0.0f, 0.0f, size, -size, -size);
		mVerts[9].set(0.0f, 1.0f, 1.0f, 0.0f, 0.0f, size, size, size);
		mVerts[10].set(1.0f, 0.0f, 1.0f, 0.0f, 0.0f, size, -size, -size);
		mVerts[11].set(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, size, size, -size);

		//C (Back/-z)
		mVerts[12].set(0.0f, 1.0f, 0.0f, 0.0f, -1.0f, size, size, -size);
		mVerts[13].set(0.0f, 0.0f, 0.0f, 0.0f, -1.0f, size, -size, -size);
		mVerts[14].set(1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -size, -size, -size);
		mVerts[15].set(0.0f, 1.0f, 0.0f, 0.0f, -1.0f, size, size, -size);
		mVerts[16].set(1.0f, 0.0f, 0.0f, 0.0f, -1.0f, -size, -size, -size);
		mVerts[17].set(1.0f, 1.0f, 0.0f, 0.0f, -1.0f, -size, size, -size);

		//D (Left/-x)
		mVerts[18].set(0.0f, 1.0f, -1.0f, 0.0f, 0.0f, -size, size, -size);
		mVerts[19].set(0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -size, -size, -size);
		mVerts[20].set(1.0f, 0.0f, -1.0f, 0.0f, 0.0f, -size, -size, size);
		mVerts[21].set(0.0f, 1.0f, -1.0f, 0.0f, 0.0f, -size, size, -size);
		mVerts[22].set(1.0f, 0.0f, -1.0f, 0.0f, 0.0f, -size, -size, size);
		mVerts[23].set(1.0f, 1.0f, -1.0f, 0.0f, 0.0f, -size, size, size);

		//E (Top/+y)
		mVerts[24].set(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -size, size, -size);
		mVerts[25].set(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, -size, size, size);
		mVerts[26].set(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, size, size, size);
		mVerts[27].set(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, -size, size, -size);
		mVerts[28].set(1.0f, 0.0f, 0.0f, 1.0f, 0.0f, size, size, size);
		mVerts[29].set(1.0f, 1.0f, 0.0f, 1.0f, 0.0f, size, size, -size);

		//F (Bottom/-y)
		mVerts[30].set(0.0f, 1.0f, 0.0f, -1.0f, 0.0f, -size, -size, size);
		mVerts[31].set(0.0f, 0.0f, 0.0f, -1.0f, 0.0f, -size, -size, -size);
		mVerts[32].set(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, size, -size, -size);
		mVerts[33].set(0.0f, 1.0f, 0.0f, -1.0f, 0.0f, -size, -size, size);
		mVerts[34].set(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, size, -size, -size);
		mVerts[35].set(1.0f, 1.0f, 0.0f, -1.0f, 0.0f, size, -size, size);
	}
	else if(tmm == CubeMap)
	{
		//A (front/+z)
		mVerts[0].set(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -size, size, size);
		mVerts[1].set(0.0f, 0.5f, 0.0f, 0.0f, 1.0f, -size, -size, size);
		mVerts[2].set(0.333333f, 0.5f, 0.0f, 0.0f, 1.0f, size, -size, size);
		mVerts[3].set(0.0f, 1.0f, 0.0f, 0.0f, 1.0f, -size, size, size);
		mVerts[4].set(0.333333f, 0.5f, 0.0f, 0.0f, 1.0f, size, -size, size);
		mVerts[5].set(0.333333f, 1.0f, 0.0f, 0.0f, 1.0f, size, size, size);

		//B (right/+x)
		mVerts[6].set(0.333334f, 1.0f, 1.0f, 0.0f, 0.0f, size, size, size);
		mVerts[7].set(0.333334f, 0.5f, 1.0f, 0.0f, 0.0f, size, -size, size);
		mVerts[8].set(0.666666f, 0.5f, 1.0f, 0.0f, 0.0f, size, -size, -size);
		mVerts[9].set(0.333334f, 1.0f, 1.0f, 0.0f, 0.0f, size, size, size);
		mVerts[10].set(0.666666f, 0.5f, 1.0f, 0.0f, 0.0f, size, -size, -size);
		mVerts[11].set(0.666666f, 1.0f, 1.0f, 0.0f, 0.0f, size, size, -size);

		//C (Back/-z)
		mVerts[12].set(0.666667f, 1.0f, 0.0f, 0.0f, -1.0f, size, size, -size);
		mVerts[13].set(0.666667f, 0.5f, 0.0f, 0.0f, -1.0f, size, -size, -size);
		mVerts[14].set(1.0f, 0.5f, 0.0f, 0.0f, -1.0f, -size, -size, -size);
		mVerts[15].set(0.666667f, 1.0f, 0.0f, 0.0f, -1.0f, size, size, -size);
		mVerts[16].set(1.0f, 0.5f, 0.0f, 0.0f, -1.0f, -size, -size, -size);
		mVerts[17].set(1.0f, 1.0f, 0.0f, 0.0f, -1.0f, -size, size, -size);

		//D (Left/-x)
		mVerts[18].set(0.0f, 0.5f, -1.0f, 0.0f, 0.0f, -size, size, -size);
		mVerts[19].set(0.0f, 0.0f, -1.0f, 0.0f, 0.0f, -size, -size, -size);
		mVerts[20].set(0.333333f, 0.0f, -1.0f, 0.0f, 0.0f, -size, -size, size);
		mVerts[21].set(0.0f, 0.5f, -1.0f, 0.0f, 0.0f, -size, size, -size);
		mVerts[22].set(0.333333f, 0.0f, -1.0f, 0.0f, 0.0f, -size, -size, size);
		mVerts[23].set(0.333333f, 0.5f, -1.0f, 0.0f, 0.0f, -size, size, size);

		//E (Top/+y)
		mVerts[24].set(0.333334f, 0.5f, 0.0f, 1.0f, 0.0f, -size, size, -size);
		mVerts[25].set(0.333334f, 0.0f, 0.0f, 1.0f, 0.0f, -size, size, size);
		mVerts[26].set(0.666666f, 0.0f, 0.0f, 1.0f, 0.0f, size, size, size);
		mVerts[27].set(0.333334f, 0.5f, 0.0f, 1.0f, 0.0f, -size, size, -size);
		mVerts[28].set(0.666666f, 0.0f, 0.0f, 1.0f, 0.0f, size, size, size);
		mVerts[29].set(0.666666f, 0.5f, 0.0f, 1.0f, 0.0f, size, size, -size);

		//F (Bottom/-y)
		mVerts[30].set(0.666667f, 0.5f, 0.0f, -1.0f, 0.0f, -size, -size, size);
		mVerts[31].set(0.666667f, 0.0f, 0.0f, -1.0f, 0.0f, -size, -size, -size);
		mVerts[32].set(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, size, -size, -size);
		mVerts[33].set(0.666667f, 0.5f, 0.0f, -1.0f, 0.0f, -size, -size, size);
		mVerts[34].set(1.0f, 0.0f, 0.0f, -1.0f, 0.0f, size, -size, -size);
		mVerts[35].set(1.0f, 0.5f, 0.0f, -1.0f, 0.0f, size, -size, size);
	}
	else //skybox
	{
		//A (front/+z)
		mVerts[0].set(1.000f, 0.666666f, 0.0f, 0.0f, 1.0f, -size, size, size);
		mVerts[1].set(1.000f, 0.333334f, 0.0f, 0.0f, 1.0f, -size, -size, size);
		mVerts[2].set(0.751f, 0.333334f, 0.0f, 0.0f, 1.0f, size, -size, size);
		mVerts[3].set(1.000f, 0.666666f, 0.0f, 0.0f, 1.0f, -size, size, size);
		mVerts[4].set(0.751f, 0.333334f, 0.0f, 0.0f, 1.0f, size, -size, size);
		mVerts[5].set(0.751f, 0.666666f, 0.0f, 0.0f, 1.0f, size, size, size);

		//B (right/+x)
		mVerts[6].set(0.749f, 0.666666f, 1.0f, 0.0f, 0.0f, size, size, size);
		mVerts[7].set(0.749f, 0.333334f, 1.0f, 0.0f, 0.0f, size, -size, size);
		mVerts[8].set(0.501f, 0.333334f, 1.0f, 0.0f, 0.0f, size, -size, -size);
		mVerts[9].set(0.749f, 0.666666f, 1.0f, 0.0f, 0.0f, size, size, size);
		mVerts[10].set(0.501f, 0.333334f, 1.0f, 0.0f, 0.0f, size, -size, -size);
		mVerts[11].set(0.501f, 0.666666f, 1.0f, 0.0f, 0.0f, size, size, -size);

		//C (Back/-z)
		mVerts[12].set(0.499f, 0.666666f, 0.0f, 0.0f, -1.0f, size, size, -size);
		mVerts[13].set(0.499f, 0.333334f, 0.0f, 0.0f, -1.0f, size, -size, -size);
		mVerts[14].set(0.251f, 0.333334f, 0.0f, 0.0f, -1.0f, -size, -size, -size);
		mVerts[15].set(0.499f, 0.666666f, 0.0f, 0.0f, -1.0f, size, size, -size);
		mVerts[16].set(0.251f, 0.333334f, 0.0f, 0.0f, -1.0f, -size, -size, -size);
		mVerts[17].set(0.251f, 0.666666f, 0.0f, 0.0f, -1.0f, -size, size, -size);

		//D (Left/-x)
		mVerts[18].set(0.249f, 0.666666f, -1.0f, 0.0f, 0.0f, -size, size, -size);
		mVerts[19].set(0.249f, 0.333334f, -1.0f, 0.0f, 0.0f, -size, -size, -size);
		mVerts[20].set(0.000f, 0.333334f, -1.0f, 0.0f, 0.0f, -size, -size, size);
		mVerts[21].set(0.249f, 0.666666f, -1.0f, 0.0f, 0.0f, -size, size, -size);
		mVerts[22].set(0.000f, 0.333334f, -1.0f, 0.0f, 0.0f, -size, -size, size);
		mVerts[23].set(0.000f, 0.666666f, -1.0f, 0.0f, 0.0f, -size, size, size);

		//E (Top/+y)
		mVerts[24].set(0.251f, 0.666667f, 0.0f, 1.0f, 0.0f, -size, size, -size);
		mVerts[25].set(0.251f, 1.000000f, 0.0f, 1.0f, 0.0f, -size, size, size);
		mVerts[26].set(0.499f, 1.000000f, 0.0f, 1.0f, 0.0f, size, size, size);
		mVerts[27].set(0.251f, 0.666667f, 0.0f, 1.0f, 0.0f, -size, size, -size);
		mVerts[28].set(0.499f, 1.000000f, 0.0f, 1.0f, 0.0f, size, size, size);
		mVerts[29].set(0.499f, 0.666667f, 0.0f, 1.0f, 0.0f, size, size, -size);

		//F (Bottom/-y)
		mVerts[30].set(0.251f, 0.000000f, 0.0f, -1.0f, 0.0f, -size, -size, size);
		mVerts[31].set(0.251f, 0.333333f, 0.0f, -1.0f, 0.0f, -size, -size, -size);
		mVerts[32].set(0.499f, 0.333333f, 0.0f, -1.0f, 0.0f, size, -size, -size);
		mVerts[33].set(0.251f, 0.000000f, 0.0f, -1.0f, 0.0f, -size, -size, size);
		mVerts[34].set(0.499f, 0.333333f, 0.0f, -1.0f, 0.0f, size, -size, -size);
		mVerts[35].set(0.499f, 0.000000f, 0.0f, -1.0f, 0.0f, size, -size, size);
	}

	createVBO();

	if( !sgct::Engine::checkForOGLErrors() ) //if error occured
	{
		sgct::MessageHandler::Instance()->print("SGCT Utils: Box creation error!\n");
		void cleanup();
	}

	//free data
	if( mVerts != NULL )
	{
		delete [] mVerts;
		mVerts = NULL;
	}
}

sgct_utils::SGCTBox::~SGCTBox()
{
	cleanUp();
}

void sgct_utils::SGCTBox::draw()
{
	//if not set
	if( mVBO == 0 )
		return;

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_VERTEX_ARRAY);
	
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	
	glInterleavedArrays(GL_T2F_N3F_V3F, 0, 0);
	glDrawArrays(GL_TRIANGLES, 0, 36);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glPopClientAttrib();
}

void sgct_utils::SGCTBox::cleanUp()
{
	//cleanup
	if(mVBO != 0)
	{
		glDeleteBuffers(1, &mVBO);
		mVBO = 0;
	}
}

void sgct_utils::SGCTBox::createVBO()
{
	glGenBuffers(1, &mVBO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, 36 * sizeof(sgct_helpers::SGCTVertexData), mVerts, GL_STATIC_DRAW);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}