/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#ifdef SGCT_HAS_TEXT

#include <sgct/fontmanager.h>

#include <sgct/error.h>
#include <sgct/format.h>
#include <sgct/log.h>
#include <sgct/vulkan/context.h>
#include <sgct/vulkan/texture.h>
#include <algorithm>
#include <cstring>
#include <sstream>

#ifdef WIN32
#include <Windows.h>
#endif // WIN32

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif // __clang__

#include <freetype/ftglyph.h>

#ifdef __clang__
#pragma clang diagnostic pop
#elif __GNUC__
#pragma GCC diagnostic pop
#endif // __clang__

namespace {
    std::string SystemFontPath;

    void check(VkResult result, const char* what) {
        if (result != VK_SUCCESS) {
            throw sgct::Error(
                sgct::Error::Component::Window,
                4440,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    }

    // The per-glyph parameters. The glyph is drawn as a unit quad that is transformed by
    // the matrix
    struct GlyphConstants {
        sgct::mat4 mvp;
        sgct::vec4 color;
        sgct::vec4 texRect;
    };

    // The unit quad of the glyph as a triangle strip. The top of the quad samples the top
    // row of the glyph in the atlas
    constexpr const char* FontVertShader = R"(
#version 460

layout (push_constant) uniform Constants {
  mat4 mvp;
  vec4 color;
  vec4 texRect;
} pc;

layout (location = 0) out vec2 out_texCoords;

const vec2 Positions[4] = vec2[](
  vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(0.0, 0.0), vec2(1.0, 0.0)
);

void main() {
  vec2 p = Positions[gl_VertexIndex];
  gl_Position = pc.mvp * vec4(p, 0.0, 1.0);
  out_texCoords = mix(pc.texRect.xy, pc.texRect.zw, vec2(p.x, 1.0 - p.y));
})";

    constexpr const char* FontFragShader = R"(
#version 460

layout (push_constant) uniform Constants {
  mat4 mvp;
  vec4 color;
  vec4 texRect;
} pc;

layout (set = 0, binding = 0) uniform sampler2D tex;

layout (location = 0) in vec2 in_texCoords;
layout (location = 0) out vec4 out_color;

const vec4 StrokeCol = vec4(0.0, 0.0, 0.0, 0.9);

void main() {
  vec2 luminanceAlpha = texture(tex, in_texCoords).rg;
  vec4 blend = mix(StrokeCol, pc.color, luminanceAlpha.r);
  out_color = blend * vec4(1.0, 1.0, 1.0, luminanceAlpha.g);
})";
} // namespace

namespace sgct::text {

FontManager* FontManager::_instance = nullptr;

FontManager& FontManager::instance() {
    if (!_instance) {
        _instance = new FontManager;
    }
    return *_instance;
}

void FontManager::destroy() {
    delete _instance;
    _instance = nullptr;
}

FontManager::FontManager() {
    const FT_Error error = FT_Init_FreeType(&_library);

    if (error != 0) {
        Log::Error("Could not initiate Freetype library");
        return;
    }

    // Set default font path
#ifdef WIN32
#ifdef UNICODE
    constexpr int BufferSize = MAX_PATH;
    WCHAR winDir[BufferSize];
    const UINT success = GetWindowsDirectoryW(winDir, BufferSize);
    if (success > 0) {
        std::wstringstream ss;
        ss << winDir << "\\Fonts\\";
        std::wstring wSystemFontPath = ss.str();
        SystemFontPath = std::string(wSystemFontPath.begin(), wSystemFontPath.end());
    }
#else // ^^^^ UNICODE // !UNICODE vvvv
    constexpr int BufferSize = 256;
    char winDir[BufferSize];
    const UINT success = GetWindowsDirectoryA(winDir, BufferSize);
    if (success > 0) {
        std::stringstream ss;
        ss << winDir << "\\Fonts\\";
        std::string wSystemFontPath = ss.str();
        SystemFontPath = std::string(wSystemFontPath.begin(), wSystemFontPath.end());
    }
#endif // UNICODE
#else // ^^^^ WIN32 // !WIN32 vvvv
    SystemFontPath = "/usr/share/fonts/truetype/freefont/";
#endif // WIN32
}

FontManager::~FontManager() {
    // We need to delete all of the fonts before destroying the FreeType library or the
    // destructor of the Font classes will access the library after it has been destroyed
    _fontMap.clear();

    if (_library) {
        FT_Done_FreeType(_library);
    }

    _pipelines.clear();
    if (_pipelineLayout != VK_NULL_HANDLE || _setLayout != VK_NULL_HANDLE) {
        VkDevice device = vulkan::Context::instance().device();
        vkDestroyPipelineLayout(device, _pipelineLayout, nullptr);
        vkDestroyDescriptorSetLayout(device, _setLayout, nullptr);
    }
}

void FontManager::bindPipeline(VkCommandBuffer cmd, VkRenderPass renderPass,
                               const vulkan::SceneFormat& format, const Font& font)
{
    vulkan::Context& ctx = vulkan::Context::instance();

    if (_pipelineLayout == VK_NULL_HANDLE) {
        // The atlas is set through a push descriptor as every font has its own
        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
        VkDescriptorSetLayoutCreateInfo setLayoutInfo = {};
        setLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        setLayoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT;
        setLayoutInfo.bindingCount = 1;
        setLayoutInfo.pBindings = &binding;
        check(
            vkCreateDescriptorSetLayout(ctx.device(), &setLayoutInfo, nullptr, &_setLayout),
            "vkCreateDescriptorSetLayout"
        );

        VkPushConstantRange pushConstants = {};
        pushConstants.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        pushConstants.offset = 0;
        pushConstants.size = sizeof(GlyphConstants);
        VkPipelineLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layoutInfo.setLayoutCount = 1;
        layoutInfo.pSetLayouts = &_setLayout;
        layoutInfo.pushConstantRangeCount = 1;
        layoutInfo.pPushConstantRanges = &pushConstants;
        check(
            vkCreatePipelineLayout(ctx.device(), &layoutInfo, nullptr, &_pipelineLayout),
            "vkCreatePipelineLayout"
        );
    }

    // All render passes with the same format that text is drawn into are compatible, so
    // the pipeline can be shared between them
    std::unique_ptr<vulkan::Pipeline>& pipeline = _pipelines[format];
    if (!pipeline) {
        vulkan::Pipeline::Settings settings;
        settings.renderPass = renderPass;
        settings.setSceneFormat(format);
        settings.layout = _pipelineLayout;
        settings.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        settings.cullMode = VK_CULL_MODE_NONE;
        // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA) of the OpenGL path, which
        // applies to the alpha channel as well
        settings.blend = true;
        settings.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        settings.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        pipeline = std::make_unique<vulkan::Pipeline>(
            ctx, FontVertShader, FontFragShader, settings
        );
    }

    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline->handle());

