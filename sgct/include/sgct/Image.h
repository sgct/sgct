#ifndef _IMAGE_H_
#define _IMAGE_H_

namespace core_sgct
{

class Image
{
public:
	bool load(const char * filename);
	bool loadPNG(const char * filename);
	void cleanup();
	unsigned char * getData();
	int getChannels();
	int getSizeX();
	int getSizeY();

private:
	int channels;
	int size_x;	
	int size_y;
	char _filename[256];
	unsigned char * data;
};

}

#endif

