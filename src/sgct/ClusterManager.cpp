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

#include "../include/sgct/ClusterManager.h"
#include <glm/gtx/euler_angles.hpp>

core_sgct::ClusterManager * core_sgct::ClusterManager::mInstance = NULL;

core_sgct::ClusterManager::ClusterManager(void)
{
	masterIndex = -1;
	mThisNodeId = -1;
	validCluster = false;
	mUser = new SGCTUser();
	mTrackingManager = new sgct::SGCTTrackingManager();
	mSceneTrans = glm::mat4(1.0f);
	mMeshImpl = DISPLAY_LIST; //default
}

core_sgct::ClusterManager::~ClusterManager()
{
	nodes.clear();
	
	delete mUser;
	mUser = NULL;

	delete mTrackingManager;
	mTrackingManager = NULL;
}

void core_sgct::ClusterManager::addNode(core_sgct::SGCTNode node)
{
	nodes.push_back(node);
}

core_sgct::SGCTNode * core_sgct::ClusterManager::getNodePtr(unsigned int index)
{
	return &nodes[index];
}

core_sgct::SGCTNode * core_sgct::ClusterManager::getThisNodePtr()
{
	return mThisNodeId < 0 ? NULL : &nodes[mThisNodeId];
}

void core_sgct::ClusterManager::updateSceneTransformation(float yaw, float pitch, float roll, glm::vec3 offset)
{
	mSceneTrans =
		glm::yawPitchRoll(
			yaw,
			pitch,
			roll)
        * glm::translate( glm::mat4(1.0f), offset);
}