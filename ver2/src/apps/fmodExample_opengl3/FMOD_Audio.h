#include "sgct.h"
#include "fmod.hpp"
#include <vector>
#include <string>

#ifndef _FMOD_Audio_
#define _FMOD_Audio_

class SoundItem
{
public:
	SoundItem();
	FMOD::Sound     *mSound;
    FMOD::Channel   *mChannel;
	std::string		mName;
};

class FMOD_Audio
{
public:
	FMOD_Audio();
	~FMOD_Audio();
	void init();
	void update();
	bool addSound(const std::string & name, const std::string & path, bool loop, bool stream = true);
	void setSoundNearFarLimits(const std::string & name, float nearLimit, float farLimit);
	void setSoundNearFarLimits(std::size_t index, float nearLimit, float farLimit);
	void setLoopMode(const std::string & name, bool loop);
	void setLoopMode(std::size_t index, bool loop);
	void playSound(const std::string & name);
	void playSound(std::size_t index);
	void setSoundPositionAndVelocity( const std::string & name, glm::vec3 pos, glm::vec3 vel );
	void setSoundPositionAndVelocity( std::size_t index, glm::vec3 pos, glm::vec3 vel );
	inline std::size_t getNumberOfSounds() { return mSoundItems.size(); }
	inline SoundItem * getSoundItemAtIndex( std::size_t index ) { return &mSoundItems[index]; }
	
private:
	SoundItem * findSoundItem(const std::string & name);

private:
	FMOD::System * mSystem;
	std::vector<SoundItem> mSoundItems;
};

#endif