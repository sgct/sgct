/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/error.h>
#include <sgct/profiling.h>
#include <sgct/sgct.h>
#include <sgct/vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <format>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace vk = sgct::vulkan;

namespace {
    // Grid vertex: elevation and azimuth in radians
    struct GridVertex {
        float elevation;
        float azimuth;
    };

    // Box vertex: position, texture coordinates, and the id of the face texture
    struct BoxVertex {
        float x;
        float y;
        float z;

        float s;
        float t;

        uint8_t textureId;
    };

    // Uniform buffer contents; the layout must match the shader UBO declarations
    struct GridUniforms {
        glm::mat4 mvp;
        glm::mat4 camera;
        float radius;
    };

    struct BoxUniforms {
        glm::mat4 mvp;
        glm::mat4 camera;
    };

    // Vulkan resources for the wireframe grid
    struct GridResources {
        std::unique_ptr<vk::Buffer> vertexBuffer;
        std::unique_ptr<vk::Buffer> indexBuffer;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        uint32_t indexCount = 0;
    };

    // Vulkan resources for the textured box
    struct BoxResources {
        std::unique_ptr<vk::Buffer> vertexBuffer;
        std::array<std::unique_ptr<vk::Texture>, 6> textures;
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        uint32_t vertexCount = 36;
    };

    GridResources grid;
    BoxResources box;
    // The draw callback runs once per viewport and eye into the same command buffer, so
    // every draw gets its own slice of this ring through a dynamic uniform buffer
    // descriptor instead of overwriting a single uniform buffer
    constexpr VkDeviceSize UniformBytesPerFrame = 64 * 1024;
    std::unique_ptr<vk::UniformRing> uniforms;
    // The pipelines for each format of the windows' render targets. All render passes of
    // the same format are compatible, so the pipelines work for all of them
    struct Pipelines {
        std::unique_ptr<vk::Pipeline> grid;
        std::unique_ptr<vk::Pipeline> box;
    };
    std::map<vk::SceneFormat, Pipelines> pipelines;
    bool resourcesCreated = false;

    bool runTests = false;
    uint64_t frameNumber = 0;

    float phi = glm::pi<float>();
    float theta = 0.f;

    bool takeScreenshot = false;
    bool captureBackbuffer = false;
    bool renderGrid = false;
    bool renderBox = true;
    bool showId = false;
    bool showStats = false;

    float radius = 7.4f;

    constexpr std::string_view GridVertexShader = R"(
#version 460

layout(location = 0) in vec2 in_position;

layout(set = 0, binding = 0) uniform GridUniforms {
    mat4 mvp;
    mat4 camera;
    float radius;
} u;

layout(location = 0) out vec4 out_color;

void main() {
    float elevation = in_position.x;
    float azimuth = in_position.y;

    vec3 p = vec3(
        u.radius * cos(elevation) * sin(azimuth),
        u.radius * sin(elevation),
        -u.radius * cos(elevation) * cos(azimuth)
    );
    gl_Position = u.mvp * u.camera * vec4(p, 1.0);
    out_color = vec4(p, 1.0);
}
)";

    constexpr std::string_view GridFragmentShader = R"(
#version 460

layout(location = 0) in vec4 in_color;
layout(location = 0) out vec4 out_color;

void main() { out_color = in_color; }
)";

    constexpr std::string_view BoxVertexShader = R"(
#version 460

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texCoords;
layout(location = 2) in uint in_textureId;

layout(set = 0, binding = 0) uniform BoxUniforms {
    mat4 mvp;
    mat4 camera;
} u;

layout(location = 0) out vec2 out_texCoords;
layout(location = 1) out flat uint out_textureId;

void main() {
    gl_Position = u.mvp * u.camera * vec4(in_position, 1.0);
    out_texCoords = in_texCoords;
    out_textureId = in_textureId;
}
)";

    constexpr std::string_view BoxFragmentShader = R"(
#version 460

layout(location = 0) in vec2 in_texCoords;
layout(location = 1) in flat uint in_textureId;

layout(set = 0, binding = 1) uniform sampler2D tex[6];

layout(location = 0) out vec4 color;

