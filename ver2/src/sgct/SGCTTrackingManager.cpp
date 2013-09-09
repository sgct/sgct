/*************************************************************************
Copyright (c) 2012-2013 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#include "../include/sgct/Engine.h"

#include "../include/vrpn/vrpn_Tracker.h"
#include "../include/vrpn/vrpn_Button.h"
#include "../include/vrpn/vrpn_Analog.h"

#include "../include/sgct/SGCTTracker.h"
#include "../include/sgct/ClusterManager.h"
#include "../include/sgct/MessageHandler.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/euler_angles.hpp>

struct VRPNPointer
{
	vrpn_Tracker_Remote * mSensorDevice;
	vrpn_Analog_Remote * mAnalogDevice;
	vrpn_Button_Remote * mButtonDevice;
};

struct VRPNTracker
{
	std::vector<VRPNPointer> mDevices;
};

std::vector<VRPNTracker> gTrackers;

void VRPN_CALLBACK update_tracker_cb(void *userdata, const vrpn_TRACKERCB t );
void VRPN_CALLBACK update_button_cb(void *userdata, const vrpn_BUTTONCB b );
void VRPN_CALLBACK update_analog_cb(void * userdata, const vrpn_ANALOGCB a );

void samplingLoop(void *arg);

sgct::SGCTTrackingManager::SGCTTrackingManager()
{
	mHead = NULL;
	mNumberOfDevices = 0;
	mSamplingThread = NULL;
	mSamplingTime = 0.0;
	mRunning = true;
}

bool sgct::SGCTTrackingManager::isRunning()
{
	bool tmpVal;
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Checking if tracking is running...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mRunning;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	return tmpVal;
}

sgct::SGCTTrackingManager::~SGCTTrackingManager()
{
	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Disconnecting VRPN...");

#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
	fprintf(stderr, "Destructing, setting running to false...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		mRunning = false;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	//destroy thread
	if( mSamplingThread != NULL )
	{
		mSamplingThread->join();
		delete mSamplingThread;
		mSamplingThread = NULL;
	}

	//delete all instances
	for(size_t i=0; i<mTrackers.size(); i++)
	{
		if( mTrackers[i] != NULL )
		{
			delete mTrackers[i];
			mTrackers[i] = NULL;
		}

		//clear vrpn pointers
		for(size_t j=0; j<gTrackers[i].mDevices.size(); j++)
		{
			//clear sensor pointers
			if( gTrackers[i].mDevices[j].mSensorDevice != NULL )
			{
				delete gTrackers[i].mDevices[j].mSensorDevice;
				gTrackers[i].mDevices[j].mSensorDevice = NULL;
			}

			//clear analog pointers
			if( gTrackers[i].mDevices[j].mAnalogDevice != NULL )
			{
				delete gTrackers[i].mDevices[j].mAnalogDevice;
				gTrackers[i].mDevices[j].mAnalogDevice = NULL;
			}

			//clear button pointers
			if( gTrackers[i].mDevices[j].mButtonDevice != NULL )
			{
				delete gTrackers[i].mDevices[j].mButtonDevice;
				gTrackers[i].mDevices[j].mButtonDevice = NULL;
			}
		}
		gTrackers[i].mDevices.clear();
	}

	mTrackers.clear();
	gTrackers.clear();

	MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, " done.\n");
}

void sgct::SGCTTrackingManager::startSampling()
{
	if( !mTrackers.empty() )
	{
		//link the head tracker
		setHeadTracker( sgct_core::ClusterManager::instance()->getUserPtr()->getHeadTrackerName(),
			sgct_core::ClusterManager::instance()->getUserPtr()->getHeadTrackerDeviceName() );

		mSamplingThread = new tthread::thread( samplingLoop, this );
	}
}

/*
	Update the user position if headtracking is used. This function is called from the engine.
*/
void sgct::SGCTTrackingManager::updateTrackingDevices()
{
	for(size_t i=0; i<mTrackers.size(); i++)
		for(size_t j=0; j<mTrackers[i]->getNumberOfDevices(); j++)
		{
			SGCTTrackingDevice * tdPtr = mTrackers[i]->getDevicePtr(j);
			if( tdPtr->isEnabled() && tdPtr == mHead )
			{
				sgct_core::ClusterManager * cm = sgct_core::ClusterManager::instance();

				//set head rot & pos
				cm->getUserPtr()->setTransform( tdPtr->getTransformMat() );
			}
		}
}

void sgct::SGCTTrackingManager::addTracker(std::string name)
{
	mTrackers.push_back( new SGCTTracker(name) );

	VRPNTracker tmpVRPNTracker;
	gTrackers.push_back( tmpVRPNTracker );
}

