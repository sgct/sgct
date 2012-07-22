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

#ifndef _CORRECTION_MESH_H_
#define _CORRECTION_MESH_H_

struct CorrectionMeshVertex
{
	float x, y;	//Vertex 8
	float s0, t0;	//Texcoord0 8
	float s1, t1;	//Texcoord1 8
	unsigned char r, g, b; //color 3

	//ATI performs better using sizes of power of two
	unsigned char padding[5]; //32 - 8 - 8 - 8 - 3 = 5
};

namespace core_sgct
{

/*!
Helper class for reading and rendering a correction mesh.
A correction mesh is used for warping and edge-blending.
*/
class CorrectionMesh
{
public:
	CorrectionMesh();
	~CorrectionMesh();
	void setViewportCoords(float vpXSize, float vpYSize, float vpXPos, float vpYPos);
	bool readAndGenerateMesh(const char * meshPath);
	void render();
	inline const double * getOrthoCoords() { return &mOrthoCoords[0]; }

private:
	void createMesh();
	void cleanUp();
	void renderMesh();

	enum buffer { Vertex = 0, Index };

	CorrectionMeshVertex * mVertices;
	CorrectionMeshVertex * mVertexList;
	unsigned int * mFaces;
    double mOrthoCoords[5];
	unsigned int mResolution[2];

	unsigned int mNumberOfVertices;
	unsigned int mNumberOfFaces;
	unsigned int mMeshData[2];

	float mXSize;
	float mYSize;
	float mXOffset;
	float mYOffset;
	
	bool hasMesh;
};

} //core_sgct

#endif