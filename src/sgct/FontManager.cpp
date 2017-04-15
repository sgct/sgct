/*************************************************************************
Copyright (c) 2012-2015 Miroslav Andel
All rights reserved.

For conditions of distribution and use, see copyright notice in sgct.h
*************************************************************************/

#include <sgct/ogl_headers.h>
#include <sgct/FontManager.h>
#include <sgct/MessageHandler.h>
#include <sgct/Engine.h>
#include <sgct/helpers/SGCTStringFunctions.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <stdio.h>

const static std::string Font_Vert_Shader = "\
**glsl_version**\n\
\n\
layout (location = 0) in vec2 TexCoord;\n\
layout (location = 1) in vec2 Position;\n\
\n\
uniform mat4 MVP;\n\
out vec2 UV;\n\
\n\
void main()\n\
{\n\
    gl_Position = MVP * vec4(Position, 0.0, 1.0);\n\
    UV = TexCoord;\n\
}\n";

const static std::string Font_Frag_Shader = "\
**glsl_version**\n\
\n\
uniform vec4 Col;\n\
uniform vec4 StrokeCol;\n\
uniform sampler2D Tex;\n\
\n\
in vec2 UV;\n\
out vec4 Color;\n\
\n\
void main()\n\
{\n\
    vec2 LuminanceAlpha = texture(Tex, UV.st).rg;\n\
    vec4 blend = mix(StrokeCol, Col, LuminanceAlpha.r);\n\
    Color = blend * vec4(1.0, 1.0, 1.0, LuminanceAlpha.g);\n\
}\n";

const static std::string Font_Vert_Shader_Legacy = "\
**glsl_version**\n\
\n\
void main()\n\
{\n\
    gl_TexCoord[0] = gl_MultiTexCoord0;\n\
    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;\n\
}\n";

const static std::string Font_Frag_Shader_Legacy = "\
**glsl_version**\n\
\n\
uniform vec4 Col;\n\
uniform vec4 StrokeCol;\n\
uniform sampler2D Tex;\n\
\n\
void main()\n\
{\n\
    vec4 LuminanceAlpha = texture2D(Tex, gl_TexCoord[0].st);\n\
    vec4 blend = mix(StrokeCol, Col, LuminanceAlpha.r);\n\
    gl_FragColor = blend * vec4(1.0, 1.0, 1.0, LuminanceAlpha.a);\n\
}\n";

//----------------------------------------------------------------------
// Helper functions
//----------------------------------------------------------------------

/*!
This function gets the first power of 2 >= the int that we pass it.
*/
inline int NextP2 ( int a )
{
    int rval=1;
    while(rval<a) rval<<=1;
    return rval;
}

//----------------------------------------------------------------------
// FontManager Implementations
//----------------------------------------------------------------------

/*! Initiate FontManager instance to NULL */
sgct_text::FontManager * sgct_text::FontManager::mInstance = NULL;

/*! Default height in pixels for all font faces */
const FT_Short sgct_text::FontManager::mDefaultHeight = 10;

/*!
Constructor initiates the freetyp library
*/
sgct_text::FontManager::FontManager(void)
{
    FT_Error error = FT_Init_FreeType( &mFTLibrary );
    mStrokeSize = 1;
    mStrokeColor.r = 0.0f;
    mStrokeColor.g = 0.0f;
    mStrokeColor.b = 0.0f;
    mStrokeColor.a = 0.9f;

    mDrawInScreenSpace = true;

    if ( error != 0 )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Could not initiate Freetype library.\n" );
        return; // No need to continue
    }

    //
    // Set default font path
    //
#if __WIN32__
    char fontDir[128];
    if( GetWindowsDirectory(fontDir,128) > 0)
    {
        mDefaultFontPath.assign( fontDir );
        mDefaultFontPath += "\\Fonts\\";
    }
#elif __APPLE__
    //System Fonts
    mDefaultFontPath.assign( "/Library/Fonts/" );
#else
   mDefaultFontPath.assign( "/usr/share/fonts/truetype/freefont/" );
#endif

    mMVPLoc = -1;
    mColLoc = -1;
    mStkLoc = -1;
    mTexLoc = -1;

    mUseMipMaps = false;
}