void main() {
    color = texture(tex[in_textureId], in_texCoords);
}
)";

    // Converts the OpenGL-style NDC depth range [-1, 1] to the Vulkan range [0, 1]
    const glm::mat4 depthScaleBias(
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 0.5f, 0.f,
        0.f, 0.f, 0.5f, 1.f
    );

    glm::mat4 toGlm(const sgct::mat4& m) {
        glm::mat4 result;
        std::memcpy(&result[0][0], m.values.data(), sizeof(glm::mat4));
        return result;
    }

    // sgct::Image stores 3/4-channel pixels in BGR(A) order (see Image::load, which
    // swaps the R and B channels stb_image produces so that the legacy OpenGL path can
    // upload them with GL_BGR/GL_BGRA). Pick the matching Vulkan format so the bytes are
    // interpreted in the same order they were written
    VkFormat formatForChannels(int channels) {
        switch (channels) {
            case 3:
                return VK_FORMAT_B8G8R8_UNORM;
            case 4:
            default:
                return VK_FORMAT_B8G8R8A8_UNORM;
        }
    }

    void check(VkResult result, const char* what) {
        if (result != VK_SUCCESS) {
            // Throw sgct::Error (not a plain runtime_error) so that Window::draw's
            // catch (const Error&) rethrows it instead of misreading it as a
            // "swapchain out of date" condition
            const std::string msg = std::format(
                "Vulkan error ({}): {}", static_cast<int>(result), what);
            sgct::Log::Error(msg);
            throw sgct::Error(sgct::Error::Component::Window, 4600, msg);
        }
    }
} // namespace

using namespace sgct;

void createGridResources(vk::Context& ctx) {
    constexpr uint16_t RestartIndex = std::numeric_limits<uint16_t>::max();

    constexpr int ElevationSteps = 40;
    constexpr int AzimuthSteps = 160;

    std::vector<GridVertex> vertices;
    // (abock, 2019-10-09) We generate the vertices ring-wise; first iterating over the
    // elevation and then the azimuth will lead to the bottom most ring be filled first,
    // before going to the upper rings. That also means that two vertices on top of each
    // other should be separated in the vertices list by 'AzimuthSteps' positions
    for (int e = 0; e <= ElevationSteps; e++) {
        for (int a = 0; a < AzimuthSteps; a++) {
            float ev = static_cast<float>(e) / static_cast<float>(ElevationSteps - 1);
            float av = static_cast<float>(a) / static_cast<float>(AzimuthSteps - 1);

            GridVertex vertex;
            vertex.elevation = glm::radians(ev * 90.f);
            vertex.azimuth = glm::radians(av * 360.f);
            vertices.push_back(vertex);
        }
    }

    // Line representation
    std::vector<uint16_t> indices;
    indices.reserve(2 * ElevationSteps * AzimuthSteps);

    // First the horizontal, azimuth lines
    for (int e = 0; e < ElevationSteps; e++) {
        std::vector<uint16_t> t(AzimuthSteps);
        std::iota(t.begin(), t.end(), static_cast<uint16_t>(e * AzimuthSteps));
        t.push_back(static_cast<uint16_t>(e * AzimuthSteps)); // close the ring
        t.push_back(RestartIndex); // restart for the next ring
        indices.insert(indices.end(), t.begin(), t.end());
    }
    indices.push_back(RestartIndex);
    // Then the vertical lines; every vertical vertex is separated by exactly
    // 'AzimuthSteps' positions in the vertex array
    for (int a = 0; a < AzimuthSteps; a++) {
        for (int e = 0; e < ElevationSteps; e++) {
            indices.push_back(static_cast<uint16_t>(a + e * AzimuthSteps));
        }
        indices.push_back(RestartIndex);
    }

    grid.indexCount = static_cast<uint32_t>(indices.size());

    grid.vertexBuffer = std::make_unique<vk::Buffer>(
        ctx, vertices.size() * sizeof(GridVertex), vk::Buffer::Type::Vertex);
    grid.vertexBuffer->upload(vertices.data());

    grid.indexBuffer = std::make_unique<vk::Buffer>(
        ctx, indices.size() * sizeof(uint16_t), vk::Buffer::Type::Index);
    grid.indexBuffer->upload(indices.data());

    // Descriptor set layout: a single uniform buffer in the uniform ring
    VkDescriptorSetLayoutBinding binding = {};
    binding.binding = 0;
    binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    binding.descriptorCount = 1;
    binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo dslInfo = {};
    dslInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslInfo.bindingCount = 1;
    dslInfo.pBindings = &binding;
    check(
        vkCreateDescriptorSetLayout(ctx.device(), &dslInfo, nullptr, &grid.descriptorSetLayout),
        "vkCreateDescriptorSetLayout"
    );

    VkPipelineLayoutCreateInfo plInfo = {};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &grid.descriptorSetLayout;
    check(
        vkCreatePipelineLayout(ctx.device(), &plInfo, nullptr, &grid.pipelineLayout),
        "vkCreatePipelineLayout"
    );

    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSize.descriptorCount = 1;
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    check(
        vkCreateDescriptorPool(ctx.device(), &poolInfo, nullptr, &grid.descriptorPool),
        "vkCreateDescriptorPool"
    );

    VkDescriptorSetLayout layouts[1] = { grid.descriptorSetLayout };
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = grid.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;
    check(vkAllocateDescriptorSets(ctx.device(), &allocInfo, &grid.descriptorSet),
        "vkAllocateDescriptorSets");

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniforms->handle();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(GridUniforms);
    VkWriteDescriptorSet write = {};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = grid.descriptorSet;
    write.dstBinding = 0;
    write.dstArrayElement = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    write.pBufferInfo = &bufferInfo;
    // vkUpdateDescriptorSets returns void in this SDK, so there is no result to check
    vkUpdateDescriptorSets(ctx.device(), 1, &write, 0, nullptr);
}

