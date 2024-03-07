/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2024                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifdef SGCT_HAS_TEXT

#include <sgct/freetype.h>

#include <sgct/engine.h>
#include <sgct/font.h>
#include <sgct/fontmanager.h>
#include <sgct/opengl.h>
#include <sgct/window.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <sstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cstdarg>

namespace {
    glm::mat4 setupOrthoMat(const sgct::Window& win, const sgct::BaseViewport& vp) {
        sgct::vec2 res = sgct::vec2{
            static_cast<float>(win.resolution().x),
            static_cast<float>(win.resolution().y)
        };
        sgct::vec2 size = vp.size();
        sgct::vec2 scale = win.scale();
        return glm::ortho(0.f, size.x * res.x * scale.x, 0.f, size.y * res.y * scale.y);
    }

    std::vector<std::string> split(std::string str, char delimiter) {
        std::stringstream ss(std::move(str));
        std::string part;

        std::vector<std::string> tmpVec;
        while (getline(ss, part, delimiter)) {
            tmpVec.push_back(part);
        }

        return tmpVec;
    }

    float getLineWidth(sgct::text::Font& font, const std::string& line) {
        // figure out width
        float lineWidth = 0.f;
        for (size_t j = 0; j < line.length() - 1; j++) {
            char c = line.c_str()[j];
            const sgct::text::Font::FontFaceData& ffd = font.fontFaceData(c);
            lineWidth += ffd.distToNextChar;
        }
        // add last char width
        char c = line.c_str()[line.length() - 1];
        const sgct::text::Font::FontFaceData& ffd = font.fontFaceData(c);
        lineWidth += ffd.size.x;

        return lineWidth;
    }
} // namespace

namespace sgct::text {

void print(const Window& window, const BaseViewport& viewport, Font& font, Alignment mode,
           float x, float y, const vec4& color, std::string text)
{
    if (text.empty()) {
        return;
    }

    std::vector<std::string> lines = split(std::move(text), '\n');
    glm::mat4 orthoMatrix = setupOrthoMat(window, viewport);

    const float h = font.height() * 1.59f;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(font.vao());
    glActiveTexture(GL_TEXTURE0);

    for (size_t i = 0; i < lines.size(); i++) {
        glm::vec3 offset(x, y - h * i, 0.f);

        if (mode == Alignment::TopCenter) {
            offset.x -= getLineWidth(font, lines[i]) / 2.f;
        }
        else if (mode == Alignment::TopRight) {
            offset.x -= getLineWidth(font, lines[i]);
        }

        for (const char c : lines[i]) {
            const sgct::text::Font::FontFaceData& ffd = font.fontFaceData(c);

            glBindTexture(GL_TEXTURE_2D, ffd.texId);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

            glm::mat4 trans = glm::translate(
                orthoMatrix,
                glm::vec3(offset.x + ffd.pos.x, offset.y + ffd.pos.y, offset.z)
            );
            glm::mat4 scale = glm::scale(trans, glm::vec3(ffd.size.x, ffd.size.y, 1.f));
            sgct::mat4 s;
            std::memcpy(&s, glm::value_ptr(scale), sizeof(sgct::mat4));

            FontManager::instance().bindShader(s, color, 0);

            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            offset += glm::vec3(ffd.distToNextChar, 0.f, 0.f);
        }
    }

    glBindVertexArray(0);
    sgct::ShaderProgram::unbind();
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