/*!
Destructor cleans up all font objects, textures and shaders
*/
sgct_text::FontManager::~FontManager(void)
{
    std::set<Font>::iterator it = mFonts.begin();
    std::set<Font>::iterator end = mFonts.end();

    for( ; it != end; ++it )
    {
        const_cast<Font&>( (*it) ).clean();
    }

    if( mFTLibrary != NULL )
    {
        FT_Done_FreeType( mFTLibrary );
    }

    mShader.deleteProgram();
}

/*!
Set the default font path. This will be the directory where font files will be searched
for by default. If not explicitly set the default font path will be the windows font folder.
@param    path    The directory where the default font files are located
*/
void sgct_text::FontManager::setDefaultFontPath( const std::string & path )
{
    mDefaultFontPath = path;
}

/*!
Set the stroke (border) size
@param    size    The stroke size in pixels
*/
void sgct_text::FontManager::setStrokeSize( signed long size )
{
    mStrokeSize = size;
}

/*!
Set the stroke (border) color
*/
void sgct_text::FontManager::setStrokeColor( glm::vec4 color )
{
    mStrokeColor = color;
}

/*!
Set if screen space coordinates should be used or buffer coordinates
*/
void sgct_text::FontManager::setDrawInScreenSpace( bool state )
{
    mDrawInScreenSpace = state;
}

/*!
Set if mip maps should be generated when loading the font
*/
void sgct_text::FontManager::setUseMipMaps(bool state)
{
    mUseMipMaps = state;
}

/*!
Adds a font file to the manager.
@param    fontName    Specify a name for the font
@param    path        Path to the font file
@param    fontPath    If it is a local font path directory or using the default path
*/
bool sgct_text::FontManager::addFont( const std::string & fontName, std::string path, FontPath fontPath )
{
    // Perform file exists check
    if( fontPath == FontPath_Default )
    {
        path = mDefaultFontPath + path;
    }

    bool inserted = mFontPaths.insert( std::pair<std::string, std::string>( fontName, path ) ).second;

    if( !inserted )
    {

        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_WARNING, "Font with name '%s' already specified.\n", fontName.c_str() );
        return false;
    }

    return true;
}

/*!
Get a font face that is loaded into memory.
@param    name    Name of the font
@param    height    Height in  pixels for the font
@return    Pointer to the font face, NULL if not found
*/
const sgct_text::Font * sgct_text::FontManager::getFont( const std::string & fontName, unsigned int height )
{
    // If there will be a lot of switching between font sizes consider saving every font face as a unique font instead
    // of resizing
    sgct_text::Font searchFont( fontName, static_cast<float>( height ) );

    std::set<sgct_text::Font>::iterator it = std::find( mFonts.begin(), mFonts.end(), searchFont );

    if( it == mFonts.end() )
    {
        it = createFont( fontName, height );
    }

    return (it != mFonts.end() ) ? &(*it) : NULL;
}

/*!
Get the SGCT default font face that is loaded into memory.
@param    height    Height in  pixels for the font
@return    Pointer to the font face, NULL if not found
*/
const sgct_text::Font * sgct_text::FontManager::getDefaultFont( unsigned int height )
{
    return getFont("SGCTFont", height);
}

