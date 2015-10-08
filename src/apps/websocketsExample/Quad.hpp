#ifndef _QUAD_
#define _QUAD_

class Quad
{
public:
    Quad();
	~Quad();
    void create(float width, float height);
    void bind();
    void unbind();
    void draw();
    void clear();
		
private:
	unsigned int mVertexArray;
    unsigned int mVertexPositionBuffer;
    unsigned int mVertexTextureCoordBuffer;
};

#endif