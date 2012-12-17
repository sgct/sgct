#ifndef _RenderToTexture_
#define _RenderToTexture_

#include "sgct.h"
#include <osg/Node>
#include <osg/Camera>

/*!
	Render whole scene to a texture.
*/
class RenderToTexture
{
public:
	RenderToTexture();
	osg::Camera * createOrGetOffScreenRenderer(osg::Node * localRoot, osg::StateSet * ss, int width, int height);

	/*!
		Get the camera.
	*/
	inline osg::Camera * getCamera(){ return camera.get(); }
	
	/*!
		Indicates if post processing is active.
	*/
	inline bool & isEnabled() { return enabled; }

private:
	osg::ref_ptr<osg::Camera> camera;
	osg::Texture* tFrameBuffer;
	//osg::Texture* tDepthBuffer;
	osg::ref_ptr<osg::StateSet> stateset;
	bool enabled;
};

#endif