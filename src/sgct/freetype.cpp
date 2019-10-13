/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2019                                                               *
 * For conditions of distribution and use, see copyright notice in sgct.h                *
 ****************************************************************************************/

#ifdef SGCT_HAS_TEXT

#include <sgct/freetype.h>

#include <sgct/engine.h>
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/window.h>
#include <sgct/helpers/portedfunctions.h>
#include <sgct/helpers/stringfunctions.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <sstream>

namespace {

void setupViewport() {
    sgct::Window& cWin = sgct::Engine::instance()->getCurrentWindow();

    glm::ivec2 position = glm::ivec2(
        cWin.getCurrentViewport()->getPosition() *
        glm::vec2(cWin.getFramebufferResolution())
    );
    glm::ivec2 size = glm::ivec2(
        cWin.getCurrentViewport()->getSize() * glm::vec2(cWin.getFramebufferResolution())
    );

    sgct::Window::StereoMode sm = cWin.getStereoMode();
    if (sm >= sgct::Window::StereoMode::SideBySide) {
        if (sgct::Engine::instance()->getCurrentFrustumMode() ==
            sgct::core::Frustum::Mode::StereoLeftEye)
        {
            switch (sm) {
                case sgct::Window::StereoMode::SideBySide:
                    position.x /= 2;
                    size.x /= 2;
                    break;
                case sgct::Window::StereoMode::SideBySideInverted:
                    position.x = (position.x / 2) + (size.x / 2);
                    size.x /= 2;
                    break;
                case sgct::Window::StereoMode::TopBottom:
                    position.y = (position.y / 2) + (size.y / 2);
                    size.y /= 2;
                    break;
                case sgct::Window::StereoMode::TopBottomInverted:
                    position.y /= 2;
                    size.y /= 2;
                    break;
                default:
                    break;
            }
        }
        else {
            switch (sm) {
                case sgct::Window::StereoMode::SideBySide:
                    position.x = (position.x / 2) + (size.x / 2);
                    size.x /= 2;
                    break;
                case sgct::Window::StereoMode::SideBySideInverted:
                    position.x /= 2;
                    size.x /= 2;
                    break;
                case sgct::Window::StereoMode::TopBottom:
                    position.y /= 2;
                    size.y /= 2;
                    break;
                case sgct::Window::StereoMode::TopBottomInverted:
                    position.y = (position.y / 2) + (size.y / 2);
                    size.y /= 2;
                    break;
                default:
                    break;
            }
        }
    }

    glViewport(position.x, position.y, size.x, size.y);
}

glm::mat4 setupOrthoMat() {
    glm::mat4 orthoMat;
    sgct::Window& win = sgct::Engine::instance()->getCurrentWindow();

    glm::ivec2 res = win.getResolution();
    glm::vec2 size = win.getCurrentViewport()->getSize();
    glm::vec2 scale = win.getScale();
    orthoMat = glm::ortho(
        0.f,
        size.x * static_cast<float>(res.x) * scale.x,
        0.f,
        size.y * static_cast<float>(res.y) * scale.y
    );

    return orthoMat;
}

void pushScreenCoordinateMatrix() {
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    setupViewport();
    glLoadMatrixf(glm::value_ptr(setupOrthoMat()));
}

/// Pops the projection matrix without changing the current
/// MatrixMode.
void pop_projection_matrix() {
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
}

std::vector<char> parseArgList(va_list args, const char* format) {
    int size = 1 + vscprintf(format, args);
    std::vector<char> buffer(size, 0);

#if (_MSC_VER >= 1400) //visual studio 2005 or later
    vsprintf_s(buffer.data(), size, format, args);
#else
    vsprintf(buffer.data(), format, args);
#endif

    return buffer;
}

std::vector<wchar_t> parseArgList(va_list args, const wchar_t* format) {
    int size = 1 + vscwprintf(format, args);
    std::vector<wchar_t> buffer(size, 0);

#if defined(_WIN32)
#if (_MSC_VER >= 1400) //visual studio 2005 or later
    vswprintf_s(buffer.data(), size, format, args);
#else
    vswprintf(buffer.data(), size, format, args);
#endif
#else
    vswprintf(buffer.data(), 1024, format, args);
#endif

    return buffer;
}

std::vector<std::wstring> split(std::wstring str, wchar_t delimiter) {
    std::vector<std::wstring> tmpVec;
    std::wstringstream ss(std::move(str));
    std::wstring part;

    while (getline(ss, part, delimiter)) {
        tmpVec.push_back(part);
    }

    return tmpVec;
}

std::vector<std::wstring> split(std::string str, wchar_t delimiter) {
    std::vector<std::wstring> tmpVec;
    std::wstring ws;
    ws.assign(str.begin(), str.end());

    std::wstringstream ss(ws);
    std::wstring part;

    while (getline(ss, part, delimiter)) {
        tmpVec.push_back(part);
    }

    return tmpVec;
}


float getLineWidth(sgct::text::Font& font, const std::wstring& line) {
    // figure out width
    float lineWidth = 0.f;
    for (size_t j = 0; j < line.length() - 1; ++j) {
        wchar_t c = line.c_str()[j];
        const sgct::text::Font::FontFaceData& ffd = font.getFontFaceData(c);
        lineWidth += ffd.distToNextChar;
    }
    // add last char width
    wchar_t c = line.c_str()[line.length() - 1];
    const sgct::text::Font::FontFaceData& ffd = font.getFontFaceData(c);
    lineWidth += ffd.size.x;

    return lineWidth;
}

void render2d(const std::vector<std::wstring>& lines, sgct::text::Font& font,
              const sgct::text::TextAlignMode& mode, float x, float y,
              const glm::vec4& color)
{
    using namespace sgct::text;

    const float h = font.getHeight() * 1.59f;

    setupViewport();
    const glm::mat4 projectionMat(setupOrthoMat());

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    FontManager::instance()->getShader().bind();

    glBindVertexArray(font.getVAO());
    glActiveTexture(GL_TEXTURE0);

    glUniform4fv(
        FontManager::instance()->getColorLocation(),
        1,
        glm::value_ptr(color)
    );
    const glm::vec4 stroke = FontManager::instance()->getStrokeColor();
    glUniform4fv(
        FontManager::instance()->getStrokeLocation(),
        1,
        glm::value_ptr(stroke)
    );

    for (size_t i = 0; i < lines.size(); i++) {
        glm::vec3 offset(x, y - h * i, 0.f);

        if (mode == TextAlignMode::TopCenter) {
            offset.x -= getLineWidth(font, lines[i]) / 2.f;
        }
        else if (mode == TextAlignMode::TopRight) {
            offset.x -= getLineWidth(font, lines[i]);
        }

        for (size_t j = 0; j < lines[i].length(); j++) {
            const wchar_t c = lines[i].c_str()[j];
            const sgct::text::Font::FontFaceData& ffd = font.getFontFaceData(c);

            glm::mat4 trans = glm::translate(
                projectionMat,
                glm::vec3(offset.x + ffd.pos.x, offset.y + ffd.pos.y, offset.z)
            );
            glm::mat4 scale = glm::scale(
                trans,
                glm::vec3(ffd.size.x, ffd.size.y, 1.f)
            );

            glBindTexture(GL_TEXTURE_2D, ffd.texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glUniform1i(FontManager::instance()->getTextureLoc(), 0);

            glUniformMatrix4fv(
                FontManager::instance()->getMVPLocation(),
                1,
                GL_FALSE,
                glm::value_ptr(scale)
            );

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            offset += glm::vec3(ffd.distToNextChar, 0.f, 0.f);
        }
    }

    glBindVertexArray(0);
    sgct::ShaderProgram::unbind();
}

void render3d(const std::vector<std::wstring>& lines, sgct::text::Font& font,
              const sgct::text::TextAlignMode& mode, const glm::mat4& mvp,
              const glm::vec4& color)
{
    using namespace sgct::text;

    const float h = font.getHeight() * 1.59f;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    FontManager::instance()->getShader().bind();

    glBindVertexArray(font.getVAO());
    glActiveTexture(GL_TEXTURE0);

    glUniform4fv(
        FontManager::instance()->getColorLocation(),
        1,
        glm::value_ptr(color)
    );
    const glm::vec4 stroke = FontManager::instance()->getStrokeColor();
    glUniform4fv(
        FontManager::instance()->getStrokeLocation(),
        1,
        glm::value_ptr(stroke)
    );

    const float textScale = 1.f / font.getHeight();
    const glm::mat4 textScaleMat = glm::scale(mvp, glm::vec3(textScale));

    for (size_t i = 0; i < lines.size(); i++) {
        glm::vec3 offset(0.f, -h * i, 0.f);

        if (mode == TextAlignMode::TopCenter) {
            offset.x -= getLineWidth(font, lines[i]) / 2.f;
        }
        else if (mode == TextAlignMode::TopRight) {
            offset.x -= getLineWidth(font, lines[i]);
        }

        for (size_t j = 0; j < lines[i].length(); j++) {
            const wchar_t c = lines[i].c_str()[j];
            const Font::FontFaceData& ffd = font.getFontFaceData(c);

            const glm::mat4 trans = glm::translate(
                textScaleMat,
                glm::vec3(offset.x + ffd.pos.x, offset.y + ffd.pos.y, offset.z)
            );
            const glm::mat4 scale = glm::scale(
                trans,
                glm::vec3(ffd.size.x, ffd.size.y, 1.f)
            );

            glBindTexture(GL_TEXTURE_2D, ffd.texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

            glUniform1i(FontManager::instance()->getTextureLoc(), 0);
            glUniformMatrix4fv(
                FontManager::instance()->getMVPLocation(),
                1,
                GL_FALSE,
                glm::value_ptr(scale)
            );

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            offset += glm::vec3(ffd.distToNextChar, 0.f, 0.f);
        }
    }

    glBindVertexArray(0);
    sgct::ShaderProgram::unbind();
}

} // namespace

namespace sgct::text {

void print(Font& font, TextAlignMode mode, float x, float y, const char* format, ...) {
    va_list args;
    va_start(args, format);
    std::vector<char> buf = parseArgList(args, format);
    va_end(args);

    if (!buf.empty()) {
        std::vector<std::wstring> lines = split(std::string(buf.data()), L'\n');
        render2d(lines, font, mode, x, y, glm::vec4(1.f));
    }
}

void print(Font& font, TextAlignMode mode, float x, float y, const wchar_t* format, ...) {
    va_list	args;
    va_start(args, format);
    // @TODO (abock, 2019-10-13) This does not seem to work properly on OSX. It results
    // in an empty buffer object
    std::vector<wchar_t> buf = parseArgList(args, format);
    va_end(args);

    if (!buf.empty()) {
        std::vector<std::wstring> lines = split(std::wstring(buf.data()), L'\n');
        render2d(lines, font, mode, x, y, glm::vec4(1.f));
    }
}

void print(Font& font, TextAlignMode mode, float x, float y, const glm::vec4& color,
           const char* format, ...)
{
    va_list	args;
    va_start(args, format);
    std::vector<char> buf = parseArgList(args, format);
    va_end(args);

    if (!buf.empty()) {
        std::vector<std::wstring> lines = split(std::string(buf.data()), L'\n');
        render2d(lines, font, mode, x, y, color);
    }
}

void print(Font& font, TextAlignMode mode, float x, float y, const glm::vec4& color,
           const wchar_t* format, ...)
{
    va_list	args;
    va_start(args, format);
    std::vector<wchar_t> buf = parseArgList(args, format);
    va_end(args);

    if (!buf.empty()) {
        std::vector<std::wstring> lines = split(std::wstring(buf.data()), L'\n');
        render2d(lines, font, mode, x, y, color);
    }
}

void print3d(Font& font, TextAlignMode mode, glm::mat4 mvp, const char* format, ...) {
    va_list	args;
    va_start(args, format);
    std::vector<char> buf = parseArgList(args, format);
    va_end(args);

    if (!buf.empty()) {
        std::vector<std::wstring> lines = split(std::string(buf.data()), L'\n');
        render3d(lines, font, mode, mvp, glm::vec4(1.f));
    }
}

void print3d(Font& font, TextAlignMode mode, glm::mat4 mvp, const wchar_t* format, ...) {
    va_list	args;
    va_start(args, format);
    std::vector<wchar_t> buf = parseArgList(args, format);
    va_end(args);

    if (!buf.empty()) {
        std::vector<std::wstring> lines = split(std::wstring(buf.data()), L'\n');
        render3d(lines, font, mode, mvp, glm::vec4(1.f));
    }
}

void print3d(Font& font, TextAlignMode mode, glm::mat4 mvp, const glm::vec4& color,
             const char* format, ...)
{
    va_list	args;
    va_start(args, format);
    std::vector<char> buf = parseArgList(args, format);
    va_end(args);

    if (!buf.empty()) {
        std::vector<std::wstring> lines = split(std::string(buf.data()), L'\n');
        render3d(lines, font, mode, mvp, color);
    }
}

void print3d(Font& font, TextAlignMode mode, glm::mat4 mvp, const glm::vec4& color,
             const wchar_t* format, ...)
{
    va_list	args;
    va_start(args, format);
    std::vector<wchar_t> buf = parseArgList(args, format);
    va_end(args);

    if (!buf.empty()) {
        std::vector<std::wstring> lines = split(std::wstring(buf.data()), L'\n');
        render3d(lines, font, mode, mvp, color);
    }  
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
