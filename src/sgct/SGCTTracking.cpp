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
#include "../include/sgct/SGCTTracking.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/vrpn/vrpn_Tracker.h"

class vrpn_Tracker_Remote;
vrpn_Tracker_Remote * mTracker = NULL;
void VRPN_CALLBACK update_position_cb(void *userdata, const vrpn_TRACKERCB t);

core_sgct::SGCTTracking::~SGCTTracking()
{
	delete mTracker;
	mTracker = NULL;
}

void core_sgct::SGCTTracking::connect(const char * name)
{
	mTracker = new vrpn_Tracker_Remote(name);
	mTracker->register_change_handler(NULL, update_position_cb);
}

void core_sgct::SGCTTracking::update()
{
	if(mTracker != NULL)
		mTracker->mainloop();
}

void VRPN_CALLBACK update_position_cb(void *userdata, const vrpn_TRACKERCB info)
{
	printf("Sensor %d is now at (%g,%g,%g)\n",
	info.sensor, info.pos[0], info.pos[1], info.pos[2]);

	//if(info.sensor == 0)
		core_sgct::ClusterManager::Instance()->getUserPtr()->setPos((double *)info.pos);
}