/*!
Creates font textures with a specific height if a path to the font exists
@param    fontName    Name of the font
@param    height        Height of the font in pixels
@return    Iterator to the newly created font, end of the Fonts container if something went wrong
*/
std::set<sgct_text::Font>::iterator sgct_text::FontManager::createFont( const std::string & fontName, unsigned int height )
{
    std::map<std::string, std::string>::iterator it = mFontPaths.find( fontName );

    if( it == mFontPaths.end() )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: No font file specified for font [%s].\n", fontName.c_str() );
        return mFonts.end();
    }

    if( mFTLibrary == NULL )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: Freetype library is not initialized, can't create font [%s].\n", fontName.c_str() );
        return mFonts.end();
    }

    FT_Face face;
    FT_Error error = FT_New_Face( mFTLibrary, it->second.c_str(), 0, &face );

    if ( error == FT_Err_Unknown_File_Format )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: Unsopperted file format [%s] for font [%s].\n", it->second.c_str(), fontName.c_str() );
        return mFonts.end();
    }
    else if( error != 0 || face == NULL )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: Font '%s' not found!\n", it->second.c_str());
        return mFonts.end();
    }

    if( FT_Set_Char_Size( face, height << 6, height << 6, 96, 96) != 0 )
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: Could not set pixel size for font[%s].\n", fontName.c_str() );
        return mFonts.end();
    }

    // Create the font when all error tests are done
    Font newFont = Font();
    newFont.init( fontName, height );

    static bool shaderCreated = false;

    if( !shaderCreated )
    {
        std::string vert_shader;
        std::string frag_shader;
        
        mShader.setName("FontShader");
        if( sgct::Engine::instance()->isOGLPipelineFixed() )
        {
            vert_shader = Font_Vert_Shader_Legacy;
            frag_shader = Font_Frag_Shader_Legacy;
        }
        else
        {
            vert_shader = Font_Vert_Shader;
            frag_shader = Font_Frag_Shader;
        }

        //replace glsl version
        sgct_helpers::findAndReplace(vert_shader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());
        sgct_helpers::findAndReplace(frag_shader, "**glsl_version**", sgct::Engine::instance()->getGLSLVersion());

        if(!mShader.addShaderSrc(vert_shader, GL_VERTEX_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load font vertex shader\n");
        if(!mShader.addShaderSrc(frag_shader, GL_FRAGMENT_SHADER, sgct::ShaderProgram::SHADER_SRC_STRING))
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "Failed to load font fragment shader\n");
        mShader.createAndLinkProgram();
        mShader.bind();

        if( !sgct::Engine::instance()->isOGLPipelineFixed() )
            mMVPLoc = mShader.getUniformLocation( "MVP" );
        mColLoc = mShader.getUniformLocation( "Col" );
        mStkLoc = mShader.getUniformLocation( "StrokeCol" );
        mTexLoc = mShader.getUniformLocation( "Tex" );
        mShader.unbind();

        shaderCreated = true;
    }

    //This is where we actually create each of the fonts display lists.
    if(sgct::Engine::instance()->isOGLPipelineFixed() )
    {
        for( unsigned char i = 0;i < 128; ++i )
            if(!makeDisplayList( face, i, newFont ))
            {
                newFont.clean();
                return mFonts.end();
            }
    }
    else
    {
        if( !makeVBO( face, newFont ) )
        {
            newFont.clean();
            return mFonts.end();
        }
    }

    FT_Done_Face(face);

    return mFonts.insert( newFont ).first;
}


