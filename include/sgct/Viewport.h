/*************************************************************************
Copyright (c) 2012 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h 
*************************************************************************/

#ifndef _VIEWPORT_H
#define _VIEWPORT_H

#include <glm/glm.hpp>
#include "Frustum.h"
#include "CorrectionMesh.h"
#include <stddef.h> //get definition for NULL

namespace sgct_core
{

/*!
	This class holds and manages viewportdata and calculates frustums
*/
class Viewport
{
public:
	enum ViewPlaneCorner { LowerLeft = 0, UpperLeft, UpperRight };

	Viewport();
	Viewport(double x, double y, double xSize, double ySize);

	void set(double x, double y, double xSize, double ySize);
	void setPos(double x, double y);
	void setSize(double x, double y);
	void setEye(Frustum::FrustumMode eye);
	void setOverlayTexture(const char * texturePath);
	void setCorrectionMesh(const char * meshPath);
	void setTracked(bool state);
	void setEnabled(bool state);
	void loadData();
	void calculateFrustum(const sgct_core::Frustum::FrustumMode &frustumMode, glm::vec3 * eyePos, float near, float far);
	void calculateFisheyeFrustum(const sgct_core::Frustum::FrustumMode &frustumMode, glm::vec3 * eyePos, glm::vec3 * offset, float near, float far);
	void setViewPlaneCoords(const unsigned int cornerIndex, glm::vec3 cornerPos);
	void setViewPlaneCoords(const unsigned int cornerIndex, glm::vec4 cornerPos);
	void renderMesh();

	/*!
		\returns the normalized x viewport coordinate
	*/
	inline double getX() { return mX; }
	/*!
		\returns the normalized y viewport coordinate
	*/
	inline double getY() { return mY; }
	inline double getXSize() { return mXSize; }
	inline double getYSize() { return mYSize; }
	inline Frustum::FrustumMode getEye() { return mEye; }
	inline Frustum * getFrustum(sgct_core::Frustum::FrustumMode frustumMode) { return &mFrustums[frustumMode]; }
	inline Frustum * getFrustum() { return &mFrustums[mEye]; }
	inline const glm::mat4 & getViewProjectionMatrix( sgct_core::Frustum::FrustumMode frustumMode ) { return mViewProjectionMatrix[frustumMode]; }
	inline const glm::mat4 & getViewMatrix( sgct_core::Frustum::FrustumMode frustumMode ) { return mViewMatrix[frustumMode]; }
	inline const glm::mat4 & getProjectionMatrix( sgct_core::Frustum::FrustumMode frustumMode ) { return mProjectionMatrix[frustumMode]; }
	inline const glm::vec3 getViewPlaneCoords( ViewPlaneCorner vpc ) { return mViewPlaneCoords[ vpc ]; }
	inline bool hasOverlayTexture() { return mOverlayTexture; }
	inline bool hasCorrectionMesh() { return mCorrectionMesh; }
	inline bool isTracked() { return mTracked; }
	inline bool isEnabled() { return mEnabled; }
	inline std::size_t getOverlayTextureIndex() { return mTextureIndex; }
	inline CorrectionMesh * getCorrectionMeshPtr() { return &mCM; }

private:
	glm::vec3 mViewPlaneCoords[3];
	glm::mat4 mViewMatrix[3];
	glm::mat4 mViewProjectionMatrix[3];
	glm::mat4 mProjectionMatrix[3];

	double mX;
	double mY;
	double mXSize;
	double mYSize;
	Frustum mFrustums[3];
	Frustum::FrustumMode mEye;
	CorrectionMesh mCM;
	char * mOverlayFilename;
	char * mMeshFilename;
	bool mOverlayTexture;
	bool mCorrectionMesh;
	bool mTracked;
	bool mEnabled;
	std::size_t mTextureIndex;
};

}

#endif
