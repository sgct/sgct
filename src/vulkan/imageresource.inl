/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2026                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/error.h>
#include <format>

namespace sgct::vulkan {

template <typename F>
void submitImmediately(Context& context, F record) {
    auto check = [](VkResult result, const char* what) {
        if (result != VK_SUCCESS) {
            throw Error(
                Error::Component::Window,
                4405,
                std::format("Vulkan error ({}): {}", static_cast<int>(result), what)
            );
        }
    };

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = context.commandPool();
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;
    VkCommandBuffer cmd = VK_NULL_HANDLE;
    check(
        vkAllocateCommandBuffers(context.device(), &allocInfo, &cmd),
        "vkAllocateCommandBuffers"
    );

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    check(vkBeginCommandBuffer(cmd, &beginInfo), "vkBeginCommandBuffer");
    record(cmd);
    check(vkEndCommandBuffer(cmd), "vkEndCommandBuffer");

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &cmd;
    check(vkQueueSubmit(context.queue(), 1, &submitInfo, VK_NULL_HANDLE), "vkQueueSubmit");
    check(vkQueueWaitIdle(context.queue()), "vkQueueWaitIdle");

    vkFreeCommandBuffers(context.device(), context.commandPool(), 1, &cmd);
}

} // namespace sgct::vulkan