void createBoxResources(vk::Context& ctx) {
    constexpr std::array<BoxVertex, 2 * 3 * 6> Vertices = {
        // Front
        BoxVertex{ -10.f, -10.f, -10.f, 0.f, 0.f, 0 },  // ---
        BoxVertex{  10.f,  10.f, -10.f, 1.f, 1.f, 0 },  // ++-
        BoxVertex{ -10.f,  10.f, -10.f, 0.f, 1.f, 0 },  // -+-

        BoxVertex{ -10.f, -10.f, -10.f, 0.f, 0.f, 0 },  // ---
        BoxVertex{  10.f, -10.f, -10.f, 1.f, 0.f, 0 },  // +--
        BoxVertex{  10.f,  10.f, -10.f, 1.f, 1.f, 0 },  // ++-

        // Right
        BoxVertex{  10.f, -10.f, -10.f, 0.f, 0.f, 1 },  // +--
        BoxVertex{  10.f,  10.f,  10.f, 1.f, 1.f, 1 },  // +++
        BoxVertex{  10.f,  10.f, -10.f, 0.f, 1.f, 1 },  // ++-

        BoxVertex{  10.f, -10.f, -10.f, 0.f, 0.f, 1 },  // +--
        BoxVertex{  10.f, -10.f,  10.f, 1.f, 0.f, 1 },  // +-+
        BoxVertex{  10.f,  10.f,  10.f, 1.f, 1.f, 1 },  // +++

        // Back
        BoxVertex{  10.f, -10.f,  10.f, 0.f, 0.f, 2 },  // +-+
        BoxVertex{ -10.f,  10.f,  10.f, 1.f, 1.f, 2 },  // -++
        BoxVertex{  10.f,  10.f,  10.f, 0.f, 1.f, 2 },  // +++

        BoxVertex{  10.f, -10.f,  10.f, 0.f, 0.f, 2 },  // +-+
        BoxVertex{ -10.f, -10.f,  10.f, 1.f, 0.f, 2 },  // --+
        BoxVertex{ -10.f,  10.f,  10.f, 1.f, 1.f, 2 },  // -++

        // Left
        BoxVertex{ -10.f, -10.f,  10.f, 0.f, 0.f, 3 },  // --+
        BoxVertex{ -10.f,  10.f, -10.f, 1.f, 1.f, 3 },  // -+-
        BoxVertex{ -10.f,  10.f,  10.f, 0.f, 1.f, 3 },  // -++

        BoxVertex{ -10.f, -10.f,  10.f, 0.f, 0.f, 3 },  // --+
        BoxVertex{ -10.f, -10.f, -10.f, 1.f, 0.f, 3 },  // ---
        BoxVertex{ -10.f,  10.f, -10.f, 1.f, 1.f, 3 },  // -+-

        // Top
        BoxVertex{ -10.f,  10.f, -10.f, 0.f, 0.f, 4 },  // -+-
        BoxVertex{  10.f,  10.f,  10.f, 1.f, 1.f, 4 },  // +++
        BoxVertex{ -10.f,  10.f,  10.f, 0.f, 1.f, 4 },  // -++

        BoxVertex{ -10.f,  10.f, -10.f, 0.f, 0.f, 4 },  // -+-
        BoxVertex{  10.f,  10.f, -10.f, 1.f, 0.f, 4 },  // ++-
        BoxVertex{  10.f,  10.f,  10.f, 1.f, 1.f, 4 },  // +++

        // Bottom
        BoxVertex{ -10.f, -10.f,  10.f, 0.f, 0.f, 5 },  // --+
        BoxVertex{  10.f, -10.f, -10.f, 1.f, 1.f, 5 },  // +--
        BoxVertex{ -10.f, -10.f, -10.f, 0.f, 1.f, 5 },  // ---

        BoxVertex{ -10.f, -10.f,  10.f, 0.f, 0.f, 5 },  // --+
        BoxVertex{  10.f, -10.f,  10.f, 1.f, 0.f, 5 },  // +-+
        BoxVertex{  10.f, -10.f, -10.f, 1.f, 1.f, 5 },  // +--
    };

    box.vertexBuffer = std::make_unique<vk::Buffer>(
        ctx, Vertices.size() * sizeof(BoxVertex), vk::Buffer::Type::Vertex);
    box.vertexBuffer->upload(Vertices.data());

    // Load the six face textures
    Log::Info("Loading test pattern images...");
    const char* filenames[6] = {
        "test-pattern-0.png",
        "test-pattern-1.png",
        "test-pattern-2.png",
        "test-pattern-3.png",
        "test-pattern-4.png",
        "test-pattern-5.png"
    };

    for (int i = 0; i < 6; i++) {
        if (!std::filesystem::exists(filenames[i])) {
            Log::Error(std::format("Could not find image '{}'", filenames[i]));
            exit(EXIT_FAILURE);
        }
        sgct::Image img;
        img.load(filenames[i]);
        const sgct::ivec2 size = img.size();
        box.textures[i] = std::make_unique<vk::Texture>(
            ctx,
            static_cast<uint32_t>(size.x),
            static_cast<uint32_t>(size.y),
            formatForChannels(img.channels()),
            img.data(),
            VK_FILTER_LINEAR,
            // The same number of mipmap levels as the OpenGL TextureManager generated
            8
        );
    }

    // Descriptor set layout: a uniform buffer in the uniform ring and an array of six
    // combined image samplers
    VkDescriptorSetLayoutBinding bindings[2] = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[1].descriptorCount = 6;
    bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo dslInfo = {};
    dslInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    dslInfo.bindingCount = 2;
    dslInfo.pBindings = bindings;
    check(
        vkCreateDescriptorSetLayout(ctx.device(), &dslInfo, nullptr, &box.descriptorSetLayout),
        "vkCreateDescriptorSetLayout"
    );

    VkPipelineLayoutCreateInfo plInfo = {};
    plInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plInfo.setLayoutCount = 1;
    plInfo.pSetLayouts = &box.descriptorSetLayout;
    check(
        vkCreatePipelineLayout(ctx.device(), &plInfo, nullptr, &box.pipelineLayout),
        "vkCreatePipelineLayout"
    );

    VkDescriptorPoolSize poolSizes[2] = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    poolSizes[0].descriptorCount = 1;
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = 6;
    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = 2;
    poolInfo.pPoolSizes = poolSizes;
    check(
        vkCreateDescriptorPool(ctx.device(), &poolInfo, nullptr, &box.descriptorPool),
        "vkCreateDescriptorPool"
    );

    VkDescriptorSetLayout layouts[1] = { box.descriptorSetLayout };
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = box.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = layouts;
    check(vkAllocateDescriptorSets(ctx.device(), &allocInfo, &box.descriptorSet),
        "vkAllocateDescriptorSets");

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = uniforms->handle();
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(BoxUniforms);

    VkDescriptorImageInfo imageInfos[6] = {};
    for (int i = 0; i < 6; i++) {
        imageInfos[i].sampler = box.textures[i]->sampler();
        imageInfos[i].imageView = box.textures[i]->imageView();
        imageInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet writes[2] = {};
    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].dstSet = box.descriptorSet;
    writes[0].dstBinding = 0;
    writes[0].dstArrayElement = 0;
    writes[0].descriptorCount = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    writes[0].pBufferInfo = &bufferInfo;

    writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[1].dstSet = box.descriptorSet;
    writes[1].dstBinding = 1;
    writes[1].dstArrayElement = 0;
    writes[1].descriptorCount = 6;
    writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    writes[1].pImageInfo = imageInfos;

    // vkUpdateDescriptorSets returns void in this SDK, so there is no result to check
    vkUpdateDescriptorSets(ctx.device(), 2, writes, 0, nullptr);
}

