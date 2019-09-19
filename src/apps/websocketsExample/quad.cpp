#include "quad.h"
#include "sgct.h"

Quad::Quad()
{
    mVertexArray = GL_FALSE;
    mVertexPositionBuffer = GL_FALSE;
    mVertexTextureCoordBuffer = GL_FALSE;
}

Quad::~Quad()
{
}

void Quad::create(float width, float height)
{
    const GLfloat vertex_position_data[] = {
        -width/2.0f, height/2.0f, 0.0f,
        -width/2.0f, -height/2.0f, 0.0f,
        width/2.0f, height/2.0f, 0.0f,
        width/2.0f, -height/2.0f, 0.0f
        
    };
    
    const GLfloat tex_coord_data[] = {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 0.0f
    };
    
    //generate the VAO
    glGenVertexArrays(1, &mVertexArray);
    glBindVertexArray(mVertexArray);
    
    //generate VBO for vertex positions
    glGenBuffers(1, &mVertexPositionBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexPositionBuffer);
    //upload data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_position_data), vertex_position_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(
                          0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          reinterpret_cast<void*>(0) // array buffer offset
                          );
    
    //generate VBO for vertex positions
    glGenBuffers(1, &mVertexTextureCoordBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, mVertexTextureCoordBuffer);
    //upload data to GPU
    glBufferData(GL_ARRAY_BUFFER, sizeof(tex_coord_data), tex_coord_data, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(
                          1,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
                          2,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          reinterpret_cast<void*>(0) // array buffer offset
                          );

    
    //unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void Quad::bind()
{
    glBindVertexArray(mVertexArray);
}

void Quad::unbind()
{
    glBindVertexArray(0);
}

void Quad::draw()
{
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void Quad::clear()
{
    if(mVertexTextureCoordBuffer)
        glDeleteBuffers(1, &mVertexTextureCoordBuffer);
    if(mVertexPositionBuffer)
        glDeleteBuffers(1, &mVertexPositionBuffer);
    if(mVertexArray)
        glDeleteVertexArrays(1, &mVertexArray);
}