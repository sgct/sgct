#include "Dome.h"
#include <fstream>
#include <iostream>
#include <iomanip>

#define EXPORT_DOME_MODEL 0

Dome::Dome(float radius, float tilt)
{
	mRadius = radius;
	mTilt = tilt;
	mTiltOffset = 0.0f;

	mGeoDisplayList = GL_FALSE;
	mBlendZonesDisplayList = GL_FALSE;
	mChannelZonesDisplayList = GL_FALSE;
	mTexDisplayList = GL_FALSE;
}

Dome::~Dome()
{
	if( mGeoDisplayList )
		glDeleteLists(mGeoDisplayList, 4);
}

void Dome::drawGeoCorrPattern()
{
	glCallList( mGeoDisplayList );
}

void Dome::drawChannelZones()
{
	glCallList( mChannelZonesDisplayList );
}

void Dome::drawBlendZones()
{
	glCallList( mBlendZonesDisplayList );
}

void Dome::drawTexturedSphere()
{
	/*if( projectionType == LATLONMAP )
	{
		glPushMatrix();
		glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
	}*/

	
	
	glPushMatrix();
		//if( projectionType == LATLONMAP )
		//	glRotatef(180.0f, 0.0f, 1.0f, 0.0f);

		glRotatef(-(mTilt+mTiltOffset), 1.0f, 0.0f, 0.0f);

		//glPushMatrix();
		//glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
		
		
		
		glCallList( mTexDisplayList );
	glPopMatrix();
	//glPopMatrix();

	/*if( projectionType == LATLONMAP )
		glPopMatrix();*/
}

void Dome::drawColCorrPattern(glm::vec3 * color, int mode)
{
	float x0, y0, z0;
	float x1, y1, z1;

	float elevation0, elevation1;
	float azimuth;
	float intensity0, intensity1;
	int e;

	const int elevationSteps = 32;
	const int azimuthSteps = 128;
	
	glPushMatrix();
	glRotatef(-mTilt, 1.0f, 0.0f, 0.0f);

	for(e=0; e<(elevationSteps-1); e++)
	{
		elevation0 = glm::radians( static_cast<float>(e * 90)/static_cast<float>(elevationSteps) );
		elevation1 = glm::radians( static_cast<float>((e+1) * 90)/static_cast<float>(elevationSteps) );
		
		switch(mode)
		{
		case STEP:
			intensity0 = 1.0f - static_cast<float>(e)/static_cast<float>(elevationSteps-1);
			intensity1 = intensity0;
			break;
		case STEP_INVERTED:
			intensity0 = static_cast<float>(e)/static_cast<float>(elevationSteps-1);
			intensity1 = intensity0;
			break;
		case GRADIENT:
			intensity0 = 1.0f - static_cast<float>(e)/static_cast<float>(elevationSteps-2);
			intensity1 = 1.0f - static_cast<float>(e+1)/static_cast<float>(elevationSteps-2);
			break;
		case GRADIENT_INVERTED:
			intensity0 = static_cast<float>(e)/static_cast<float>(elevationSteps-2);
			intensity1 = static_cast<float>(e+1)/static_cast<float>(elevationSteps-2);
			break;
		case SOLID:
			intensity0 = 1.0;
			intensity1 = intensity0;
			break;
		default:
			intensity0 = 1.0;
			intensity1 = intensity0;
			break;
		}

		glBegin( GL_TRIANGLE_STRIP );

		y0 = mRadius * sinf( elevation0 );
		y1 = mRadius * sinf( elevation1 );
			
		for(int a=0; a<=azimuthSteps; a++)
		{
			azimuth = glm::radians( static_cast<float>(a * 360)/static_cast<float>(azimuthSteps) );
			
			x0 = mRadius * cosf( elevation0 ) * sinf( azimuth );
			z0 = -mRadius * cosf( elevation0 ) * cosf( azimuth );

			x1 = mRadius * cosf( elevation1 ) * sinf( azimuth );
			z1 = -mRadius * cosf( elevation1 ) * cosf( azimuth );

			glColor3f( color->r * intensity0,  color->g * intensity0,  color->b * intensity0);
			glVertex3f( x0, y0, z0 );
			glColor3f(  color->r * intensity1,  color->g * intensity1,  color->b * intensity1);
			glVertex3f( x1, y1, z1 );
		}

		glEnd();
	}
	
	//CAP
	e = elevationSteps - 1;
	elevation0 = glm::radians( static_cast<float>(e * 90)/static_cast<float>(elevationSteps) );
	elevation1 = glm::radians( static_cast<float>((e+1) * 90)/static_cast<float>(elevationSteps) );

	glBegin( GL_TRIANGLE_FAN );

	y0 = mRadius * sinf( elevation0 );
	y1 = mRadius * sinf( elevation1 );

	glColor3f( color->r * intensity1, color->g * intensity1, color->b * intensity1);
	glVertex3f( 0.0f, y1, 0.0f );
		
	for(int a=0; a<=azimuthSteps; a++)
	{
		azimuth = glm::radians( static_cast<float>(a * 360)/static_cast<float>(azimuthSteps) );
		
		x0 = mRadius * cosf( elevation0 ) * sinf( azimuth );
		z0 = -mRadius * cosf( elevation0 ) * cosf( azimuth );

		glColor3f( color->r * intensity0,  color->g * intensity0,  color->b * intensity0);
		glVertex3f( x0, y0, z0 );
	}

	glEnd();

	glPopMatrix();
}