Pipelines createPipelines(vk::Context& ctx, VkRenderPass renderPass,
                          const vk::SceneFormat& format)
{
    Pipelines result;

    {
        vk::Pipeline::Settings settings;
        settings.renderPass = renderPass;
        settings.setSceneFormat(format);
        settings.layout = grid.pipelineLayout;
        settings.topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        settings.depthTest = true;
        settings.depthWrite = true;
        settings.depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkVertexInputBindingDescription binding = {};
        binding.binding = 0;
        binding.stride = sizeof(GridVertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        settings.vertexBindings.push_back(binding);

        VkVertexInputAttributeDescription attr = {};
        attr.location = 0;
        attr.binding = 0;
        attr.format = VK_FORMAT_R32G32_SFLOAT;
        attr.offset = 0;
        settings.vertexAttributes.push_back(attr);

        result.grid = std::make_unique<vk::Pipeline>(
            ctx, GridVertexShader.data(), GridFragmentShader.data(), settings);
    }

    {
        vk::Pipeline::Settings settings;
        settings.renderPass = renderPass;
        settings.setSceneFormat(format);
        settings.layout = box.pipelineLayout;
        settings.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        settings.depthTest = true;
        settings.depthWrite = true;
        settings.depthCompare = VK_COMPARE_OP_LESS_OR_EQUAL;

        VkVertexInputBindingDescription binding = {};
        binding.binding = 0;
        binding.stride = sizeof(BoxVertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        settings.vertexBindings.push_back(binding);

        VkVertexInputAttributeDescription attrs[3] = {};
        attrs[0].location = 0;
        attrs[0].binding = 0;
        attrs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attrs[0].offset = 0;

        attrs[1].location = 1;
        attrs[1].binding = 0;
        attrs[1].format = VK_FORMAT_R32G32_SFLOAT;
        attrs[1].offset = offsetof(BoxVertex, s);

        attrs[2].location = 2;
        attrs[2].binding = 0;
        attrs[2].format = VK_FORMAT_R8_UINT;
        attrs[2].offset = offsetof(BoxVertex, textureId);

        settings.vertexAttributes.insert(
            settings.vertexAttributes.end(), attrs, attrs + 3);

        result.box = std::make_unique<vk::Pipeline>(
            ctx, BoxVertexShader.data(), BoxFragmentShader.data(), settings);
    }
    return result;
}

const Pipelines& ensureResources(const Window& window) {
    vk::Context& ctx = vk::Context::instance();
    if (!resourcesCreated) {
        uniforms = std::make_unique<vk::UniformRing>(ctx, UniformBytesPerFrame);
        createGridResources(ctx);
        createBoxResources(ctx);
        resourcesCreated = true;
    }

    Pipelines& p = pipelines[window.sceneFormat()];
    if (!p.grid) {
        p = createPipelines(ctx, window.renderPass(), window.sceneFormat());
    }
    return p;
}

void postSyncPreDraw() {
    Engine::instance().setCaptureFromBackBuffer(captureBackbuffer);
    Engine::instance().setStatsGraphVisibility(showStats);
    if (takeScreenshot) {
        Log::Info("Triggering screenshot");
        Engine::instance().takeScreenshot();
        takeScreenshot = false;
    }
}

void draw(const RenderData& data) {
    if (data.commandBuffer == VK_NULL_HANDLE) {
        return;
    }

    const Pipelines& p = ensureResources(data.window);

    VkCommandBuffer cmd = data.commandBuffer;

    // Dynamic viewport and scissor. The window provides the area of the current viewport,
    // with the viewport's y-axis flipped so that the OpenGL-style SGCT projection (y=+1 is
    // up) produces an upright image, matching the original OpenGL rendering
    vkCmdSetViewport(cmd, 0, 1, &data.vkViewport);
    vkCmdSetScissor(cmd, 0, 1, &data.vkScissor);

    // Camera matrices
    const glm::mat4 mvp = depthScaleBias * toGlm(data.modelViewProjectionMatrix);

    const glm::vec3 direction = {
        std::cos(theta) * std::sin(phi),
        std::sin(theta),
        std::cos(theta) * std::cos(phi)
    };
    const glm::vec3 right = {
        std::sin(phi - glm::half_pi<float>()),
        0.f,
        std::cos(phi - glm::half_pi<float>())
    };
    const glm::vec3 up = glm::cross(right, direction);
    const glm::mat4 camera = glm::lookAt(glm::vec3(0.f), direction, up);

    if (renderGrid) {
        GridUniforms u;
        u.mvp = mvp;
        u.camera = camera;
        u.radius = radius;
        const uint32_t uniformOffset = uniforms->push(data.frameNumber, u);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p.grid->handle());
        const VkBuffer vertexBuffer = grid.vertexBuffer->handle();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
        vkCmdBindIndexBuffer(cmd, grid.indexBuffer->handle(), 0, VK_INDEX_TYPE_UINT16);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p.grid->layout(), 0, 1,
            &grid.descriptorSet, 1, &uniformOffset);
        vkCmdDrawIndexed(cmd, grid.indexCount, 1, 0, 0, 0);
    }

    if (renderBox) {
        BoxUniforms u;
        u.mvp = mvp;
        u.camera = camera;
        const uint32_t uniformOffset = uniforms->push(data.frameNumber, u);

        vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p.box->handle());
        const VkBuffer vertexBuffer = box.vertexBuffer->handle();
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(cmd, 0, 1, &vertexBuffer, &offset);
        vkCmdBindDescriptorSets(
            cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, p.box->layout(), 0, 1,
            &box.descriptorSet, 1, &uniformOffset);
        vkCmdDraw(cmd, box.vertexCount, 1, 0, 0);
    }
}

