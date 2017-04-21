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
	mFace = NULL;
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
}

/*!
Destructor cleans up all font objects, textures and shaders
*/
sgct_text::FontManager::~FontManager(void)
{
	for (auto& a : mFontMap)
	{
		for (auto& b : a.second)
		{
			b.second->clean();
			delete b.second;
		}
		a.second.clear();
	}
	mFontMap.clear();

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

std::size_t sgct_text::FontManager::getTotalNumberOfLoadedChars()
{
	std::size_t counter = 0;
	for (auto& a : mFontMap)
		for (auto& b : a.second)
			counter += b.second->getNumberOfLoadedChars();
	return counter;
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
sgct_text::Font * sgct_text::FontManager::getFont( const std::string & fontName, unsigned int height )
{
	if(mFontMap[fontName].count(height) == 0)
		mFontMap[fontName][height] = createFont(fontName, height);
		
	return mFontMap[fontName][height];
}

/*!
Get the SGCT default font face that is loaded into memory.
@param    height    Height in  pixels for the font
@return    Pointer to the font face, NULL if not found
*/
sgct_text::Font * sgct_text::FontManager::getDefaultFont( unsigned int height )
{
    return getFont("SGCTFont", height);
}

/*!
Creates font textures with a specific height if a path to the font exists
@param    fontName    Name of the font
@param    height        Height of the font in pixels
@return    Iterator to the newly created font, end of the Fonts container if something went wrong
*/
sgct_text::Font * sgct_text::FontManager::createFont( const std::string & fontName, unsigned int height )
{
	std::map<std::string, std::string>::iterator it = mFontPaths.find( fontName );

	if( it == mFontPaths.end() )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: No font file specified for font [%s].\n", fontName.c_str() );
		return mFontMap.end()->second.end()->second;
	}

	if( mFTLibrary == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: Freetype library is not initialized, can't create font [%s].\n", fontName.c_str() );
		return mFontMap.end()->second.end()->second;
	}

	FT_Error error = FT_New_Face( mFTLibrary, it->second.c_str(), 0, &mFace);

	if ( error == FT_Err_Unknown_File_Format )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: Unsopperted file format [%s] for font [%s].\n", it->second.c_str(), fontName.c_str() );
		return mFontMap.end()->second.end()->second;
	}
	else if( error != 0 || mFace == NULL )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: Font '%s' not found!\n", it->second.c_str());
		return mFontMap.end()->second.end()->second;
	}

	if( FT_Set_Char_Size(mFace, height << 6, height << 6, 96, 96) != 0 )
	{
		sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_ERROR, "FontManager: Could not set pixel size for font[%s].\n", fontName.c_str() );
		return mFontMap.end()->second.end()->second;
	}

	// Create the font when all error tests are done
	Font * newFont = new Font();
	newFont->init( mFTLibrary, mFace, fontName, height );

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

	//sgct::MessageHandler::instance()->print(sgct::MessageHandler::NOTIFY_INFO, "Number of textures loaded: %u\n", newFont.getNumberOfTextures());

	mFontMap[fontName][height] = newFont;
	return newFont;
}
