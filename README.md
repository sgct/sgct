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
For tutorials on how to use SGCT, look at the `src/apps` folder for a large amount of examples.  These can be compiled by enabling the `SGCT_EXAMPLES` CMake option.

# License
SGCT is licensed under the [3-clause BSD license](https://choosealicense.com/licenses/bsd-3-clause/)

```
Copyright (c) 2012-2020
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
SGCT uses and acknowledges the following external libraries:

 - [FreeType](http://www.freetype.org)
 - [GLAD](https://github.com/Dav1dde/glad)
 - [GLFW](ttps://www.glfw.org)
 - [GLM](http://glm.g-truc.net)
 - [libpng](http://www.libpng.org)
 - [OpenVR](https://github.com/ValveSoftware/openvr)
 - [Spout](https://github.com/box/spout)
 - [stb_image](https://github.com/let-def/stb_image)
 - [TinyXML](https:/github.com/leethomason/tinyxml2)
 - [Tracy](https://github.com/nette/tracy)
 - [VRPN](https://github.com/vrpn/vrpn)
 - [zlib](https://www.zlib.net)