void draw2D(const RenderData& data) {
#ifdef SGCT_HAS_TEXT
    if (showId) {
        const float w =
            static_cast<float>(data.window.windowSize().x) * data.viewport.size().x;
        const float h =
            static_cast<float>(data.window.windowSize().y) * data.viewport.size().y;

        const float offset = w / 2.f - w / 7.f;

        const float s1 = h / 8.f;
        const unsigned int fontSize1 = static_cast<unsigned int>(s1);
        text::Font* f1 = text::FontManager::instance().font("SGCTFont", fontSize1);

        text::print(
            data.window,
            data.viewport,
            *f1,
            text::Alignment::TopLeft,
            offset,
            h / 2.f - s1,
            vec4{ 0.f, 0.f, 1.f, 1.f },
            std::to_string(ClusterManager::instance().thisNodeId())
        );

        const float s2 = h / 20.f;
        const unsigned int fontSize2 = static_cast<unsigned int>(s2);
        text::Font* f2 = text::FontManager::instance().font("SGCTFont", fontSize2);
        text::print(
            data.window,
            data.viewport,
            *f2,
            text::Alignment::TopLeft,
            offset,
            h / 2.f - (s1 + s2) * 1.2f,
            vec4{ 0.f, 0.f, 1.f, 1.f },
            ClusterManager::instance().thisNode().address()
        );
    }
#endif // SGCT_HAS_TEXT
}

