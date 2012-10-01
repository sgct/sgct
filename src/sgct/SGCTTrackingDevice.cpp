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

#include <stdlib.h>
#include <stdio.h>
#include "../include/sgct/SGCTTrackingDevice.h"

core_sgct::SGCTTrackingDevice::SGCTTrackingDevice(size_t index, const char * name)
{
	mEnabled = true;
	mName.assign( name );
	mIndex = index;
	mNumberOfButtons = 0;
	mNumberOfAxes = 0;
	mTrackedPos = glm::dvec4(1.0);

	mButtons = NULL;
	mAxes = NULL;
}

core_sgct::SGCTTrackingDevice::~SGCTTrackingDevice()
{
	mEnabled = false;

	if( mButtons != NULL )
	{
		delete [] mButtons;
		mButtons = NULL;
	}

	if( mAxes != NULL )
	{
		delete [] mAxes;
		mAxes = NULL;
	}
}

void core_sgct::SGCTTrackingDevice::setEnabled(bool state)
{
	mEnabled = state;
}

void core_sgct::SGCTTrackingDevice::setNumberOfButtons(size_t numOfButtons)
{
	if( mButtons == NULL )
	{
		mButtons = new bool[numOfButtons];
		mNumberOfButtons = numOfButtons;
		for(size_t i=0; i<mNumberOfButtons; i++)
			mButtons[i] = false;
	}
}

void core_sgct::SGCTTrackingDevice::setNumberOfAxes(size_t numOfAxes)
{
	if( mAxes == NULL )
	{
		mAxes = new double[numOfAxes];
		mNumberOfAxes = numOfAxes;
		for(size_t i=0; i<mNumberOfAxes; i++)
			mAxes[i] = 0.0;
	}
}

void core_sgct::SGCTTrackingDevice::setPosition(const double &x, const double &y, const double &z)
{
	mTrackedPos = glm::dvec4( x, y, z, 1.0 );
}

void core_sgct::SGCTTrackingDevice::setRotation(const double &w, const double &x, const double &y, const double &z)
{
	mTrackedRot = glm::dquat(w, x, y, z);
}

void core_sgct::SGCTTrackingDevice::setButtonVal(const bool val, size_t index)
{
	if( index < mNumberOfButtons )
	{
		mButtons[index] = val;
	}
}

void core_sgct::SGCTTrackingDevice::setAnalogVal(const double &val, size_t index)
{
	if( index < mNumberOfAxes )
	{
		mAxes[index] = val;
	}
}
