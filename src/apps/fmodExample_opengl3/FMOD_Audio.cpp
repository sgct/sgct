#include "fmod_errors.h"
#include "FMOD_Audio.h"

#define USE_FMOD_EX 1
#define USE_FMOD_HARDWARE 0

SoundItem::SoundItem()
{
	mSound = NULL;
	mChannel = NULL;
}

FMOD_Audio::FMOD_Audio()
{
	mSystem = NULL;
}

FMOD_Audio::~FMOD_Audio()
{
	if( mSystem != NULL )
	{
		FMOD_RESULT result;

		/*
			Shut down
		*/
		for(std::size_t i=0; i<mSoundItems.size(); i++)
		{
			result = mSoundItems[i].mSound->release();
			if (result != FMOD_OK)
			{
				sgct::MessageHandler::instance()->print("Failed to release sound '%s', FMOD error %d - %s\n",
					mSoundItems[i].mName.c_str(), result, FMOD_ErrorString(result));
			}
		}

		result = mSystem->close();
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("FMOD error %d - %s\n", result, FMOD_ErrorString(result));
		}

		result = mSystem->release();
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("FMOD error %d - %s\n", result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::update()
{
	if( mSystem != NULL )
	{
		FMOD_RESULT result = mSystem->update();
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("FMOD update error %d - %s\n", result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::init()
{
	FMOD_RESULT result;
	unsigned int version;
	
	/*
        Create a System object and initialize.
    */
    result = FMOD::System_Create(&mSystem);
    if (result != FMOD_OK)
    {
        sgct::MessageHandler::instance()->print("FMOD error %d - %s\n", result, FMOD_ErrorString(result));
    }
    
    result = mSystem->getVersion(&version);
    if (result != FMOD_OK)
    {
        sgct::MessageHandler::instance()->print("FMOD error %d - %s\n", result, FMOD_ErrorString(result));
    }

    if (version < FMOD_VERSION)
    {
        sgct::MessageHandler::instance()->print("FMOD lib version %08x doesn't match header version %08x\n", version, FMOD_VERSION);
    }

#if USE_FMOD_EX
    result = mSystem->init(100, FMOD_INIT_NORMAL, NULL);
#else
	result = mSystem->init(100, FMOD_INIT_NORMAL | FMOD_INIT_PROFILE_ENABLE, NULL);
#endif
    if (result != FMOD_OK)
    {
        sgct::MessageHandler::instance()->print("FMOD error %d - %s\n", result, FMOD_ErrorString(result));
    }
    
	//distance factor set in meters
	result = mSystem->set3DSettings(1.0f, 1.0f, 1.0f);
    if (result != FMOD_OK)
    {
        sgct::MessageHandler::instance()->print("FMOD error %d - %s\n", result, FMOD_ErrorString(result));
    }
}

bool FMOD_Audio::addSound(const std::string & name, const std::string & path, bool loop, bool stream)
{
	sgct::MessageHandler::instance()->print("Adding sound '%s'...\n", name.c_str());
	
	FMOD_RESULT result;

	SoundItem si;
	si.mName.assign( name );

#if USE_FMOD_HARDWARE
	result = stream ?
		mSystem->createStream( path.c_str(), FMOD_3D, 0, &(si.mSound) ):
		mSystem->createSound( path.c_str(), FMOD_3D, 0, &(si.mSound) );
#else
	result = stream ?
		mSystem->createStream(path.c_str(), FMOD_SOFTWARE | FMOD_3D, 0, &(si.mSound)) :
		mSystem->createSound(path.c_str(), FMOD_SOFTWARE | FMOD_3D, 0, &(si.mSound));
#endif
    
	if (result != FMOD_OK)
    {
        sgct::MessageHandler::instance()->print("Failed to load file '%s', FMOD error %d - %s\n",
			path.c_str(), result, FMOD_ErrorString(result));

		return false;
    }

	result = si.mSound->setMode( loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
	if (result != FMOD_OK)
	{
		sgct::MessageHandler::instance()->print("Failed to set loop mode, FMOD error %d - %s\n", result, FMOD_ErrorString(result));
	}

	//sgct::MessageHandler::instance()->print("Creating channel...\n");

	//add a channel to the sound (paused)
#if USE_FMOD_EX
	result = mSystem->playSound(FMOD_CHANNEL_FREE, si.mSound, true, &(si.mChannel));
#else
	result = mSystem->playSound(si.mSound, 0, true, &(si.mChannel));
#endif
	if (result != FMOD_OK)
	{
		sgct::MessageHandler::instance()->print("Failed to make sound '%s' playable, FMOD error %d - %s\n",
			si.mName.c_str(), result, FMOD_ErrorString(result));
		
		si.mSound->release();
		return false;
	}

	mSoundItems.push_back( si );
	//sgct::MessageHandler::instance()->print("Done.\n");
	return true;
}

//set near & far in meters
void FMOD_Audio::setSoundNearFarLimits(const std::string & name, float nearLimit, float farLimit)
{
	SoundItem * siPtr = findSoundItem( name );
	if( siPtr != NULL )
	{
		FMOD_RESULT result;
		result = siPtr->mSound->set3DMinMaxDistance(nearLimit, farLimit);
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("Failed to set near & far limits, FMOD error %d - %s\n", result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::setSoundNearFarLimits(std::size_t index, float nearLimit, float farLimit)
{
	if( index < mSoundItems.size() )
	{
		FMOD_RESULT result;
		result = mSoundItems[index].mSound->set3DMinMaxDistance(nearLimit, farLimit);

		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("Failed to set near & far limits, FMOD error %d - %s\n", result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::setLoopMode(const std::string & name, bool loop)
{
	SoundItem * siPtr = findSoundItem( name );
	if( siPtr != NULL )
	{
		FMOD_RESULT result;
		result = siPtr->mSound->setMode( loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("Failed to set loop mode, FMOD error %d - %s\n", result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::setLoopMode(std::size_t index, bool loop)
{
	if( index < mSoundItems.size() )
	{
		FMOD_RESULT result;
		result = mSoundItems[index].mSound->setMode( loop ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);

		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("Failed to set loop mode, FMOD error %d - %s\n", result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::playSound(const std::string & name)
{
	SoundItem * siPtr = findSoundItem( name );
	if( siPtr != NULL && siPtr->mChannel != NULL )
	{
		FMOD_RESULT result;
		result = siPtr->mChannel->setPaused(false);
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("Failed to play sound '%s', FMOD error %d - %s\n",
				siPtr->mName.c_str(), result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::playSound(std::size_t index)
{
	if( index < mSoundItems.size() && mSoundItems[index].mChannel != NULL)
	{
		FMOD_RESULT result;
		result = mSoundItems[index].mChannel->setPaused(false);
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("Failed to play sound '%s', FMOD error %d - %s\n",
				mSoundItems[index].mName.c_str(), result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::setSoundPositionAndVelocity( const std::string & name, glm::vec3 pos, glm::vec3 vel )
{
	SoundItem * siPtr = findSoundItem( name );
	if( siPtr != NULL && siPtr->mChannel != NULL )
	{
		FMOD_RESULT result;
		FMOD_VECTOR _pos = { pos.x, pos.y, pos.z };
		FMOD_VECTOR _vel = { vel.x, vel.y, vel.z };

		result = siPtr->mChannel->set3DAttributes(&_pos, &_vel);
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("Failed to set 3D Attributes for sound '%s', FMOD error %d - %s\n",
				siPtr->mName.c_str(), result, FMOD_ErrorString(result));
		}
	}
}

void FMOD_Audio::setSoundPositionAndVelocity( std::size_t index, glm::vec3 pos, glm::vec3 vel )
{
	if( index < mSoundItems.size() && mSoundItems[index].mChannel != NULL )
	{
		FMOD_RESULT result;
		FMOD_VECTOR _pos = { pos.x, pos.y, pos.z };
		FMOD_VECTOR _vel = { vel.x, vel.y, vel.z };

		result = mSoundItems[index].mChannel->set3DAttributes(&_pos, &_vel);
		if (result != FMOD_OK)
		{
			sgct::MessageHandler::instance()->print("Failed to set 3D Attributes for sound '%s', FMOD error %d - %s\n",
				mSoundItems[index].mName.c_str(), result, FMOD_ErrorString(result));
		}
	}
}

SoundItem * FMOD_Audio::findSoundItem(const std::string & name)
{
	for(std::size_t i=0; i<mSoundItems.size(); i++)
	{
		if( mSoundItems[i].mName.compare(name) == 0 )
			return &mSoundItems[i];
	}

	sgct::MessageHandler::instance()->print("Sound '%s' was not found!\n", name.c_str() );

	//if not found return null
	return NULL;
}