void postDraw() {
    if (runTests) {
        frameNumber++;
        Log::Info(std::format("Frame: {}", frameNumber));
    }

    if (Engine::instance().isMaster() && runTests) {
        constexpr int FrameGridFirstScreenshot = 5;
        constexpr int FrameGridSettingBackBuffer = 10;
        constexpr int FrameGridSecondScreenshot = 15;
        constexpr int ChangeToBox = 20;
        constexpr int FrameBoxFirstScreenshot = 25;
        constexpr int FrameBoxSettingBackBuffer = 30;
        constexpr int FrameBoxSecondScreenshot = 35;
        constexpr int FrameTerminate = 40;

        switch (frameNumber) {
            case FrameGridFirstScreenshot:
                Log::Info("Setting to take first grid screenshot");
                takeScreenshot = true;
                break;
            case FrameGridFirstScreenshot + 2:
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case FrameGridSettingBackBuffer:
                Log::Info("Setting to capture from Back buffer");
                captureBackbuffer = true;
                break;
            case FrameGridSecondScreenshot:
                Log::Info("Setting to take second grid screenshot");
                takeScreenshot = true;
                break;
            case FrameGridSecondScreenshot + 2:
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case ChangeToBox:
                Log::Info("Setting to capture from front buffer");
                Log::Info("Changing the rendering to the test box");
                captureBackbuffer = false;
                renderGrid = false;
                renderBox = true;
                break;
            case FrameBoxFirstScreenshot:
                Log::Info("Setting to take first box screenshot");
                takeScreenshot = true;
                break;
            case FrameBoxFirstScreenshot + 2:
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case FrameBoxSettingBackBuffer:
                Log::Info("Setting to capture from Back buffer");
                captureBackbuffer = true;
                break;
            case FrameBoxSecondScreenshot:
                Log::Info("Setting to take second box screenshot");
                takeScreenshot = true;
                break;
            case FrameBoxSecondScreenshot + 2:
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                break;
            case FrameTerminate:
                std::this_thread::sleep_for(std::chrono::seconds(5));
                Engine::instance().terminate();
                break;
            default:
                break;
        }
    }
}