void Dome::generateDisplayList()
{
	mGeoDisplayList = glGenLists(4);
	mBlendZonesDisplayList = mGeoDisplayList+1;
	mChannelZonesDisplayList = mGeoDisplayList+2;
	mTexDisplayList = mGeoDisplayList+3;

	glNewList( mGeoDisplayList, GL_COMPILE );
	
	glPushMatrix();
	glRotatef(-mTilt, 1.0f, 0.0f, 0.0f);

	float elevation;
	float azimuth;
	float x, y, z;

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glBlendFunc(GL_ONE, GL_ONE);
	
	//view plane Quad
	/*glColor4f(0.5f, 0.5f, 0.5f, 0.3f);
	glBegin(GL_QUADS);
		glVertex3f(-5.18765f, 0.852762f, -5.88139f);
		glVertex3f(5.18765f, 0.852762f, -5.88139f);
		glVertex3f(5.18765f, 5.939094f, -2.23591f);
		glVertex3f(-5.18765f, 5.939094f, -2.23591f);
	glEnd();*/

	glLineWidth(2.0f);

	for(float e=0.0f; e<=90.0f; e+=2.25f)
	//for(int e=0; e<=90; e+=5)
	{
		elevation = glm::radians(e);
	
		if(static_cast<int>(e)%9 == 0) //nine degrees
			glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
		else
			glColor4f(1.0f, 1.0f, 1.0f, 0.5f);
		
		glBegin(GL_LINE_LOOP);

		y = mRadius * sinf( elevation );
		
		//for(int a = 0; a<360; a++)
		for(float a = 0.0f; a<360.0f; a+=2.25f)
		//for(int a = 0; a<360; a+=5)
	    {
			azimuth = glm::radians(a);
			x = mRadius * cosf( elevation ) * sinf( azimuth );
			z = -mRadius * cosf( elevation ) * cosf( azimuth );
			
			glVertex3f(x, y, z);
		}

		glEnd();
	}

	for(float a = 0.0f; a<360.0f; a+=2.25f)
	//for(int a = 0; a<360; a++)
	//for(int a = 0; a<360; a+=5)
	{
		azimuth = glm::radians(a);
		if(static_cast<int>(a)%45 == 0)
			glColor4f(0.0f, 1.0f, 1.0f, 0.5f);
		else if(static_cast<int>(a)%9 == 0)
			glColor4f(1.0f, 1.0f, 0.0f, 0.5f);
		else
			glColor4f(1.0f, 1.0f, 1.0f, 0.5f);

		
		glBegin(GL_LINE_STRIP);
		for(float e=0.0f; e<=90.0f; e+=2.25f)
		//for(int e=0; e<=90; e+=5)
		{
			elevation = glm::radians(e);
			x = mRadius * cosf( elevation ) * sinf( azimuth );
			y = mRadius * sinf( elevation );
			z = -mRadius * cosf( elevation ) * cosf( azimuth );
				
			glVertex3f(x, y, z);
		}

		glEnd();
	}

	glDisable(GL_BLEND);
	glPopMatrix();
	glEndList();

	glNewList( mBlendZonesDisplayList, GL_COMPILE );
	
	glPushMatrix();
	glRotatef(-mTilt, 1.0f, 0.0f, 0.0f);
	drawLongitudeLines(-180.0f, 8.25f, 90.0f, 128);
	drawLongitudeLines(72.0f - 180.0f, 8.25f, 90.0f, 128);
	drawLongitudeLines(144.0f - 180.0f, 8.25f, 90.0f, 128);
	drawLongitudeLines(-144.0f - 180.0f, 8.25f, 90.0f, 128);
	drawLongitudeLines(-72.0f - 180.0f, 8.25f, 90.0f, 128);
	drawLongitudeLines(-2.75f - 180.0f, 8.25f, 90.0f, 128);

	//Cap blend area
	drawLatitudeLines(8.25f+49.5f, 0.0f, 360.0f, 256);
	drawLatitudeLines(8.25f+56.0f, 0.0f, 360.0f, 256);

	//Cap-Ch1 blend
	/*drawVerticalFrustumLine(gmtl::Vec3f(-5.18765f, 0.852762f, -5.88139f),
		gmtl::Vec3f(-5.18765f, 5.939094f, -2.23591f),
		64);*/


	//Side blends
	drawLongitudeLines(-180.0f-6.35f, 0.0f, 8.25f+56.0f, 64);
	drawLongitudeLines(-180.0f+6.35f, 0.0f, 8.25f+56.0f, 64);

	drawLongitudeLines(72.0f-180.0f-6.35f, 0.0f, 8.25f+56.0f, 64);
	drawLongitudeLines(72.0f-180.0f+6.35f, 0.0f, 8.25f+56.0f, 64);

	drawLongitudeLines(144.0f-180.0f-6.35f, 0.0f, 8.25f+56.0f, 64);
	drawLongitudeLines(144.0f-180.0f+6.35f, 0.0f, 8.25f+56.0f, 64);

	drawLongitudeLines(-144.0f-180.0f-6.35f, 0.0f, 8.25f+56.0f, 64);
	drawLongitudeLines(-144.0f-180.0f+6.35f, 0.0f, 8.25f+56.0f, 64);

	drawLongitudeLines(-72.0f-180.0f-6.35f, 0.0f, 8.25f+56.0f, 64);
	drawLongitudeLines(-72.0f-180.0f+6.35f, 0.0f, 8.25f+56.0f, 64);

	//draw dome bottom
	drawLatitudeLines(8.25f, 0.0f, 360.0f, 256);

	//New blends
	//gmtl::Vec3f 

	glPopMatrix();
	glEndList();

	glNewList( mChannelZonesDisplayList, GL_COMPILE );
	
	glPushMatrix();
	glRotatef(-mTilt, 1.0f, 0.0f, 0.0f);
	//Draw channel edges
	//CH 1
	//Left
	drawVerticalFrustumLine( glm::vec3(-5.18765f, 0.852762f, -5.88139f),
		glm::vec3(-5.18765f, 5.939094f, -2.23591f),
		64);
	//Right
	drawVerticalFrustumLine( glm::vec3(5.18765f, 0.852762f, -5.88139f),
		glm::vec3(5.18765f, 5.939094f, -2.23591f),
		64);
	//Bottom
	drawVerticalFrustumLine( glm::vec3(-5.18765f, 0.852762f, -5.88139f),
		glm::vec3(5.18765f, 0.852762f, -5.88139f),
		64);
	//Top
	drawVerticalFrustumLine( glm::vec3(-5.18765f, 5.939094f, -2.23591f),
		glm::vec3(5.18765f, 5.939094f, -2.23591f),
		64);

	//CH 2
	//Left
	drawVerticalFrustumLine(glm::vec3(3.99047f, 0.85276f, -6.75120f),
		glm::vec3(0.52340f, 5.93909f, -5.62468f),
		64);
	//Right
	drawVerticalFrustumLine(glm::vec3(7.19661f, 0.85276f, 3.11630f),
		glm::vec3(3.72955f, 5.93909f, 4.24281f),
		64);
	//Bottom
	drawVerticalFrustumLine(glm::vec3(7.19661f, 0.85276f, 3.11630f),
		glm::vec3(3.99047f, 0.85276f, -6.75120f),
		64);
	//Top
	drawVerticalFrustumLine(glm::vec3(0.52340f, 5.93909f, -5.62468f),
		glm::vec3(3.72955f, 5.93909f, 4.24281f),
		64);

	//CH 3
	//Left
	drawVerticalFrustumLine(glm::vec3(7.65389f, 0.85276f, 1.70892f),
		glm::vec3(5.51113f, 5.93909f, -1.24034f),
		64);
	//Right
	drawVerticalFrustumLine(glm::vec3(-0.73990f, 0.85276f, 7.80737f),
		glm::vec3(-2.88266f, 5.93909f, 4.85811f),
		64);
	//Bottom
	drawVerticalFrustumLine(glm::vec3(7.65389f, 0.85276f, 1.70892f),
		glm::vec3(-0.73990f, 0.85276f, 7.80737f),
		64);
	//Top
	drawVerticalFrustumLine(glm::vec3(-2.88266f, 5.93909f, 4.85811f),
		glm::vec3(5.51113f, 5.93909f, -1.24034f),
		64);

	//CH 4
	//Left
	drawVerticalFrustumLine(glm::vec3(0.73990f, 0.85276f, 7.80737f),
		glm::vec3(2.88266f, 5.93909f, 4.85811f),
		64);
	//Right
	drawVerticalFrustumLine(glm::vec3(-7.65389f, 0.85276f, 1.70892f),
		glm::vec3(-5.51113f, 5.93909f, -1.24034f),
		64);
	//Bottom
	drawVerticalFrustumLine(glm::vec3(0.73990f, 0.85276f, 7.80737f),
		glm::vec3(-7.65389f, 0.85276f, 1.70892f),
		64);
	//Top
	drawVerticalFrustumLine(glm::vec3(-5.51113f, 5.93909f, -1.24034f),
		glm::vec3(2.88266f, 5.93909f, 4.85811f),
		64);

	//CH 5
	//Left
	drawVerticalFrustumLine(glm::vec3(-7.19661f, 0.85276f, 3.11630f),
		glm::vec3(-3.72955f, 5.93909f, 4.24281f),
		64);
	//Right
	drawVerticalFrustumLine(glm::vec3(-3.99047f, 0.85276f, -6.75120f),
		glm::vec3(-0.52340f, 5.93909f, -5.62468f),
		64);
	//Bottom
	drawVerticalFrustumLine(glm::vec3(-7.19661f, 0.85276f, 3.11630f),
		glm::vec3(-3.99047f, 0.85276f, -6.75120f),
		64);
	//Top
	drawVerticalFrustumLine(glm::vec3(-0.52340f, 5.93909f, -5.62468f),
		glm::vec3(-3.72955f, 5.93909f, 4.24281f),
		64);

	//CH 6
	//Left
	drawVerticalFrustumLine(glm::vec3(-4.04754f, 6.30954f, -3.67653f),
		glm::vec3(-3.67653f, 6.30954f, 4.04754f),
		64);
	//Right
	drawVerticalFrustumLine(glm::vec3(3.67653f, 6.30954f, -4.04754f),
		glm::vec3(4.04754f, 6.30954f, 3.67653f),
		64);
	//Bottom
	drawVerticalFrustumLine(glm::vec3(-4.04754f, 6.30954f, -3.67653f),
		glm::vec3(3.67653f, 6.30954f, -4.04754f),
		64);
	//Top
	drawVerticalFrustumLine(glm::vec3(4.04754f, 6.30954f, 3.67653f),
		glm::vec3(-3.67653f, 6.30954f, 4.04754f),
		64);
	glPopMatrix();
	glEndList();

	/*drawLongitudeLines(0.0f, 8.25f, 90.0f, 128);
	drawLongitudeLines(0.0f, 8.25f, 90.0f, 128);
	drawLongitudeLines(0.0f, 8.25f, 90.0f, 128);*/


	//Draw elevation line
	/*glColor3f(0.3f, 0.5f, 1.0f);
	glBegin(GL_LINE_LOOP);
	y = mRadius * sinf( gmtl::Math::deg2Rad(35.63f) );
	for(int a = 0; a<360; a++)
	{
		azimuth = gmtl::Math::deg2Rad(static_cast<float>(a));
		x = mRadius * cosf( gmtl::Math::deg2Rad(35.63f) ) * sinf( azimuth );
		z = -mRadius * cosf( gmtl::Math::deg2Rad(35.63f) ) * cosf( azimuth );
		
		glVertex3f(x, y, z);
	}
	glEnd();*/

	/*glLineWidth(1);
	glColor3f(0.7f, 0.1f, 1.0f);
	//frustum lines
	glBegin(GL_LINES);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(-5.18765f, 0.852762f, -5.88139f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(5.18765f, 0.852762f, -5.88139f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(5.18765f, 5.939094f, -2.23591f);
		glVertex3f(0.0f, 0.0f, 0.0f);
		glVertex3f(-5.18765f, 5.939094f, -2.23591f);
	glEnd();
	//viewplane
	glBegin(GL_LINE_LOOP);
		glVertex3f(-5.18765f, 0.852762f, -5.88139f);
		glVertex3f(5.18765f, 0.852762f, -5.88139f);
		glVertex3f(5.18765f, 5.939094f, -2.23591f);
		glVertex3f(-5.18765f, 5.939094f, -2.23591f);
	glEnd();

	drawLatitudeLines(8.25f, -42.35f, 42.35f, 64);
	drawLatitudeLines(21.9375f, -42.35f, 42.35f, 64);
	drawLatitudeLines(35.625f, -42.35f, 42.35f, 64);
	drawLatitudeLines(49.3125f, -42.35f, 42.35f, 64);
	drawLatitudeLines(63.0f, -42.35f, 42.35f, 64);

	drawLongitudeLines(-42.35f, 8.25f, 63.0f, 64);
	drawLongitudeLines(-21.175f, 8.25f, 63.0f, 64);
	drawLongitudeLines(0.0f, 8.25f, 63.0f, 64);
	drawLongitudeLines(21.175f, 8.25f, 63.0f, 64);
	drawLongitudeLines(42.35f, 8.25f, 63.0f, 64);

	drawVerticalFrustumLine(gmtl::Vec3f(-5.18765f, 0.852762f, -5.88139f),
		gmtl::Vec3f(-5.18765f, 5.939094f, -2.23591f),
		64);
	drawVerticalFrustumLine(gmtl::Vec3f(5.18765f, 0.852762f, -5.88139f),
		gmtl::Vec3f(5.18765f, 5.939094f, -2.23591f),
		64);
	drawVerticalFrustumLine(gmtl::Vec3f(-5.18765f, 0.852762f, -5.88139f),
		gmtl::Vec3f(5.18765f, 0.852762f, -5.88139f),
		64);
	drawVerticalFrustumLine(gmtl::Vec3f(-5.18765f, 5.939094f, -2.23591f),
		gmtl::Vec3f(5.18765f, 5.939094f, -2.23591f),
		64);

	glPopMatrix();
	glEndList();*/

	glNewList( mTexDisplayList, GL_COMPILE );
	glColor4f( 1.0f, 1.0f, 1.0f, 1.0f );

	DomeVertex dv0, dv1;

	float elevation0, elevation1;
	int e;
	float de0, de1;
	float lift = 7.5f;

	const int elevationSteps = 64;
	const int azimuthSteps = 256;

	//const int elevationSteps = 8;
	//const int azimuthSteps = 32;
	std::vector<DomeVertex> mRingVertices;
	for(e=0; e<(elevationSteps-1); e++)
	{
		mRingVertices.clear();
		
		de0 = static_cast<float>(e) / static_cast<float>(elevationSteps);
		elevation0 = glm::radians(lift + de0 * (90.0f - lift));
		
		de1 = static_cast<float>(e+1) / static_cast<float>(elevationSteps);
		elevation1 = glm::radians(lift + de1 * (90.0f - lift));
			
		glBegin( GL_TRIANGLE_STRIP );

		dv0.y = sinf( elevation0 );
		dv1.y = sinf( elevation1 );

		for(int a=0; a<=azimuthSteps; a++)
		{
			azimuth = glm::radians( static_cast<float>(a * 360)/static_cast<float>(azimuthSteps) );
				
			dv0.x = cosf( elevation0 ) * sinf( azimuth );
			dv0.z = -cosf( elevation0 ) * cosf( azimuth );

			dv1.x = cosf( elevation1 ) * sinf( azimuth );
			dv1.z = -cosf( elevation1 ) * cosf( azimuth );

			dv0.s = (static_cast<float>(elevationSteps - e ) / static_cast<float>(elevationSteps)) * sinf(azimuth);
			dv1.s = (static_cast<float>(elevationSteps - (e + 1)) / static_cast<float>(elevationSteps)) * sinf(azimuth);
			dv0.t = (static_cast<float>(elevationSteps - e ) / static_cast<float>(elevationSteps)) * -cosf(azimuth);
			dv1.t = (static_cast<float>(elevationSteps - (e + 1)) / static_cast<float>(elevationSteps)) * -cosf(azimuth);

			dv0.s = (dv0.s * 0.5f) + 0.5f;
			dv1.s = (dv1.s * 0.5f) + 0.5f;
			dv0.t = (dv0.t * 0.5f) + 0.5f;
			dv1.t = (dv1.t * 0.5f) + 0.5f;

			glMultiTexCoord2f( GL_TEXTURE0, dv0.s, dv0.t);
			glMultiTexCoord2f( GL_TEXTURE1, dv0.s, dv0.t);
			glVertex3f(dv0.x*mRadius, dv0.y*mRadius, dv0.z*mRadius);

			glMultiTexCoord2f( GL_TEXTURE0, dv1.s, dv1.t);
			glMultiTexCoord2f( GL_TEXTURE1, dv1.s, dv1.t);
			glVertex3f(dv1.x*mRadius, dv1.y*mRadius, dv1.z*mRadius);

			mRingVertices.push_back(dv0);
			//mRingVertices.push_back(dv1);
		}

		glEnd();
		mVertices.push_back(mRingVertices);
	}
		
	//CAP
	DomeVertex pole;
	mRingVertices.clear();

	e = elevationSteps - 1;
	de0 = static_cast<float>(e) / static_cast<float>(elevationSteps);
	elevation0 = glm::radians(lift + de0 * (90.0f - lift));

	de1 = static_cast<float>(e + 1) / static_cast<float>(elevationSteps);
	elevation1 = glm::radians(lift + de1 * (90.0f - lift));

	glBegin( GL_TRIANGLE_FAN );

	dv1.x = 0.0f;
	dv1.z = 0.0f;

	dv0.y = sinf( elevation0 );
	dv1.y = sinf( elevation1 );

	dv1.s = 0.5f;
	dv1.t = 0.5f;

	glMultiTexCoord2f( GL_TEXTURE0, dv1.s, dv1.t);
	glMultiTexCoord2f( GL_TEXTURE1, dv1.s, dv1.t);
	glVertex3f(dv1.x * mRadius, dv1.y * mRadius, dv1.z * mRadius);

	for(int a=0; a<=azimuthSteps; a++)
	{
		azimuth = glm::radians( static_cast<float>(a * 360)/static_cast<float>(azimuthSteps) );
			
		dv0.x =  cosf( elevation0 ) * sinf( azimuth );
		dv0.z = -cosf( elevation0 ) * cosf( azimuth );

		dv0.s = (static_cast<float>(elevationSteps - e) / static_cast<float>(elevationSteps)) * sinf(azimuth);
		dv0.t = (static_cast<float>(elevationSteps - e) / static_cast<float>(elevationSteps)) * -cosf(azimuth);
		dv0.s = (dv0.s * 0.5f) + 0.5f;
		dv0.t = (dv0.t * 0.5f) + 0.5f;
			
		glMultiTexCoord2f( GL_TEXTURE0, dv0.s, dv0.t);
		glMultiTexCoord2f( GL_TEXTURE1, dv0.s, dv0.t);
		glVertex3f(dv0.x * mRadius, dv0.y * mRadius, dv0.z * mRadius);

		mRingVertices.push_back(dv0);
	}

	mVertices.push_back(mRingVertices);
	pole = dv1;

	glEnd();
	glEndList();

#if EXPORT_DOME_MODEL
	std::ofstream file;
	std::string outputPath = "dome.obj";
	file.open(outputPath, std::ios::out);
	if (file.is_open())
	{
		file << std::fixed;
		file << std::setprecision(8);

		file << "# SGCT dome mesh\n";

		//export vertices
		for (std::size_t i = 0; i < mVertices.size(); i++)
			for (std::size_t j = 0; j <= azimuthSteps; j++)
				file << "v " << mVertices[i][j].x << " " << mVertices[i][j].y << " " << mVertices[i][j].z << std::endl;
		file << "v " << pole.x << " " << pole.y << " " << pole.z << std::endl;

		//export texture coords
		for (std::size_t i = 0; i < mVertices.size(); i++)
			for (std::size_t j = 0; j <= azimuthSteps; j++)
				file << "vt " << mVertices[i][j].s << " " << mVertices[i][j].t << " " << 0.0f << std::endl;
		file << "vt " << pole.s << " " << pole.t << " " << 0.0f << std::endl;

		//export faces
		std::size_t index;
		for (std::size_t i = 0; i < (mVertices.size() - 1); i++)
		{
			index = i*(azimuthSteps + 1) + 1;
			
			for (std::size_t j = 0; j < azimuthSteps; j++)
			{
				file << "f ";
				file << index << "/" << index << " ";
				file << index + 1 << "/" << index + 1 << " ";
				file << index + azimuthSteps + 1 << "/" << index + azimuthSteps + 1;
				file << std::endl;

				file << "f ";
				file << index + 1 << "/" << index + 1 << " ";
				file << index + azimuthSteps + 2 << "/" << index + azimuthSteps + 2 << " ";
				file << index + azimuthSteps + 1 << "/" << index + azimuthSteps + 1;
				file << std::endl;

				index++;
			}
		}
		//cap
		index = (mVertices.size()-1)*(azimuthSteps+1) + 1;
		std::size_t last = mVertices.size()*(azimuthSteps+1) + 1;
		for (std::size_t i = 0; i < azimuthSteps; i++)
		{
			file << "f " << last << "/" << last << " ";
			file << index + i << "/" << index + i << " ";
			file << index + i + 1 << "/" << index + i + 1  << std::endl;
		}

		file.close();
	}
#endif
}

