#include "sgct.h"
#include "Animation.h"
#include <osgAnimation/UpdateMatrixTransform>
#include <osgAnimation/StackedTranslateElement>
#include <osgAnimation/StackedQuaternionElement>
#include <iostream>
#include <fstream>

AnimationData::AnimationData()
{
	mSpeed = 0.5; //meter per second
}

bool AnimationData::save(std::string filename)
{
	std::ofstream outFile(filename.c_str());

	if (outFile.is_open())
	{
		outFile.precision(9);
		
		for(size_t i=0; i<mPositions.size(); i++)
		{
			outFile << mPositions[i].x() << "\t";
			outFile << mPositions[i].y() << "\t";
			outFile << mPositions[i].z() << "\t";

			outFile << mQuats[i].w() << "\t";
			outFile << mQuats[i].x() << "\t";
			outFile << mQuats[i].y() << "\t";
			outFile << mQuats[i].z() << "\n";
		}

		outFile.close();

		sgct::MessageHandler::instance()->print("Animation path '%s' saved successfully.\n", filename.c_str());
		return true;
	}
	else
	{
		sgct::MessageHandler::instance()->print("Failed to save animation path '%s'!\n", filename.c_str());
		return false;
	}
}

bool AnimationData::read(std::string filename)
{
	std::ifstream inFile(filename.c_str());
	if (inFile.is_open())
	{
		mPositions.clear();
		mQuats.clear();
		
		size_t i = 0;

		osg::Vec3 tmpVec;
		osg::Quat tmpQuat;

		while ( inFile.good() )
		{
			inFile >> tmpVec[0];
			inFile >> tmpVec[1];
			inFile >> tmpVec[2];

			mPositions.push_back( tmpVec );

			inFile >> tmpQuat[3];
			inFile >> tmpQuat[0];
			inFile >> tmpQuat[1];
			inFile >> tmpQuat[2];

			mQuats.push_back( tmpQuat );

			i++;
		}

		inFile.close();

		sgct::MessageHandler::instance()->print("Animation path '%s' opened successfully. %d samples read.\n", filename.c_str(), i);
		return true;
	}
	else
	{
		sgct::MessageHandler::instance()->print("Failed to open animation path '%s'!\n", filename.c_str());
		return false;
	}
}

osg::Group * AnimationData::getOrCreateVisualRepresentation()
{
	if(!mGroup.valid())
	{
		mGroup = new osg::Group;
	}

	return mGroup.get();
}

void AnimationData::updateVisualRepresentation()
{

}

Animation::Animation(bool cubic)
{
	mIsSet = false;
	mUseBezier = cubic;
}

