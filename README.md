# SGCT - Simple Graphics Cluster Toolkit

![Build Status](http://dev.openspaceproject.com/buildStatus/icon?job=SGCT%2Fsgct%2Fmaster&style=flat-square)
[![GitHub Issues](https://img.shields.io/github/issues/SGCT/sgct.svg)](https://github.com/SGCT/sgct/issues)
[![Average time to resolve an issue](http://isitmaintained.com/badge/resolution/SGCT/sgct.svg)](http://isitmaintained.com/project/SGCT/sgct "Average time to resolve an issue")
[![GitHub Releases](https://img.shields.io/github/release/SGCT/sgct.svg)](https://github.com/SGCT/sgct/releases)
[![GitHub Downloads](https://img.shields.io/github/downloads/SGCT/sgct/total)](https://github.com/SGCT/sgct/releases)

SGCT is a free cross-platform C++ library for developing OpenGL applications that are synchronized across a cluster of image generating computers (IGs).  SGCT is designed to be as simple as possible to use for the developer and targets the use in immersive real-time applications.  SGCT supports a number of output formats, such as virtual reality (VR), planetarium/dome geometries, fisheye projections, and other types of projections.  In all cases, the client code only needs to render its scene using the projection matrices provided by SGCT and the compositing is then handled internally.  SGCT also supports a variety of stereoscopic formats such as active quad buffers, passive side-by-side, passive over-and-under, checkerboard/DLP/pixel interlaced, and different kinds of anaglyphic stereoscopy.  SGCT applications are scalable and use an XML configuration file format in which all IGs and their properties are specified.  With this approach, there is no need for recompilation of an application for different immersive environments and  applications extend naturally to a server-client clustered architecture without recompilation either.

# Terminology
We use the following terminology to talk about the way how SGCT works.  There is a single *Cluster* that consists of 1 or more *Node*s with each node usually corresponding to a single computer.  Each *Node* contains 1 or more *Window*s with each *Window* containing 1 or more *Viewport*s.  Some viewport types, such as Fisheye projections, can contain multiple *Subviewport*s, which are created automatically.  One the *Node*s in the *Cluster* is designated as the *server*, where as the other *Nodes* are *client*s.  The general dataflow in SGCT applications is from the *server* to the *clients*, and **not** vice versa.

Please note that in this nomenclature, even if an application is running only on a single machine, it is still considered a cluster, but only consisting of 1 node that also acts as the server for 0 clients.  As there are no clients, it does not have an impact on the performance, however.  Furthermore, usually there is a 1-to-1 mapping between Nodes and computers, but that does not have to be the case as a single computer can host an arbitrary(*-ish*) number of nodes.

# Index
1. [Documentation](https://sgct.github.io/)
1. [Features](https://sgct.github.io/features.html)
1. [How it works](https://sgct.github.io/how-it-works.html)
1. [Classes](https://sgct.github.io/classes.html)
1. [Getting started](https://sgct.github.io/getting-started.html)
1. [Configuration files](https://sgct.github.io/configuration-files.html)
1. [Error codes](https://sgct.github.io/errors.html)
1. [Doxygen-generated documentation](http://webstaff.itn.liu.se/~alebo68/sgct/doxygen/html/)

# Tutorials
See the [Getting started](https://sgct.github.io/getting-started.html) guide for a walk
through of a minimal SGCT application.

# Building
SGCT resolves its dependencies through [vcpkg](https://vcpkg.io) in manifest mode, so the only prerequisites are a C++23 compiler, CMake 4.0 or newer, and a vcpkg checkout.

1. Point `VCPKG_ROOT` at your vcpkg checkout:
   ```
   # Windows (PowerShell)
   $env:VCPKG_ROOT = "C:\path\to\vcpkg"

   # Linux
   export VCPKG_ROOT=/path/to/vcpkg
   ```
1. Clone the repository:
   ```
   git clone https://github.com/sgct/sgct.git
   cd sgct
   ```
1. Configure, build, and test through the provided CMake presets.  The first configure downloads and builds the dependencies, which takes a while; subsequent runs are served from vcpkg's binary cache.
   ```
   cmake --preset windows
   cmake --build --preset windows
   ctest --preset windows
   ```

Use `linux` instead of `windows` on Linux.  The presets also offer `windows-debug`, `windows-release`, `linux-debug`, and `linux-release`, plus `windows-static` for linking the dependencies statically.

## Build options
| Option | Default | Description |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | `OFF` | Build SGCT as a shared library |
| `SGCT_BUILD_TESTS` | `ON` when SGCT is the top-level project, otherwise `OFF` | Build the unit tests |
| `SGCT_BUILD_CALIBRATOR` | `ON` | Build the `calibrator` application that renders a test pattern for projector calibration |
| `SGCT_TRACY_SUPPORT` | `OFF` | Enable [Tracy](https://github.com/wolfpld/tracy) profiling |
| `SGCT_MEMORY_PROFILING` | `OFF` | Override `new`/`delete` for Tracy memory profiling; requires `SGCT_TRACY_SUPPORT` |
| `SGCT_NDI_SUPPORT` | `OFF` | Windows only. Proprietary SDK, the result must not be redistributed |
| `SGCT_SCALABLE_SUPPORT` | `OFF` | Windows only. Proprietary SDK, the result must not be redistributed |
| `SGCT_ENABLE_EDIT_CONTINUE` | `ON` | Windows only. Compile with `/ZI` |
| `SGCT_ENABLE_STATIC_ANALYZER` | `OFF` | Unix only. Compile with `-fanalyzer` |

Catch2 and Tracy are regular dependencies in `vcpkg.json`, so these options can be switched on an existing build directory without touching vcpkg.  A project that includes SGCT through `add_subdirectory` can override `SGCT_BUILD_TESTS`, `SGCT_BUILD_CALIBRATOR`, and `SGCT_TRACY_SUPPORT` by setting the variable before the `add_subdirectory` call.


## Consuming SGCT
A vcpkg port lives in `support/vcpkg/ports/sgct`.  Register it as an overlay from your own project's `vcpkg-configuration.json`:
```json
{
  "overlay-ports": [ "path/to/sgct/support/vcpkg/ports" ]
}
```
and then link against it:
```cmake
find_package(sgct CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE sgct::sgct)
```

The port declares the same runtime dependencies as the root `vcpkg.json`; the two are kept in sync by `support/vcpkg/check-manifest-sync.cmake`, which runs as part of the test suite.

## Upgrading from SGCT 3.x
Version 4.0 changes how SGCT is built and consumed:

 - All third-party libraries are now resolved through vcpkg. The bundled `ext/` submodules
   and vendored source have been removed, along with the `SGCT_DEP_INCLUDE_*` CMake
   options that toggled them. A vcpkg checkout is now a hard build requirement.
 - OpenVR support has been replaced by OpenXR, which is always compiled in. The
   `SGCT_OPENVR_SUPPORT` option and the `additional_includes/openvr` headers are gone.
 - Dependencies are pulled in with `find_package(... CONFIG)` rather than
   `add_subdirectory`, so a project vendoring SGCT via `add_subdirectory` must supply the
   vcpkg toolchain and the corresponding ports itself; consuming the overlay port with
   `find_package(sgct)` (see above) is the supported path.

# License
SGCT is licensed under the [3-clause BSD license](https://choosealicense.com/licenses/bsd-3-clause/)

```
Copyright (c) 2012-2026
Miroslav Andel, Linköping University
Alexander Bock, Linköping University

Contributors: Alexander Fridlund, Joel Kronander, Daniel Jönsson, Erik Sundén, Gene Payne,
              Peter Steneteg

For any questions or information about the SGCT project please contact: alexander.bock@liu.se

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1.   Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.
2.   Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.
3.   Neither the name of the copyright holder nor the names of its contributors
     may be used to endorse or promote products derived from this software
     without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ''AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDERS AND CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
```

For any questions or further information about the SGCT project, please contact [alexander.bock@liu.se](mailto:alexander.bock@liu.se) or [erik.sunden@liu.se](mailto:erik.sunden@liu.se).

## External libraries
SGCT links against the following external libraries, all of which are provided by vcpkg:

 - [Catch2](https://github.com/catchorg/Catch2) (tests only)
 - [FreeType](https://www.freetype.org)
 - [GLAD](https://github.com/Dav1dde/glad)
 - [GLFW](https://www.glfw.org)
 - [GLM](https://github.com/g-truc/glm)
 - [json-schema-validator](https://github.com/pboettch/json-schema-validator)
 - [libpng](http://www.libpng.org)
 - [minizip](https://github.com/madler/zlib/tree/master/contrib/minizip)
 - [nlohmann/json](https://github.com/nlohmann/json)
 - [OpenXR](https://github.com/KhronosGroup/OpenXR-SDK)
 - [scnlib](https://github.com/eliaskosunen/scnlib)
 - [Spout2](https://github.com/leadedge/Spout2) (Windows only)
 - [Vulkan](https://github.com/KhronosGroup/Vulkan-Loader) (Windows only; OpenXR Vulkan fallback)
 - [stb_image](https://github.com/nothings/stb)
 - [TinyXML-2](https://github.com/leethomason/tinyxml2)
 - [Tracy](https://github.com/wolfpld/tracy) (optional)
 - [zlib](https://www.zlib.net)
