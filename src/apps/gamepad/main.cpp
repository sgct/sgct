/*****************************************************************************************
 * SGCT                                                                                  *
 * Simple Graphics Cluster Toolkit                                                       *
 *                                                                                       *
 * Copyright (c) 2012-2020                                                               *
 * For conditions of distribution and use, see copyright notice in LICENSE.md            *
 ****************************************************************************************/

#include <sgct/sgct.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

namespace {
    const char* joyStick1Name = nullptr;
} // namespace

using namespace sgct;

void draw2D(const RenderData& data) {
#ifdef SGCT_HAS_TEXT
    if (joyStick1Name) {
        int numberOfAxes = 0;
        const float* pos = glfwGetJoystickAxes(
            static_cast<int>(Joystick::Joystick1),
            &numberOfAxes
        );
        std::string joystickInfoStr = "Axes: ";

        for (int i = 0; i < numberOfAxes; i++) {
            joystickInfoStr += std::to_string(pos[i]) + ' ';
        }

        int numberOfButtons = 0;
        const unsigned char* buttons = glfwGetJoystickButtons(
            static_cast<int>(Joystick::Joystick1),
            &numberOfButtons
        );

        joystickInfoStr += "\nButtons: ";
        for (int i = 0; i < numberOfButtons; i++) {
            joystickInfoStr += std::to_string(buttons[i]) + ' ';
        }

        text::print(
            data.window,
            data.viewport,
            *text::FontManager::instance().font("SGCTFont", 12),
            text::Alignment::TopLeft,
            18,
            32,
            vec4{ 1.f, 0.5f, 0.f, 1.f },
            "%s", joystickInfoStr.c_str()
        );
    }
#endif // SGCT_HAS_TEXT
}

int main(int argc, char** argv) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.draw2D = draw2D;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Log::Error("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    joyStick1Name = glfwGetJoystickName(static_cast<int>(Joystick::Joystick1));
    if (joyStick1Name) {
        Log::Info("Joystick 1 '%s' is present", joyStick1Name);

        int numberOfAxes = 0;
        glfwGetJoystickAxes(static_cast<int>(Joystick::Joystick1), &numberOfAxes);

        int numberOfButtons = 0;
        glfwGetJoystickButtons(static_cast<int>(Joystick::Joystick1), &numberOfButtons);

        Log::Info(
            "Number of axes %d\nNumber of buttons %d", numberOfAxes, numberOfButtons
        );
    }

    Engine::instance().render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
