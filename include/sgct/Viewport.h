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
	void setMaskTexture(const char * texturePath);
	void setCorrectionMesh(const char * meshPath);
	void setTracked(bool state);
	void loadData();

	void renderMesh(CorrectionMesh::MeshType mt);

	inline bool hasOverlayTexture() { return mOverlayTextureIndex != GL_FALSE; }
	inline bool hasMaskTexture() { return mMaskTextureIndex != GL_FALSE; }
	inline bool hasSubViewports() { return mNonLinearProjection != NULL; }

	inline const bool & hasCorrectionMesh() { return mCorrectionMesh; }
	inline const bool & isTracked() { return mTracked; }
	inline const unsigned int & getOverlayTextureIndex() { return mOverlayTextureIndex; }
	inline const unsigned int & getMaskTextureIndex() { return mMaskTextureIndex; }
	inline CorrectionMesh * getCorrectionMeshPtr() { return &mCM; }
	inline NonLinearProjection * getNonLinearProjectionPtr() { return mNonLinearProjection; }

private:
    void reset(float x, float y, float xSize, float ySize);
	void parsePlanarProjection(tinyxml2::XMLElement * element);
	void parseFisheyeProjection(tinyxml2::XMLElement * element);
    
private:
	CorrectionMesh mCM;
	std::string mOverlayFilename;
	std::string mMaskFilename;
	std::string mMeshFilename;
	bool mCorrectionMesh;
	bool mTracked;
	unsigned int mOverlayTextureIndex;
	unsigned int mMaskTextureIndex;

	NonLinearProjection * mNonLinearProjection;
};

}

#endif