void sgct::SGCTTrackingManager::addDeviceToCurrentTracker(std::string name)
{
	mNumberOfDevices++;

	mTrackers.back()->addDevice(name, mTrackers.size() - 1);

	VRPNPointer tmpPtr;
	tmpPtr.mSensorDevice = NULL;
	tmpPtr.mAnalogDevice = NULL;
	tmpPtr.mButtonDevice = NULL;

	gTrackers.back().mDevices.push_back( tmpPtr );
}

void sgct::SGCTTrackingManager::addSensorToCurrentDevice(const char * address, int id)
{
	if(gTrackers.empty() || gTrackers.back().mDevices.empty())
		return;

	std::pair<std::set<std::string>::iterator, bool> retVal =
		mAddresses.insert( std::string(address) );

	VRPNPointer * ptr = &gTrackers.back().mDevices.back();
	SGCTTrackingDevice * devicePtr = mTrackers.back()->getLastDevicePtr();

	if( devicePtr != NULL)
	{
		devicePtr->setSensorId( id );

		if( retVal.second && (*ptr).mSensorDevice == NULL)
		{
			MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Tracking: Connecting to sensor '%s'...\n", address);
			(*ptr).mSensorDevice = new vrpn_Tracker_Remote( address );

			if( (*ptr).mSensorDevice != NULL )
			{
				(*ptr).mSensorDevice->register_change_handler( mTrackers.back(), update_tracker_cb);
			}
			else
				MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Tracking: Failed to connect to sensor '%s' on device %s!\n",
					address, devicePtr->getName().c_str());
		}
	}
	else
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Tracking: Failed to connect to sensor '%s'!\n",
			address);
}

void sgct::SGCTTrackingManager::addButtonsToCurrentDevice(const char * address, size_t numOfButtons)
{
	if(gTrackers.empty() || gTrackers.back().mDevices.empty())
		return;

	VRPNPointer * ptr = &gTrackers.back().mDevices.back();
	SGCTTrackingDevice * devicePtr = mTrackers.back()->getLastDevicePtr();

	if( (*ptr).mButtonDevice == NULL && devicePtr != NULL)
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Tracking: Connecting to buttons '%s' on device %s...\n",
					address, devicePtr->getName().c_str());

		(*ptr).mButtonDevice = new vrpn_Button_Remote( address );

		if( (*ptr).mButtonDevice != NULL ) //connected
		{
			(*ptr).mButtonDevice->register_change_handler(devicePtr, update_button_cb);
			devicePtr->setNumberOfButtons( numOfButtons );
		}
		else
			MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Tracking: Failed to connect to buttons '%s' on device %s!\n",
				address, devicePtr->getName().c_str());
	}
	else
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Tracking: Failed to connect to buttons '%s'!\n",
			address);
}

void sgct::SGCTTrackingManager::addAnalogsToCurrentDevice(const char * address, size_t numOfAxes)
{
	if(gTrackers.empty() || gTrackers.back().mDevices.empty())
		return;

	VRPNPointer * ptr = &gTrackers.back().mDevices.back();
	SGCTTrackingDevice * devicePtr = mTrackers.back()->getLastDevicePtr();

	if( (*ptr).mAnalogDevice == NULL && devicePtr != NULL)
	{
		MessageHandler::instance()->print(MessageHandler::NOTIFY_INFO, "Tracking: Connecting to analogs '%s' on device %s...\n",
				address, devicePtr->getName().c_str());

		(*ptr).mAnalogDevice = new vrpn_Analog_Remote( address );

		if( (*ptr).mAnalogDevice != NULL )
		{
			(*ptr).mAnalogDevice->register_change_handler(devicePtr, update_analog_cb);
			devicePtr->setNumberOfAxes( numOfAxes );
		}
		else
			MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Tracking: Failed to connect to analogs '%s' on device %s!\n",
				address, devicePtr->getName().c_str());
	}
	else
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Tracking: Failed to connect to analogs '%s'!\n",
				address);
}

void sgct::SGCTTrackingManager::setHeadTracker(const char * trackerName, const char * deviceName)
{
	SGCTTracker * trackerPtr = getTrackerPtr( trackerName );

	if( trackerPtr != NULL )
		mHead = trackerPtr->getDevicePtr( deviceName );

	if( mHead == NULL && strlen(trackerName) > 0 && strlen(deviceName) > 0 )
		MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "Tracking: Failed to set head tracker to %s@%s!\n",
				deviceName, trackerName);
}