/*!
Create a display list for the passed character
@param    face        Font face to create glyph from
@param    ch            Character to create glyph for
@param    font        Font to create for
@param    texBase        Texture base
@return If display list character created successfully
*/
bool sgct_text::FontManager::makeDisplayList ( FT_Face face, char ch, Font & font )
{

    //The first thing we do is get FreeType to render our character
    //into a bitmap.  This actually requires a couple of FreeType commands:

    //Load the Glyph for our character.
    /*
     Hints:
     http://www.freetype.org/freetype2/docs/reference/ft2-base_interface.html#FT_LOAD_XXX
    */

    FT_UInt char_index = FT_Get_Char_Index(face, ch);
    if (char_index == 0)
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "FontManager: Missing face for char %u!\n", static_cast<unsigned int>(ch));

    if (FT_Load_Glyph(face, char_index, FT_LOAD_FORCE_AUTOHINT))
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FT_Load_Glyph failed for char [%c].\n", ch );
        // Implement error message " char %s"
        return false;
    }

    int width;
    int height;
    unsigned char * pixels = NULL;

    //load pixel data
    GlyphData gd;
    if (!getPixelData(face, width, height, &pixels, &gd))
    {
        sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FT_Get_Glyph failed for char [%c].\n", ch);
    }

    //create texture
    if (char_index > 0)
        font.generateTexture(ch, width, height, pixels, mUseMipMaps);

    //With the texture created, we don't need to expanded data anymore
    delete[] pixels;

    //So now we can create the display list
    glNewList( font.getListBase() + ch, GL_COMPILE );

    //glBindTexture( GL_TEXTURE_2D, textureId );

    //glPushMatrix();

    //first we need to move over a little so that
    //the character has the right amount of space
    //between it and the one before it.
    //glTranslatef( (GLfloat)bitmap_glyph->left,0,0);

    //Now we move down a little in the case that the
    //bitmap extends past the bottom of the line
    //(this is only true for characters like 'g' or 'y'.
    //glTranslatef( 0, (GLfloat)bitmap_glyph->top-bitmap.rows, 0 );

    glm::vec2 offset( static_cast<float>(gd.mBitmapGlyph->left),
        static_cast<float>(gd.mBitmapGlyph->top - gd.mBitmapPtr->rows));

    //Here we draw the texturemaped quads.
    //The bitmap that we got from FreeType was not
    //oriented quite like we would like it to be,
    //so we need to link the texture to the quad
    //so that the result will be properly aligned.

    glBegin(GL_QUADS);
        glTexCoord2f( 0.0f, 0.0f );
        glVertex3f( offset.x, (GLfloat)height + offset.y, 0.0f );

        glTexCoord2f( 0.0f, 1.0f );
        glVertex3f(    offset.x, offset.y, 0.0f );

        glTexCoord2f( 1.0f, 1.0f );
        glVertex3f( (GLfloat)width + offset.x, offset.y, 0.0f );

        glTexCoord2f( 1.0f, 0.0f );
        glVertex3f( (GLfloat)width + offset.x, (GLfloat)height + offset.y, 0.0f );
    glEnd();
    //glPopMatrix();

    //Finnish the display list
    glEndList();

    //delete the stroke glyph
    FT_Stroker_Done(gd.mStroker);
    FT_Done_Glyph(gd.mStrokeGlyph);
    // Can't delete them while they are used, delete when font is cleaned
    font.AddGlyph(gd.mGlyph);
    font.setCharWidth( ch, static_cast<float>( face->glyph->advance.x >> 6 ) );

    return true;
}

/*!
Create vertex buffer objects for the passed character
@param    face        Font face to create glyph from
@param    ch            Character to create glyph for
@param    font        Font to create for
@param    texBase        Texture base
@return If display list character created successfully
*/
bool sgct_text::FontManager::makeVBO( FT_Face face, Font & font )
{
    std::vector<float> coords;

    for( unsigned char ch = 0; ch < 128; ++ch )
    {
        FT_UInt char_index = FT_Get_Char_Index(face, ch);
        if (char_index == 0)
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_DEBUG, "FontManager: Missing face for char %u!\n", static_cast<unsigned int>(ch));

        if( FT_Load_Glyph( face, char_index, FT_LOAD_FORCE_AUTOHINT ) )
        {
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FT_Load_Glyph failed for char [%c].\n", ch );
            return false;
        }

        int width;
        int height;
        unsigned char * pixels = NULL;

        //load pixel data
        GlyphData gd;
        if (!getPixelData(face, width, height, &pixels, &gd))
        {
            sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FT_Get_Glyph failed for char [%c].\n", ch);
        }

        //create texture
        if (char_index > 0)
            font.generateTexture(ch, width, height, pixels, mUseMipMaps);

        //With the texture created, we don't need to expanded data anymore
        delete[] pixels;

        glm::vec2 offset( static_cast<float>(gd.mBitmapGlyph->left),
            static_cast<float>(gd.mBitmapGlyph->top - gd.mBitmapPtr->rows));

        coords.push_back( 0.0f );
        coords.push_back( 1.0f );
        coords.push_back( offset.x );
        coords.push_back( offset.y );

        coords.push_back( 1.0f );
        coords.push_back( 1.0f );
        coords.push_back( static_cast<float>(width) + offset.x );
        coords.push_back( offset.y );

        coords.push_back( 0.0f ); //s
        coords.push_back( 0.0f ); //t
        coords.push_back( offset.x ); //x
        coords.push_back( static_cast<float>(height) + offset.y ); //y

        coords.push_back( 1.0f );
        coords.push_back( 0.0f );
        coords.push_back( static_cast<float>(width) + offset.x );
        coords.push_back( static_cast<float>(height) + offset.y );

        //delete the stroke glyph
        FT_Stroker_Done( gd.mStroker );
        FT_Done_Glyph( gd.mStrokeGlyph );

        // Can't delete them while they are used, delete when font is cleaned
        font.AddGlyph( gd.mGlyph );
        font.setCharWidth( ch, static_cast<float>( face->glyph->advance.x >> 6 ) );
    }

    glBindVertexArray(font.getVAO());
    glBindBuffer(GL_ARRAY_BUFFER, font.getVBO());
    glBufferData(GL_ARRAY_BUFFER, coords.size() * sizeof(float), &coords[0], GL_STATIC_DRAW );

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(
        0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        4*sizeof(float),    // stride
        reinterpret_cast<void*>(0) // array buffer offset
    );

    glVertexAttribPointer(
        1,                  // attribute 1
        2,                  // size
        GL_FLOAT,           // type
        GL_FALSE,           // normalized?
        4*sizeof(float),    // stride
        reinterpret_cast<void*>(8) // array buffer offset
    );

    //unbind
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    return true;
}

