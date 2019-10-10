#include <sgct/commandline.h>
#include <sgct/engine.h>
#include <sgct/fontmanager.h>
#include <sgct/freetype.h>

namespace {
    sgct::Engine* gEngine;
    const char* joyStick1Name = nullptr;
} // namespace

using namespace sgct;

void myDraw2DFun() {
#ifdef SGCT_HAS_TEXT
    if (joyStick1Name) {
        int numberOfAxes = 0;
        const float* pos = Engine::getJoystickAxes(joystick::Joystick1, &numberOfAxes);
        std::string joystickInfoStr = "Axes: ";

        for (int i = 0; i < numberOfAxes; i++) {
            joystickInfoStr += std::to_string(pos[i]) + ' ';
        }

        int numberOfButtons = 0;
        const unsigned char* buttons = Engine::getJoystickButtons(
            joystick::Joystick1,
            &numberOfButtons
        );

        joystickInfoStr += "\nButtons: ";
        for (int i = 0; i < numberOfButtons; i++) {
            joystickInfoStr += std::to_string(buttons[i]) + ' ';
        }

        sgct::text::print(
            *sgct::text::FontManager::instance()->getFont("SGCTFont", 12),
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
    gEngine = new Engine(config);

    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    joyStick1Name = Engine::getJoystickName(joystick::Joystick1);
    if (joyStick1Name) {
        MessageHandler::instance()->printInfo(
            "Joystick 1 '%s' is present", joyStick1Name
        );

        int numberOfAxes = 0;
        Engine::getJoystickAxes(joystick::Joystick1, &numberOfAxes);

        int numberOfButtons = 0;
        Engine::getJoystickButtons(joystick::Joystick1, &numberOfButtons);

        MessageHandler::instance()->printInfo(
            "Number of axes %d\nNumber of buttons %d", numberOfAxes, numberOfButtons
        );
    }

    gEngine->setDraw2DFunction(myDraw2DFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
