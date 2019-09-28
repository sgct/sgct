#include <sgct.h>

namespace {
    sgct::Engine* gEngine;
} // namespace

using namespace sgct;

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    gEngine = new Engine(config);

    if (!gEngine->init(Engine::RunMode::Default_Mode, cluster)) {
        delete gEngine;
        return EXIT_FAILURE;
    }

    gEngine->render();
    delete gEngine;
    exit( EXIT_SUCCESS );
}
