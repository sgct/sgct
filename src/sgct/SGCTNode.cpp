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

#include "../include/sgct/SGCTNode.h"

core_sgct::SGCTNode::SGCTNode()
{
	mCurrentViewportIndex = 0;
	numberOfSamples = 1;

	//init optional parameters
	swapInterval = 1;
}

void core_sgct::SGCTNode::addViewport(float left, float right, float bottom, float top)
{
	Viewport tmpVP(left, right, bottom, top);
	mViewports.push_back(tmpVP);
}

void core_sgct::SGCTNode::addViewport(core_sgct::Viewport &vp)
{
	mViewports.push_back(vp);
}

core_sgct::Viewport * core_sgct::SGCTNode::getCurrentViewport()
{
	return &mViewports[mCurrentViewportIndex];
}

core_sgct::Viewport * core_sgct::SGCTNode::getViewport(unsigned int index)
{
	return &mViewports[index];
}

void core_sgct::SGCTNode::getCurrentViewportPixelCoords(int &x, int &y, int &xSize, int &ySize)
{
	x = static_cast<int>(getCurrentViewport()->getX() *
		static_cast<double>(getWindowPtr()->getHResolution()));
	y = static_cast<int>(getCurrentViewport()->getY() *
		static_cast<double>(getWindowPtr()->getVResolution()));
	xSize = static_cast<int>(getCurrentViewport()->getXSize() *
		static_cast<double>(getWindowPtr()->getHResolution()));
	ySize = static_cast<int>(getCurrentViewport()->getYSize() *
		static_cast<double>(getWindowPtr()->getVResolution()));
}

unsigned int core_sgct::SGCTNode::getNumberOfViewports()
{
	return mViewports.size();
}
