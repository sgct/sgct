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

#ifndef _SGCT_SPHERE
#define _SGCT_SPHERE

struct VertexData
{	
	float s, t;	//Texcoord0 8
	float nx, ny, nz; //12
	float x, y, z;	//12 = total 32 = power of two

	//ATI performs better using sizes of power of two
};

namespace sgct_utils
{

/*!

*/
class SGCTSphere
{
public:
	SGCTSphere(float radius, unsigned int segments);
	~SGCTSphere();
	void draw();

private:
	void addVertexData(unsigned int pos,
		const float &t, const float &s,
		const float &nx, const float &ny, const float &nz,
		const float &x, const float &y, const float &z);

	void createVBO();
	void cleanUp();

private:
	VertexData * mVerts;
	unsigned int * mIndices;

	unsigned int mNumberOfVertices;
	unsigned int mNumberOfFaces;

	enum bufferType { Vertex = 0, Index };
	unsigned int mVBO[2];
};

}

#endif