void Dome::drawLatitudeLines(float latitude, float minLongitude, float maxLongitude, int segments)
{
	float azimuth;
	float x, y, z;

	glLineWidth(2);
	float hStepSize = (maxLongitude-minLongitude) / static_cast<float>(segments);
	
	glColor3f(1.0f, 0.1f, 0.1f);
	glBegin(GL_LINE_STRIP);
	y = mRadius * sinf( glm::radians(latitude) );
	for(int a = 0; a<=segments; a++)
	{
		azimuth = glm::radians(minLongitude + static_cast<float>(a)*hStepSize);
		x = mRadius * cosf( glm::radians(latitude) ) * sinf( azimuth );
		z = -mRadius * cosf( glm::radians(latitude) ) * cosf( azimuth );
		
		glVertex3f(x, y, z);
	}
	glEnd();
}

void Dome::drawLongitudeLines(float longitude, float minLatitude, float maxLatitude, int segments)
{
	float elevation;
	float x, y, z;

	glLineWidth(2);
	float vStepSize = (maxLatitude - minLatitude) / static_cast<float>(segments);

	glColor3f(1.0f, 0.1f, 0.1f);

	glBegin(GL_LINE_STRIP);
	for(int e=0; e<=segments; e++)
	{
		elevation = glm::radians(minLatitude + static_cast<float>(e)*vStepSize);
		x = mRadius * cosf( elevation ) * sinf( glm::radians(longitude) );
		y = mRadius * sinf( elevation );
		z = -mRadius * cosf( elevation ) * cosf( glm::radians(longitude) );
			
		glVertex3f(x, y, z);
	}
	glEnd();
}

