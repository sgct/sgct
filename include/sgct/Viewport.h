/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include "BaseViewport.h"
#include "NonLinearProjection.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <string>
#include "CorrectionMesh.h"
#include <stddef.h> //get definition for NULL

namespace sgct_core
{

/*!
	This class holds and manages viewportdata and calculates frustums
*/
class Viewport : public BaseViewport
{
public:
	Viewport();
	Viewport(float x, float y, float xSize, float ySize);
	~Viewport();

	void configure(tinyxml2::XMLElement * element);
	void setOverlayTexture(const char * texturePath);
	void setBlendMaskTexture(const char * texturePath);
	void setBlackLevelMaskTexture(const char * texturePath);
	void setCorrectionMesh(const char * meshPath);
	void setTracked(bool state);
	void loadData();

	void renderMesh(CorrectionMesh::MeshType mt);

	inline bool hasOverlayTexture() { return mOverlayTextureIndex != GL_FALSE; }
	inline bool hasBlendMaskTexture() { return mBlendMaskTextureIndex != GL_FALSE; }
	inline bool hasBlackLevelMaskTexture() { return mBlackLevelMaskTextureIndex != GL_FALSE; }
	inline bool hasSubViewports() { return mNonLinearProjection != NULL; }

	inline const bool & hasCorrectionMesh() { return mCorrectionMesh; }
	inline const bool & isTracked() { return mTracked; }
	inline const unsigned int & getOverlayTextureIndex() { return mOverlayTextureIndex; }
	inline const unsigned int & getBlendMaskTextureIndex() { return mBlendMaskTextureIndex; }
	inline const unsigned int & getBlackLevelMaskTextureIndex() { return mBlackLevelMaskTextureIndex; }
	inline CorrectionMesh * getCorrectionMeshPtr() { return &mCM; }
	inline NonLinearProjection * getNonLinearProjectionPtr() { return mNonLinearProjection; }

private:
    void reset(float x, float y, float xSize, float ySize);
	void parsePlanarProjection(tinyxml2::XMLElement * element);
	void parseFisheyeProjection(tinyxml2::XMLElement * element);
    
private:
	CorrectionMesh mCM;
	std::string mOverlayFilename;
	std::string mBlendMaskFilename;
	std::string mBlackLevelMaskFilename;
	std::string mMeshFilename;
	std::string mMeshHint;
	bool mCorrectionMesh;
	bool mTracked;
	unsigned int mOverlayTextureIndex;
	unsigned int mBlendMaskTextureIndex;
	unsigned int mBlackLevelMaskTextureIndex;

	NonLinearProjection * mNonLinearProjection;
};

}

#endif