osg::MatrixTransform * Animation::getOrCreateAnimation()
{	
	if( !mTransform.valid() )
	{
		mTransform = new osg::MatrixTransform();
		mTransform->setName("AnimatedNode");
		mTransform->setDataVariance(osg::Object::DYNAMIC); //enable update during travelsal
		
		if( mUseBezier )
		{
			mCh1_cubic = new osgAnimation::Vec3CubicBezierChannel;
			mCh1_cubic->setName("position");
			mCh1_cubic->setTargetName("AnimatedCallback");
			mCh1Container_cubic = mCh1_cubic->getOrCreateSampler()->getOrCreateKeyframeContainer();
			mCh1Container_cubic->push_back(osgAnimation::Vec3CubicBezierKeyframe(0.0, osgAnimation::Vec3CubicBezier(
				osg::Vec3(0.0f, 0.0f, 0.0f), osg::Vec3(0.0f, 0.0f, 0.0f), osg::Vec3(0.0f, 0.0f, 0.0f))) );
			/*
			mCh1Container_cubic->push_back(osgAnimation::Vec3CubicBezierKeyframe(0, osgAnimation::Vec3CubicBezier(
                                                        v[0],
                                                        v[0],
                                                        v[0] + (v[1] - v[0])/3.0f
                                                        )));

			mCh1Container_cubic->push_back(osgAnimation::Vec3CubicBezierKeyframe(2, osgAnimation::Vec3CubicBezier(
                                                        v[1] - (v[1] - v[0])/3.0f,
                                                        v[1],
                                                        v[1] + (v[2] - v[1])/3.0f
                                                        )));

			mCh1Container_cubic->push_back(osgAnimation::Vec3CubicBezierKeyframe(4, osgAnimation::Vec3CubicBezier(
                                                        v[2] - (v[2] - v[1])/3.0f,
                                                        v[2],
                                                        v[2] + (v[3] - v[2])/3.0f
                                                        )));

			mCh1Container_cubic->push_back(osgAnimation::Vec3CubicBezierKeyframe(8, osgAnimation::Vec3CubicBezier(
                                                        v[3] - (v[3] - v[2])/3.0f,
                                                        v[3],
                                                        v[3] + (v[4] - v[3])/3.0f
                                                        )));

			mCh1Container_cubic->push_back(osgAnimation::Vec3CubicBezierKeyframe(12, osgAnimation::Vec3CubicBezier(
                                                        v[4] - (v[4] - v[3])/3.0f,
                                                        v[4],
                                                        v[4]
                                                        )));
			*/
		}
		else
		{
			mCh1 = new osgAnimation::Vec3LinearChannel;
			mCh1->setName("position");
			mCh1->setTargetName("AnimatedCallback");
			mCh1Container = mCh1->getOrCreateSampler()->getOrCreateKeyframeContainer();
			mCh1Container->push_back(osgAnimation::Vec3Keyframe(0.0, osg::Vec3(0.0f, 0.0f, 0.0f)) );
		}


		mCh2 = new osgAnimation::QuatSphericalLinearChannel;
		mCh2->setName("quat");
		mCh2->setTargetName("AnimatedCallback");
		mCh2Container = mCh2->getOrCreateSampler()->getOrCreateKeyframeContainer();
		mCh2Container->push_back(osgAnimation::QuatKeyframe(0.0, osg::Quat(0.0f, 0.0f, 0.0f, 0.0f)) );
		
		osgAnimation::UpdateMatrixTransform* updatecb = new osgAnimation::UpdateMatrixTransform("AnimatedCallback");
		updatecb->getStackedTransforms().push_back(new osgAnimation::StackedQuaternionElement("quat"));
		updatecb->getStackedTransforms().push_back(new osgAnimation::StackedTranslateElement("position"));
		
		mAnimation = new osgAnimation::Animation;
		mAnimation->setPlayMode( osgAnimation::Animation::ONCE );
		
		if( mUseBezier )
		{
			mAnimation->addChannel( mCh1_cubic.get() );
		}
		else
		{
			mAnimation->addChannel( mCh1.get() );
		}
		mAnimation->addChannel( mCh2.get() );

		mMgr = new osgAnimation::BasicAnimationManager;
		mMgr->registerAnimation( mAnimation.get() );

		//clear temp values
		if( mUseBezier )
		{
			mCh1Container_cubic->clear();
		}
		else
		{			
			mCh1Container->clear();
		}	
		mCh2Container->clear();
		
		//connect the UpdateMatrixTransform callback to the MatrixTRanform
		mTransform->setUpdateCallback(updatecb);
    
		//initialize MatrixTranform
		mTransform->setMatrix(osg::Matrix::identity());
		mIsSet = true;

		sgct::MessageHandler::instance()->print("Initiated animation.\n");
	}

	return mTransform.get();
}

osgAnimation::BasicAnimationManager * Animation::getManager()
{
	return mMgr.get();
}

void Animation::play()
{
	if( mIsSet )
	{
		mAnimation->computeDuration();
		sgct::MessageHandler::instance()->print("Staring playback of animation (duration: %lf)\n", mAnimation->getDuration() );
		mMgr->playAnimation( mAnimation.get() );
	}
}

void Animation::stop()
{
	if( mIsSet )
	{
		mMgr->stopAnimation( mAnimation.get() );
	}
}

void Animation::addPositionKeyFrame(double time, osg::Vec3 pos)
{
	if( mIsSet )
	{
		if(mUseBezier)
		{
			mCh1Container_cubic->push_back(
			osgAnimation::Vec3CubicBezierKeyframe(time, 
				osgAnimation::Vec3CubicBezier(pos, pos, pos)) );	
		}
		else
		{
			mCh1Container->push_back(
			osgAnimation::Vec3Keyframe(time, pos) );
		}
	}
}

void Animation::addCubicPositionKeyFrame(double time, osg::Vec3 p1, osg::Vec3 p2, osg::Vec3 p3)
{
	if( mIsSet )
	{
		if(mUseBezier)
		{
			mCh1Container_cubic->push_back(
			osgAnimation::Vec3CubicBezierKeyframe(time, 
				osgAnimation::Vec3CubicBezier(p1, p2, p3)) );	
		}
		else
		{
			mCh1Container->push_back(
			osgAnimation::Vec3Keyframe(time, p1) );
		}
	}
}

void Animation::addQuatKeyFrame(double time, osg::Quat quat)
{
	if( mIsSet )
	{
		mCh2Container->push_back(
			osgAnimation::QuatKeyframe(time, quat) );
	}
}