std::vector<std::byte> encode() {
    std::vector<std::byte> data;
    serializeObject(data, takeScreenshot);
    serializeObject(data, captureBackbuffer);
    serializeObject(data, renderGrid);
    serializeObject(data, renderBox);
    serializeObject(data, runTests);
    serializeObject(data, frameNumber);
    serializeObject(data, showId);
    serializeObject(data, showStats);
    serializeObject(data, theta);
    serializeObject(data, phi);
    return data;
}

void decode(const std::vector<std::byte>& data) {
    unsigned pos = 0;
    deserializeObject(data, pos, takeScreenshot);
    deserializeObject(data, pos, captureBackbuffer);
    deserializeObject(data, pos, renderGrid);
    deserializeObject(data, pos, renderBox);
    deserializeObject(data, pos, runTests);
    deserializeObject(data, pos, frameNumber);
    deserializeObject(data, pos, showId);
    deserializeObject(data, pos, showStats);
    deserializeObject(data, pos, theta);
    deserializeObject(data, pos, phi);
}

void cleanup() {
    if (!resourcesCreated) {
        return;
    }

    vk::Context& ctx = vk::Context::instance();
    VkDevice device = ctx.device();

    // Ensure the GPU is done with all resources before destroying them
    vkDeviceWaitIdle(device);

    // Destroy the pipelines first (they reference the layouts)
    pipelines.clear();

    // Destroy the grid resources
    if (grid.descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, grid.descriptorPool, nullptr);
        grid.descriptorPool = VK_NULL_HANDLE;
    }
    if (grid.pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, grid.pipelineLayout, nullptr);
        grid.pipelineLayout = VK_NULL_HANDLE;
    }
    if (grid.descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, grid.descriptorSetLayout, nullptr);
        grid.descriptorSetLayout = VK_NULL_HANDLE;
    }
    grid.vertexBuffer.reset();
    grid.indexBuffer.reset();

    // Destroy the box resources
    if (box.descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, box.descriptorPool, nullptr);
        box.descriptorPool = VK_NULL_HANDLE;
    }
    if (box.pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, box.pipelineLayout, nullptr);
        box.pipelineLayout = VK_NULL_HANDLE;
    }
    if (box.descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, box.descriptorSetLayout, nullptr);
        box.descriptorSetLayout = VK_NULL_HANDLE;
    }
    box.vertexBuffer.reset();
    box.textures = {};
    uniforms.reset();

    resourcesCreated = false;
}