void samplingLoop(void *arg)
{
	sgct::SGCTTrackingManager * tmPtr =
		reinterpret_cast<sgct::SGCTTrackingManager *>(arg);

	double t;
	bool running = true;

	while(running)
	{
		t = sgct::Engine::getTime();
		for(size_t i=0; i<tmPtr->getNumberOfTrackers(); i++)
		{
			sgct::SGCTTracker * trackerPtr = tmPtr->getTrackerPtr(i);

			if( trackerPtr != NULL )
			{
				for(size_t j=0; j<trackerPtr->getNumberOfDevices(); j++)
				{
					if( trackerPtr->getDevicePtr(j)->isEnabled() )
					{
						if( gTrackers[i].mDevices[j].mSensorDevice != NULL )
							gTrackers[i].mDevices[j].mSensorDevice->mainloop();

						if( gTrackers[i].mDevices[j].mAnalogDevice != NULL )
							gTrackers[i].mDevices[j].mAnalogDevice->mainloop();

						if( gTrackers[i].mDevices[j].mButtonDevice != NULL )
							gTrackers[i].mDevices[j].mButtonDevice->mainloop();
					}
				}
			}
		}

		running = tmPtr->isRunning();

		tmPtr->setSamplingTime(sgct::Engine::getTime() - t);

		// Sleep for 1ms so we don't eat the CPU
		vrpn_SleepMsecs(1);
	}
}

sgct::SGCTTracker * sgct::SGCTTrackingManager::getLastTrackerPtr()
{
	return mTrackers.size() > 0 ? mTrackers.back() : NULL;
}

sgct::SGCTTracker * sgct::SGCTTrackingManager::getTrackerPtr(size_t index)
{
	return index < mTrackers.size() ? mTrackers[index] : NULL;
}

sgct::SGCTTracker * sgct::SGCTTrackingManager::getTrackerPtr(const char * name)
{
	for(size_t i=0; i<mTrackers.size(); i++)
	{
		if( strcmp(name, mTrackers[i]->getName().c_str()) == 0 )
			return mTrackers[i];
	}

	MessageHandler::instance()->print(MessageHandler::NOTIFY_ERROR, "SGCTTrackingManager: Tracker '%s' not found!\n", name);

	//if not found
	return NULL;
}

void sgct::SGCTTrackingManager::setEnabled(bool state)
{
	for(size_t i=0; i<mTrackers.size(); i++)
	{
		mTrackers[i]->setEnabled( state );
	}
}

void sgct::SGCTTrackingManager::setSamplingTime(double t)
{
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Set sampling time for vrpn loop...\n");
#endif
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		mSamplingTime = t;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );
}

double sgct::SGCTTrackingManager::getSamplingTime()
{
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Get sampling time for vrpn loop...\n");
#endif
	double tmpVal;
	SGCTMutexManager::instance()->lockMutex( SGCTMutexManager::TrackingMutex );
		tmpVal = mSamplingTime;
	SGCTMutexManager::instance()->unlockMutex( SGCTMutexManager::TrackingMutex );

	return tmpVal;
}

void VRPN_CALLBACK update_tracker_cb(void *userdata, const vrpn_TRACKERCB info)
{
    sgct::SGCTTracker * trackerPtr =
		reinterpret_cast<sgct::SGCTTracker *>(userdata);
	if( trackerPtr == NULL )
		return;

	sgct::SGCTTrackingDevice * devicePtr = trackerPtr->getDevicePtrBySensorId( info.sensor );

	if(devicePtr == NULL)
		return;

#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Updating tracker...\n");
#endif
	sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::TrackingMutex );

	glm::dvec3 posVec = glm::dvec3( info.pos[0], info.pos[1], info.pos[2] );
	posVec *= trackerPtr->getScale();

	glm::dmat4 transMat = glm::translate( glm::dmat4(1.0), posVec );
	glm::dmat4 rotMat = glm::mat4_cast( glm::dquat( info.quat[3], info.quat[0], info.quat[1], info.quat[2] ) );

    devicePtr->setSensorTransform( transMat * rotMat );

    sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::TrackingMutex );
}

void VRPN_CALLBACK update_button_cb(void *userdata, const vrpn_BUTTONCB b )
{
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Update button value...\n");
#endif
	sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::TrackingMutex );

	sgct::SGCTTrackingDevice * devicePtr =
		reinterpret_cast<sgct::SGCTTrackingDevice *>(userdata);

	//fprintf(stderr, "Button: %d, state: %d\n", b.button, b.state);

	b.state == 0 ?
		devicePtr->setButtonVal( false, b.button) :
		devicePtr->setButtonVal( true, b.button);

    sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::TrackingMutex );
}

void VRPN_CALLBACK update_analog_cb(void* userdata, const vrpn_ANALOGCB a )
{
#ifdef __SGCT_TRACKING_MUTEX_DEBUG__
    fprintf(stderr, "Update analog values...\n");
#endif
	sgct::SGCTMutexManager::instance()->lockMutex( sgct::SGCTMutexManager::TrackingMutex );

	sgct::SGCTTrackingDevice * tdPtr =
		reinterpret_cast<sgct::SGCTTrackingDevice *>(userdata);

	tdPtr->setAnalogVal(a.channel, static_cast<size_t>(a.num_channel));

	sgct::SGCTMutexManager::instance()->unlockMutex( sgct::SGCTMutexManager::TrackingMutex );
}
