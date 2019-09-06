#include <sgct.h>

namespace {
    sgct::Engine* gEngine;
} // namespace

using namespace sgct;

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    gEngine = new Engine(arg);

    if (!gEngine->init()) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit( EXIT_SUCCESS );
}