void keyboard(Key key, Modifier, Action action, int, Window*) {
    if (key == Key::Space && action == Action::Press) {
        renderGrid = !renderGrid;
        renderBox = !renderBox;
    }

    if (key == Key::Enter && action == Action::Press) {
        frameNumber = 0;
        runTests = true;
    }

    if (key == Key::Esc && action == Action::Press) {
        Engine::instance().terminate();
    }

    if (key == Key::I && action == Action::Press) {
        showId = !showId;
    }

    if (key == Key::S && action == Action::Press) {
        showStats = !showStats;
    }

    if (key == Key::P && action == Action::Press) {
        takeScreenshot = true;
    }

    if (key == Key::B && action == Action::Press) {
        captureBackbuffer = !captureBackbuffer;
    }

    if (key == Key::Left && (action == Action::Press || action == Action::Repeat)) {
        phi += 0.1f;
        if (phi > glm::two_pi<float>()) {
            phi -= glm::two_pi<float>();
        }
    }

    if (key == Key::Right && (action == Action::Press || action == Action::Repeat)) {
        phi -= 0.1f;
        if (phi < -glm::two_pi<float>()) {
            phi += glm::two_pi<float>();
        }
    }

    if (key == Key::Down && (action == Action::Press || action == Action::Repeat)) {
        theta -= 0.1f;
        theta = std::clamp(theta, -glm::half_pi<float>(), glm::half_pi<float>());
    }

    if (key == Key::Up && (action == Action::Press || action == Action::Repeat)) {
        theta += 0.1f;
        theta = std::clamp(theta, -glm::half_pi<float>(), glm::half_pi<float>());
    }
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    if (!cluster.success) {
        return -1;
    }

    runTests = std::find(arg.begin(), arg.end(), "-runTests") != arg.end();

    const Engine::Callbacks callbacks = {
        .postSyncPreDraw = postSyncPreDraw,
        .draw = draw,
        .draw2D = draw2D,
        .postDraw = postDraw,
        .cleanup = cleanup,
        .encode = encode,
        .decode = decode,
        .keyboard = keyboard
    };

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::exception& e) {
        Log::Error(e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Log::Info("===========");
    Log::Info("Keybindings");
    Log::Info("SPACE: Toggle between grid and box");
    Log::Info("LEFT:  Move camera pointing to the left");
    Log::Info("RIGHT: Move camera pointing to the right");
    Log::Info("UP:    Move camera pointing to up");
    Log::Info("DOWN:  Move camera pointing to down");
    Log::Info("Enter: Run Frontbuffer/Backbuffer tests");
    Log::Info("ESC:   Terminate the program");
    Log::Info("I:     Show node id and IP");
    Log::Info("S:     Show statistics graphs");
    Log::Info("P:     Take screenshot");
    Log::Info("B:     Toggle capturing the back buffer");
    Log::Info("===========");

    try {
        Engine::instance().exec();
    }
    catch (const std::exception& e) {
        Log::Error(std::format("Render loop failed: {}", e.what()));
        Engine::destroy();
        std::cout.flush();
        return EXIT_FAILURE;
    }
    // The context is destroyed by Engine::destroy(), so query it before that
    const bool hasValidationErrors = vulkan::Context::instance().hasValidationErrors();
    if (hasValidationErrors) {
        Log::Error("The Vulkan validation layer reported errors during rendering");
    }
    Engine::destroy();
    std::cout.flush();
    return hasValidationErrors ? EXIT_FAILURE : EXIT_SUCCESS;
}
