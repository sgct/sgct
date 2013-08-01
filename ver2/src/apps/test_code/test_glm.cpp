#include "sgct.h"

int main( int argc, char* argv[] )
{

    fprintf(stderr, "GLM test.\n");
	
	/*double xRot = 90.0;
	double yRot = 0.0;
	double zRot = -90.0;*/
	
	//headrot
	double xRot = 0.0;
	double yRot = 0.0;
	double zRot = 45.0;
	
	//VR juggler
	/*double xRot = 0.0;
	double yRot = -90.0;
	double zRot = 90.0;*/
	
	glm::dquat rotQuat;

    rotQuat = glm::rotate( rotQuat, xRot, glm::dvec3(1.0, 0.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, yRot, glm::dvec3(0.0, 1.0, 0.0) );
	rotQuat = glm::rotate( rotQuat, zRot, glm::dvec3(0.0, 0.0, 1.0) );
	
	glm::dmat4 invMat = glm::inverse( glm::mat4_cast(rotQuat) );
	glm::dvec3 angles = glm::eulerAngles( glm::quat_cast(invMat) );
	
	fprintf(stderr, "Angle input: %g %g %g\n", xRot, yRot, zRot);
	fprintf(stderr, "Angle output: %g %g %g\n", angles.x, angles.y, angles.z);
	
	glm::dvec3 headPos(0.0, 2.0, 0.0);
	
	glm::dmat4 trans = glm::translate( glm::dmat4(1.0), headPos ) * glm::mat4_cast(rotQuat);
	
	double mEyeSeparation = 0.06;
	glm::dvec4 eyeOffsetVec( mEyeSeparation/2.0, 0.0, 0.0, 0.0 );
	
	glm::dvec4 centerPos(0.0, 0.0, 0.0, 1.0);
	glm::dvec4 leftPos = centerPos - eyeOffsetVec;
	glm::dvec4 rightPos = centerPos + eyeOffsetVec;
	
	glm::dvec3 centerEye( trans * centerPos );
	glm::dvec3 leftEye( trans * leftPos );
	glm::dvec3 rightEye( trans * rightPos );
	
	fprintf(stderr, "\nHead pos: x=%g y=%g z=%g\n", headPos.x, headPos.y, headPos.z);
	fprintf(stderr, "Head rot: x=%g y=%g z=%g\n\n", xRot, yRot, zRot);
	
	fprintf(stderr, "center pos: x=%g y=%g z=%g\n\n", centerEye.x, centerEye.y, centerEye.z);
	fprintf(stderr, "left pos: x=%g y=%g z=%g\n\n", leftEye.x, leftEye.y, leftEye.z);
	fprintf(stderr, "right pos: x=%g y=%g z=%g\n\n", rightEye.x, rightEye.y, rightEye.z);
	
	// Exit program
	exit( EXIT_SUCCESS );
}