    VkDescriptorImageInfo image = {};
    image.sampler = font.atlas().sampler();
    image.imageView = font.atlas().imageView();
    image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    vkCmdPushDescriptorSet(
        cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &write
    );
}

void FontManager::drawGlyph(VkCommandBuffer cmd, const mat4& mvp, const vec4& color,
                            const vec4& texRect) const
{
    GlyphConstants constants;
    constants.mvp = mvp;
    constants.color = color;
    constants.texRect = texRect;
    vkCmdPushConstants(
        cmd,
        _pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
        0,
        sizeof(GlyphConstants),
        &constants
    );
    vkCmdDraw(cmd, 4, 1, 0, 0);
}

bool FontManager::addFont(std::string name, std::string file, bool isAbsolutePath) {
    // Perform file exists check
    if (!isAbsolutePath) {
        file = SystemFontPath + file;
    }

    const bool inserted = _fontPaths.insert({ name, std::move(file) }).second;
    if (!inserted) {
        Log::Warning(std::format("Font with name '{}' already exists", name));
    }
    return inserted;
}

Font* FontManager::font(const std::string& name, unsigned int height) {
    if (!_fontMap.contains({ name, height })) {
        std::unique_ptr<Font> f = createFont(name, height);
        if (!f) {
            return nullptr;
        }
        _fontMap[{ name, height }] = std::move(f);
    }

    return _fontMap[{ name, height }].get();
}

std::unique_ptr<Font> FontManager::createFont(const std::string& name, int height) {
    const auto it = _fontPaths.find(name);

    if (it == _fontPaths.end()) {
        Log::Error(std::format("No font file specified for font '{}'", name));
        return nullptr;
    }

    if (_library == nullptr) {
        Log::Error(std::format(
            "Freetype library is not initialized, cannot create font '{}'", name
        ));
        return nullptr;
    }

    FT_Face face = nullptr;
    const FT_Error error = FT_New_Face(_library, it->second.c_str(), 0, &face);

    if (error == FT_Err_Unknown_File_Format) {
        Log::Error(std::format(
            "Unsupported file format '{}' for font '{}'", it->second, name
        ));
        return nullptr;
    }
    else if (error != 0 || face == nullptr) {
        Log::Error(std::format("Font '{}' not found", it->second));
        return nullptr;
    }

    const FT_Error charSizeErr = FT_Set_Char_Size(face, height << 6, height << 6, 96, 96);
    if (charSizeErr != 0) {
        Log::Error(std::format("Could not set pixel size for font '{}'", name));
        return nullptr;
    }

    // Create the font when all error tests are done
    return std::make_unique<Font>(_library, face, height);
}

} // namespace sgct::text

#endif // SGCT_HAS_TEXT