void Dome::drawVerticalFrustumLine(glm::vec3 v1, glm::vec3 v2, int segments)
{
	glm::vec3 ray;
	float a;

	glLineWidth(3);
	glColor3f(0.1f, 0.1f, 1.0f);
	glBegin(GL_LINE_STRIP);
	for(int e=0; e<=segments; e++)
	{
		a = static_cast<float>( segments-e ) / static_cast<float>(segments);
		ray = (v1 * a) + (v2 * (1.0f-a));
		glm::normalize( ray );
		ray *= mRadius;
		glVertex3fv( glm::value_ptr( ray ) );
	}
	glEnd();
	
	/*horAngle = gmtl::Math::deg2Rad(horAngle);
	float x,y,z;
	float elevation;

	gmtl::Vec3f v1,v2;
	gmtl::Matrix44f mat;

	glLineWidth(2);

	float vStepSize = (maxVerAngle - minVerAngle) / static_cast<float>(segments);

	glColor3f(0.1f, 0.1f, 1.0f);
	glBegin(GL_LINE_STRIP);
	for(int e=0; e<=segments; e++)
	{
		elevation = gmtl::Math::deg2Rad(minVerAngle + static_cast<float>(e)*vStepSize);
		v1.set( 0.0f, mRadius * sinf(elevation), -mRadius * cosf(elevation));
		gmtl::setRot(mat, gmtl::EulerAngleXYZf( 0.0f, horAngle, 0.0f));
		v2 = mat * v1;
		
		//gmtl::setRot(mat, gmtl::EulerAngleXYZf( elevation, horAngle, 0.0f));
		//rotVec = mat * vec;
		x = mRadius * sinf( horAngle );
		y = mRadius * cosf( horAngle ) * sinf(elevation);
		z = -mRadius * cosf( horAngle ) * cosf(elevation);
			
		glVertex3f(x, y, z);
		glVertex3fv( v2.getData() );

		//glPushMatrix();
		//glRotatef( horAngle, 0.0f, 1.0f, 0.0f);
		//glRotatef( elevation, 1.0f, 0.0f, 0.0f);
		//glVertex3fv( vec.getData() );
		//glVertex3f(0.0f, 0.0f, -4.0f);
		//glPopMatrix();
	}
	glEnd();*/
}

void Dome::drawHorizontalFrustumLine(float verAngle, float minHorAngle, float maxHorAngle, int segments)
{
	float azimuth;
	float x, y, z;

	glLineWidth(2);
	verAngle = glm::radians( verAngle );

	float hStepSize = (maxHorAngle-minHorAngle) / static_cast<float>(segments);
	
	glColor3f(0.1f, 0.1f, 1.0f);
	glBegin(GL_LINE_STRIP);
	for(int a = 0; a<=segments; a++)
	{
		azimuth = glm::radians(minHorAngle + static_cast<float>(a)*hStepSize);

		x = mRadius * sinf( azimuth );
		y = mRadius * cosf( azimuth ) * sinf(verAngle);
		z = -mRadius * cosf( azimuth ) * cosf(verAngle);
		
		glVertex3f(x, y, z);
	}
	glEnd();
}