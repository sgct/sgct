#ifndef _RenderToTexture_
#define _RenderToTexture_

#include "sgct.h"
#include <osg/Node>
#include <osg/Camera>
#include <osg/Texture2D>

struct preDrawCB : public osg::Camera::DrawCallback
{
	preDrawCB() {;}
	
	void operator()(const osg::Camera&) const{;}
	void operator()(osg::RenderInfo & renderInfo) const
	{
		//bind SGCT FBO just before main rendering
		//OSG renders a full-screen quad to the SGCT main FBO
		sgct::Engine::instance()->getActiveWindowPtr()->getFBOPtr()->bind();
	}
};

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
	osg::ref_ptr<osg::Texture2D> tFrameBuffer;
	//osg::ref_ptr<osg::Texture2D> tDepthBuffer;
	osg::ref_ptr<osg::StateSet> stateset;
	bool enabled;
};

#endif