#include <sgct/commandline.h>
#include <sgct/engine.h>

using namespace sgct;

int main(int argc, char* argv[]) {
    std::vector<std::string> arg(argv + 1, argv + argc);
    Configuration config = parseArguments(arg);
    config::Cluster cluster = loadCluster(config.configFilename);
    Engine::create(config);

    if (!Engine::instance()->init(Engine::RunMode::Default_Mode, cluster)) {
        Engine::destroy();
        return EXIT_FAILURE;
    }

    Engine::instance()->render();
    Engine::destroy();
    exit( EXIT_SUCCESS );
}
