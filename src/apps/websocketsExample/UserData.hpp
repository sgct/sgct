#ifndef _USER_DATA_
#define _USER_DATA_

class UserData
{
public:
    //enum UserColor { RED = 0, YELLOW, GREEN, CYAN, BLUE };
    
	UserData();
	~UserData();
    void set(float theta, float phi, float r, float g, float b, float timeStamp);
    void setCartesian2d(int x, int y, float r, float g, float b, float timeStamp);
    
    inline float getPhi() { return mPhi; }
    inline float getTheta() { return mTheta; }
    inline float getTimeStamp() { return mTimeStamp; }
    //inline UserColor getColor() { return mColor; }
    inline float getRed() { return mRed; }
    inline float getGreen() { return mGreen; }
    inline float getBlue() { return mBlue; }
    
		
private:
	float mTheta, mPhi;
    float mTimeStamp;
    float mRed, mGreen, mBlue;
};

#endif