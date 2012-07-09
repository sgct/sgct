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

#ifndef _FRUSTUM
#define _FRUSTUM

namespace core_sgct
{

class Frustum
{
public:
	// Frustum mode enum
	enum FrustumMode { Mono = 0, StereoLeftEye, StereoRightEye };

	Frustum()
	{
		mLeft = -1.0f;
		mRight = 1.0f;
		mBottom = -1.0f;
		mTop = 1.0f;
		mNear = 0.1f;
		mFar = 100.0f;
	}

	Frustum(float left, float right, float bottom, float top, float nearClippingPlane=0.1f, float farClippingPlane=100.0f)
	{
		mLeft = left;
		mRight = right;
		mBottom = bottom;
		mTop = top;
		mNear = nearClippingPlane;
		mFar = farClippingPlane;
	}

	void set(float left, float right, float bottom, float top, float nearClippingPlane=0.1f, float farClippingPlane=100.0f)
	{
		mLeft = left;
		mRight = right;
		mBottom = bottom;
		mTop = top;
		mNear = nearClippingPlane;
		mFar = farClippingPlane;
	}

	inline float getLeft() { return mLeft; }
	inline float getRight() { return mRight; }
	inline float getBottom() { return mBottom; }
	inline float getTop() { return mTop; }
	inline float getNear() { return mNear; }
	inline float getFar() { return mFar; }

private:
	float mLeft;
	float mRight;
	float mBottom;
	float mTop;
	float mNear;
	float mFar;
};

}

#endif