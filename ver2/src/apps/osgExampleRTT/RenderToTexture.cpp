#include "RenderToTexture.h"

/*!
	Default constructor.
*/
RenderToTexture::RenderToTexture()
{
	enabled = false;
}

/*!
	This function creates and render a local root (sub tree) to a textured quad.
	Requirements for this is that your graphic board can handle non power of two textures and support FBOs (frame buffer objects).
	\n\n
	The framebuffer is rendered to texture unit 0.\n
	
	@param	localRoot	Pointer to the sub tree which will be rendered.
	@param	ss			stateset to attach the texture to
	@param	width		texture width
	@param	height		texture height

*/
osg::Camera * RenderToTexture::createOrGetOffScreenRenderer(osg::Node * localRoot, osg::StateSet * ss, int width, int height)
{
	if( !camera.valid() ) //if not created then create
	{
		tFrameBuffer = 0;

		osg::Texture2D* tex = new osg::Texture2D;
		tex->setTextureSize(width, height);
		tex->setInternalFormat(GL_RGB);
		tex->setMaxAnisotropy(8.0f);
		tex->setResizeNonPowerOfTwoHint(false);
		tex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::LINEAR);
		tex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::LINEAR);
		tex->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE); 
		tex->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
		tFrameBuffer = tex;
		tFrameBuffer->setDataVariance(osg::Object::DYNAMIC);

		/*
		tDepthBuffer = 0;
		osg::Texture2D* ztex = new osg::Texture2D;
		ztex->setTextureSize(static_cast<int>(width), static_cast<int>(height));
		ztex->setInternalFormat(GL_DEPTH_COMPONENT);
		ztex->setFilter(osg::Texture2D::MIN_FILTER,osg::Texture2D::NEAREST);
		ztex->setFilter(osg::Texture2D::MAG_FILTER,osg::Texture2D::NEAREST);
		ztex->setResizeNonPowerOfTwoHint(false);
		tDepthBuffer = ztex;
		tDepthBuffer->setDataVariance(osg::Object::DYNAMIC);*/

		stateset = ss;
		stateset->setTextureAttributeAndModes(0,tFrameBuffer,osg::StateAttribute::ON);

		// then create the camera node to do the render to texture
		camera = new osg::Camera;

		// set up the background color and clear mask.
		camera->setClearColor(osg::Vec4(0.0f, 0.0f, 0.2f, 1.0f));
		camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//camera->setProjectionMatrix( osg::Matrix( glm::value_ptr( sgct::Engine::getPtr()->getActiveProjectionMatrix() ) ) );
		//camera->setViewMatrix(		 osg::Matrix( glm::value_ptr( sgct::Engine::getPtr()->getSceneTransform()         ) ) );
		camera->setProjectionMatrixAsPerspective(80.0, 1.0, 0.1, 100.0);
		camera->setViewMatrixAsLookAt( osg::Vec3d(0.0, 0.0, 1.0),
			osg::Vec3d(0.0, 0.0, 0.0),
			osg::Vec3d(0.0, 1.0, 0.0));
		camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
		/*
		const int * curr_vp = sgct::Engine::getPtr()->getActiveViewport();
		camera->setViewport( curr_vp[0],
			curr_vp[1],
			curr_vp[2],
			curr_vp[3]);*/

		camera->setViewport( 0, 0,
			static_cast<int>(width),
			static_cast<int>(height));
	
		// set the camera to render before the main camera.
		camera->setRenderOrder(osg::Camera::PRE_RENDER);

		// tell the camera to use OpenGL frame buffer object where supported.
		camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);

		// attach the texture and use it as the color buffer.
		camera->attach(osg::Camera::COLOR_BUFFER, tFrameBuffer, 
							   0, 0, false,
							   0, 0);

		/*camera->attach(osg::Camera::DEPTH_BUFFER, tDepthBuffer, 
							   0, 0, false,
							   0, 0);*/

		camera->addChild(localRoot);

		enabled = true;
	}

    return camera.get();
}