bool sgct_text::FontManager::getPixelData(FT_Face face, int & width, int & height, unsigned char ** pixels, GlyphData * gd)
{
    //Move the face's glyph into a Glyph object.
    if (FT_Get_Glyph(face->glyph, &(gd->mGlyph)) || FT_Get_Glyph(face->glyph, &(gd->mStrokeGlyph)))
    {
        return false;
    }

    gd->mStroker = NULL;
    FT_Error error = FT_Stroker_New(mFTLibrary, &(gd->mStroker));
    if (!error)
    {
        FT_Stroker_Set(gd->mStroker, 64 * mStrokeSize,
            FT_STROKER_LINECAP_ROUND,
            FT_STROKER_LINEJOIN_ROUND,
            0);

        error = FT_Glyph_Stroke(&(gd->mStrokeGlyph), gd->mStroker, 1);
    }

    //Convert the glyph to a bitmap.
    FT_Glyph_To_Bitmap(&(gd->mGlyph), ft_render_mode_normal, 0, 1);
    gd->mBitmapGlyph = (FT_BitmapGlyph)(gd->mGlyph);

    FT_Glyph_To_Bitmap(&(gd->mStrokeGlyph), ft_render_mode_normal, 0, 1);
    gd->mBitmapStrokeGlyph = (FT_BitmapGlyph)(gd->mStrokeGlyph);

    //This pointer will make accessing the bitmap easier
    gd->mBitmapPtr = &(gd->mBitmapGlyph->bitmap);
    gd->mStrokeBitmapPtr = &(gd->mBitmapStrokeGlyph->bitmap);

    //Use our helper function to get the widths of
    //the bitmap data that we will need in order to create
    //our texture.
    width = gd->mStrokeBitmapPtr->width; //stroke is always larger
    height = gd->mStrokeBitmapPtr->rows;

    //Allocate memory for the texture data.
    (*pixels) = new unsigned char[2 * width * height];

    //read alpha to one channel and stroke - alpha in the second channel
    //We use the ?: operator so that value which we use
    //will be 0 if we are in the padding zone, and whatever
    //is the the Freetype bitmap otherwise.
    int k, l;
    int diff_offset[2];
    diff_offset[0] = (gd->mStrokeBitmapPtr->width - gd->mBitmapPtr->width) >> 1;
    diff_offset[1] = (gd->mStrokeBitmapPtr->rows - gd->mBitmapPtr->rows) >> 1;
    for (int j = 0; j < height; ++j)
    {
        for (int i = 0; i < width; ++i)
        {
            k = i - diff_offset[0];
            l = j - diff_offset[1];
            (*pixels)[2 * (i + j*width)] =
                (k >= gd->mBitmapPtr->width || l >= gd->mBitmapPtr->rows || k < 0 || l < 0) ?
                0 : gd->mBitmapPtr->buffer[k + gd->mBitmapPtr->width*l];

            unsigned char strokeVal = (i >= gd->mStrokeBitmapPtr->width || j >= gd->mStrokeBitmapPtr->rows) ?
                0 : gd->mStrokeBitmapPtr->buffer[i + gd->mStrokeBitmapPtr->width*j];

            //simple union
            (*pixels)[2 * (i + j*width) + 1] = strokeVal < (*pixels)[2 * (i + j*width)] ?
                (*pixels)[2 * (i + j*width)] : strokeVal;
        }
    }

    return true;
}
