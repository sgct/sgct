# Getting started

## CMake
In order to use SGCT, it has to be included in the `CMakeLists.txt` of your project.  In the majority of cases it is only necessary to:
```cmake
add_subdirectory(<path to sgct>)
target_include_directories(<your application> PUBLIC <path to sgct>/include)
target_link_libraries(<your application> PUBLIC sgct)
```

In that case, SGCT will automatically be included in your project file when generating it.

## Commandline argument
You can find the available commandline arguments by either looking at the `commandline.cpp` file's `helpMessage()` function, or by executing your application with the commandline argument `-help`, which will print this message and immediately terminate the application.  The most important setting that might be useful for the user is the `-config` parameter which accepts a single argument that is the path to an XML configuration file that is loaded and applied.  The file format of the configuration files are described in greated detail [here](configuration-files.md).  If no configuration file is provided, SGCT will create a "cluster" consisting of a single node with a single 1280x720 window.

When running multiple instances of the application on the same machine (if your configuration file is configured to support that), you will also need the `-local n` and `-client` parameters.  As the automatic IP detection cannot determine which of the nodes to use, the `-local n` parameter has to be used to distinguish the nodes, starting with the number 0, using `-local 0` for the node identified by the IP address `127.0.0.1`, `-local 1` for the address `127.0.0.2` and so forth.  Each node that is *not* the server also needs to be started with the `-client` parameter or it will think it is the server and not try to connect to the actual server node.

## First application
Creating applications using SGCT is very simple and requires only a bit of coding.  Here is an example that opens a window and sets its color to white.  Please note that OpenGL only supports OpenGL versions 3.3 and newer, which means that the legacy fixed-pipeline functios will *not* work.

See also the examples in the `src/apps` folder for more examples on how to use SGCT.  These can be included in your project by enabling the `SGCT_EXAMPLES` option in CMake.

```cpp
#include "sgct/sgct.h"

void drawFun(RenderData data) {
    glClearColor(1.f, 1.f, 1.f, 1.f);
    glClear();
}

int main(int argc, char** argv) {
  std::vector<std::string> arg(argv + 1, argv + argc);
  Configuration config = parseArguments(arg);
  config::Cluster cluster = loadCluster(config.configFilename);

  Engine::Callbacks callbacks;
  callbacks.draw = drawFun;

  try {
    Engine::create(cluster, callbacks, config);
  }
  catch (const std::runtime_error& e) {
    Log::Error("%s", e.what());
    Engine::destroy();
    return EXIT_FAILURE;
  }

  Engine::instance().render();
  Engine::destroy();
  return EXIT_SUCCESS;
}
```

