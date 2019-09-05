#include <sgct.h>

namespace {
    sgct::Engine* gEngine;
    const char* joyStick1Name = nullptr;
} // namespace

using namespace sgct;

void myDraw2DFun() {
#if INCLUDE_SGCT_TEXT
    if (joyStick1Name) {
        int numberOfAxes = 0;
        const float* axesPos = Engine::getJoystickAxes(SGCT_JOYSTICK_1, &numberOfAxes);
        std::string joystickInfoStr = "Axes: ";

        for (int i = 0; i < numberOfAxes; i++) {
            joystickInfoStr += std::to_string(axesPos[i]) + ' ';
        }

        int numberOfButtons = 0;
        const unsigned char* buttons = Engine::getJoystickButtons(
            SGCT_JOYSTICK_1,
            &numberOfButtons
        );

        joystickInfoStr += "\nButtons: ";
        for (int i = 0; i < numberOfButtons; i++) {
            joystickInfoStr += std::to_string(buttons[i]) + ' ';
        }

        sgct_text::print(
            sgct_text::FontManager::instance()->getFont("SGCTFont", 12),
            sgct_text::TextAlignMode::TopLeft,
            18,
            32,
            glm::vec4(1.f, 0.5f, 0.f, 1.f),
            "%s", joystickInfoStr.c_str()
        );
    }
#endif
}

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    joyStick1Name = Engine::getJoystickName(SGCT_JOYSTICK_1);
    if (joyStick1Name) {
        MessageHandler::instance()->print("Joystick 1 '%s' is present\n", joyStick1Name);

        int numberOfAxes = 0;
        const float* axesPos = Engine::getJoystickAxes(SGCT_JOYSTICK_1, &numberOfAxes);

        int numberOfButtons = 0;
        const unsigned char* buttons = Engine::getJoystickButtons(
            SGCT_JOYSTICK_1,
            &numberOfButtons
        );

        MessageHandler::instance()->print(
            "Number of axes %d\nNumber of buttons %d\n", numberOfAxes, numberOfButtons
        );
    }

    gEngine->setDraw2DFunction(myDraw2DFun);

    gEngine->render();
    delete gEngine;
    exit(EXIT_SUCCESS);
}
