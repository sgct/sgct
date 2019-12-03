#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/joystick.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>

namespace {
    const char* joyStick1Name = nullptr;
} // namespace

using namespace sgct;

void draw2DFun(RenderData data) {
#ifdef SGCT_HAS_TEXT
    if (joyStick1Name) {
        int numberOfAxes = 0;
        const float* pos = Engine::getJoystickAxes(Joystick::Joystick1, &numberOfAxes);
        std::string joystickInfoStr = "Axes: ";

        for (int i = 0; i < numberOfAxes; i++) {
            joystickInfoStr += std::to_string(pos[i]) + ' ';
        }

        int numberOfButtons = 0;
        const unsigned char* buttons = Engine::getJoystickButtons(
            Joystick::Joystick1,
            &numberOfButtons
        );

        joystickInfoStr += "\nButtons: ";
        for (int i = 0; i < numberOfButtons; i++) {
            joystickInfoStr += std::to_string(buttons[i]) + ' ';
        }

        text::print(
            data.window,
            *text::FontManager::instance().getFont("SGCTFont", 12),
            sgct::text::TextAlignMode::TopLeft,
            18,
            32,
            glm::vec4(1.f, 0.5f, 0.f, 1.f),
            "%s", joystickInfoStr.c_str()
        );
    }
#endif // SGCT_HAS_TEXT
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);

    Engine::Callbacks callbacks;
    callbacks.draw2D = draw2DFun;

    try {
        Engine::create(cluster, callbacks, config);
    }
    catch (const std::runtime_error& e) {
        Logger::Error("%s", e.what());
        Engine::destroy();
        return EXIT_FAILURE;
    }

    joyStick1Name = Engine::getJoystickName(Joystick::Joystick1);
    if (joyStick1Name) {
        Logger::Info("Joystick 1 '%s' is present", joyStick1Name);

        int numberOfAxes = 0;
        Engine::getJoystickAxes(Joystick::Joystick1, &numberOfAxes);

        int numberOfButtons = 0;
        Engine::getJoystickButtons(Joystick::Joystick1, &numberOfButtons);

        Logger::Info(
            "Number of axes %d\nNumber of buttons %d", numberOfAxes, numberOfButtons
        );
    }

    Engine::instance().render();
    Engine::destroy();
    exit(EXIT_SUCCESS);
}
