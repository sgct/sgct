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

#include <glm/gtc/constants.hpp>
#include "../include/sgct/utils/SGCTDome.h"
#include "../include/sgct/ogl_headers.h"
#include "../include/sgct/MessageHandler.h"
#include "../include/sgct/Engine.h"
#include "../include/sgct/SGCTSettings.h"

sgct_utils::SGCTDome::SGCTDome(float radius, float FOV, unsigned int segments, unsigned int rings, unsigned int resolution)
{
	mVerts = NULL;
	mResolution = resolution;
	mRings = rings;
	mSegments = segments;
	mVBO = 0;

	if(mResolution < 4) //must be four or higher
	{
		sgct::MessageHandler::Instance()->print("Warning: Dome geometry resolution must be higher than 4.\n");
		mResolution = 4;
	}

	mNumberOfVertices = (mSegments * ((mResolution/4)+1) + mRings * mResolution)*6;

	mVerts = new float[mNumberOfVertices];
	memset(mVerts, 0, mNumberOfVertices * sizeof(float));

	float elevationAngle, theta;
	float x, y, z;
	unsigned int pos = 0;

	//create rings
	for(unsigned int r = 1; r <= mRings; r++)
	{
		elevationAngle = glm::radians<float>((FOV/2.0f) * (static_cast<float>(r)/static_cast<float>(mRings)));
		y = radius * cosf( elevationAngle );

		for(unsigned int i = 0; i < mResolution; i++)
		{
			theta = glm::pi<float>() * 2.0f * (static_cast<float>(i)/static_cast<float>(mResolution)); 
			
			x = radius * sinf( elevationAngle ) * cosf(theta);
			z = radius * sinf( elevationAngle ) * sinf(theta);
			mVerts[pos] = x;
			mVerts[pos + 1] = y;
			mVerts[pos + 2] = z;

			pos += 3;
		}
	}

	//create segments
	for(unsigned int s = 0; s < mSegments; s++)
	{
		theta = (glm::pi<float>() * 2.0f) * (static_cast<float>(s)/static_cast<float>(mSegments)); 

		for(unsigned int i = 0; i < (mResolution/4)+1; i++)
		{
			elevationAngle = glm::radians<float>(FOV/2.0f) * (static_cast<float>(i)/static_cast<float>(mResolution/4));
			x = radius * sinf( elevationAngle ) * cosf(theta);
			y = radius * cosf( elevationAngle );
			z = radius * sinf( elevationAngle ) * sinf(theta);

			mVerts[pos] = x;
			mVerts[pos + 1] = y;
			mVerts[pos + 2] = z;

			pos += 3;
		}
	}

	createVBO();
}

void sgct_utils::SGCTDome::draw()
{
	glPushMatrix();
	glRotatef(-sgct_core::SGCTSettings::Instance()->getFisheyeTilt(), 1.0f, 0.0f, 0.0f); 
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);

	glVertexPointer(3, GL_FLOAT, 0, NULL);

	for(unsigned int r=0; r<mRings; r++)
		glDrawArrays(GL_LINE_LOOP, r * mResolution, mResolution);
	for(unsigned int s=0; s<mSegments; s++)
		glDrawArrays(GL_LINE_STRIP, mRings * mResolution + s * ((mResolution/4)+1), (mResolution/4)+1);

	glDisableClientState(GL_VERTEX_ARRAY);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glPopMatrix();
}

void sgct_utils::SGCTDome::createVBO()
{
	glGenBuffers(1, &mVBO);

	glBindBuffer(GL_ARRAY_BUFFER, mVBO);
	glBufferData(GL_ARRAY_BUFFER, mNumberOfVertices * sizeof(float), mVerts, GL_STATIC_DRAW);

	//unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}