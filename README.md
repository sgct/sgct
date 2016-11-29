# SGCT - Simple Graphics Cluster Toolkit

SGCT is a free static cross-platform C++ library for developing OpenGL applications that are synchronized across a cluster of image generating computers (IGs). SGCT applications are scalable and use a XML configuration file where all cluster nodes (IGs) and their properties are specified. Therefore there is no need of recompiling an application for different VR setups. SGCT does also support running cluster applications on a single computer for testing purposes. SGCT is designed to be as simple as possible for the developer and is well suited for rapid prototyping of immersive virtual reality (VR) applications. SGCT can also be used to create planetarium/dome content and can generate fisheye projections. Its also possible to render to file to create extreeme hi-resolution stereoscopic 3D movies.

#### How it works
The most important component in SGCT is the engine. The engine handles all the initiation, rendering, network communication and configuration handling. The user can bind functions (callbacks) to the engine to customize specific tasks. Callbacks for keyboard and mouse input are handled by GLFW. Bonded functions will be called in different stages in the rendering process illustrated below:
![SGCT Pipeline Diagram](https://c-student.itn.liu.se/wiki/_media/develop:sgct:render_diagram.png "SGCT Pipeline Diagram")

#### Init OpenGL
This stage is called after the OpenGL context has been created and is only called once. This callback must be set before the Engine is initiated to have any effect. During the other stages the callbacks can be set end re-set anytime.
#### Pre Sync
This stage is called before the data is synchronized across the cluster. Set the shared variables here on the master and the slaves will receive them during the sync stage.
#### Sync
This stage distributes the shared data from the master to the slaves. The slaves wait for the data to be received before the rendering takes place. The slaves also send messages to the server if there are any (console printouts, warnings and errors). There are two callbacks that can be set here, one for the master and one for the slaves. The master encodes and serializes the data and the slaves decode and de-serialize the data.
#### Post Sync Pre Draw
At this stage the data is synchronized and can be applied.
#### Clear Buffers
This stage clears the buffers and sets the clear color. If this callback is set then it overrides the default clear buffers function. This stage can be called several times per frame depending on how many passes are rendered and if stereoscopic rendering is active. If this callback is not set, the default function is:
#### Draw
This stage draws the scene to the current back buffer (left, right or both). This stage can be called several times per frame if multiple viewports and/or if stereoscopic rendering is active.
#### Post Draw
This stage is called after the rendering is finalized.
#### Lock
During this stage the master is locked and waiting for all slaves. No callbacks can be set during this stage. The master doesn’t lock and wait if Nvidia’s swap barrier is active.
#### Swap Buffers
The front and back buffers are swapped. Triple buffering should not be used on a cluster when the application waits for vertical sync. No callbacks can be set during this stage. If Nvidia’s swap barrier is active then the buffer swap will be synchronized using hardware across the cluster.
#### Bind functions
The following functions can be found in the Engine class to bind functions as callbacks:
```sh
void setInitOGLFunction( void(*fnPtr)(void) );
void setPreSyncFunction( void(*fnPtr)(void) );
void setPostSyncPreDrawFunction( void(*fnPtr)(void) );
void setClearBufferFunction( void(*fnPtr)(void) );
void setDrawFunction( void(*fnPtr)(void) );
void setPostDrawFunction( void(*fnPtr)(void) );
void setCleanUpFunction( void(*fnPtr)(void) ); 
//called when application terminates
```
There is one additional function that binds a function as callback when an external TCP control interface is being used. This callback is called when TCP data is received.
```sh
void setExternalControlCallback( void(*fnPtr)(const char *, int, int) );
// The arguments above are: buffer, size of buffer and client index. 
```
The following functions can be found in the SharedData class to bind functions as callbacks during the Sync stage. The encode function will be called from the master and the decode function is called on the slaves.
```sh
void setEncodeFunction( void(*fnPtr)(void) );
void setDecodeFunction( void(*fnPtr)(void) );
```
#### Getting Started

More information, guides etc can be found on the [C-Student Wiki](https://c-student.itn.liu.se/wiki/) and we suggest beginners start reading the [Getting Started](https://c-student.itn.liu.se/wiki/develop:sgct:gettingstarted:gettingstarted) section.