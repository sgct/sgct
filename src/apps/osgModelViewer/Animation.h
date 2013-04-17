#ifndef _ANIMATION_
#define _ANIMATION_

#include <osg/MatrixTransform>
#include <osgAnimation/Channel>
#include <osgAnimation/Animation>
#include <osgAnimation/BasicAnimationManager>

class AnimationData
{
public:
	AnimationData();
	bool save(std::string filename);
	bool read(std::string filename);
	osg::Group * getOrCreateVisualRepresentation();
	void updateVisualRepresentation();

	std::vector< osg::Vec3 > mPositions;
	std::vector< osg::Quat > mQuats;
	double mSpeed;

private:
	osg::ref_ptr<osg::Group> mGroup;
};

class Animation
{
public:
	Animation(bool cubic);
	osg::MatrixTransform * getOrCreateAnimation();
	osgAnimation::BasicAnimationManager * getManager();
	void play();
	void stop();
	void addPositionKeyFrame(double time, osg::Vec3 pos);
	void addCubicPositionKeyFrame(double time, osg::Vec3 p1, osg::Vec3 p2, osg::Vec3 p3);
	void addQuatKeyFrame(double time, osg::Quat quat);
	bool isCubic() { return mUseBezier; } 
	double isPlaying() { return mMgr->isPlaying( mAnimation ); }
	double getDuration() { return mAnimation->getDuration(); }

private:
	bool mIsSet;
	bool mUseBezier;
	osg::ref_ptr<osg::MatrixTransform> mTransform;
	osg::ref_ptr<osgAnimation::Animation> mAnimation;

	osg::ref_ptr<osgAnimation::Vec3LinearChannel> mCh1;
	osg::ref_ptr<osgAnimation::Vec3CubicBezierChannel> mCh1_cubic;
	osg::ref_ptr<osgAnimation::QuatSphericalLinearChannel> mCh2;

	osg::ref_ptr<osgAnimation::Vec3KeyframeContainer> mCh1Container;
	osg::ref_ptr<osgAnimation::Vec3CubicBezierKeyframeContainer> mCh1Container_cubic;
	osg::ref_ptr<osgAnimation::QuatKeyframeContainer> mCh2Container;

	osg::ref_ptr<osgAnimation::BasicAnimationManager> mMgr;
};

#endif