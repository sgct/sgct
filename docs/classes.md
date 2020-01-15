# Classes

In the SGCT namespace you can find the following classes/files that are relevant to application developers.  For detailed API descriptions, see also the Doxygen documentation linked on the main page of this page.

# Basic features

### `sgct.h`
This file is a composite header that includes a number of different includes that might be used by application developers.  You can use this include to get started quickly with SGCT at the stage of the project when you don't need to worry about compilation times.

### `Engine`
This class is the core of SGCT.  There is more description about this class in the [How it works](how-it-works.md) section of this page.  For each application there has to be exactly one instance of this class at any point.

### `SharedData` / `shareddatatypes.h`
The `SharedData` class is a singleton that handles the network synchronization of state between the server and the clients.  The server has to provide the state it wants to transmit to the clients using the `write*` functions (for example `writeFloat`, `writeUInt64`, ...) in the `Encode` callback and the clients have to read the same values in the same order using the `read*` functions in the `Decode` callback.  The `sharedddatatypes.h` file contains all predefined helper types to make this functionality easy to use.  The buffer it uses grows dynamically, which means that the amount of data that can be transmitted can change from frame to frame.  However, keep in mind that an overly large buffer will lead to performance degredation.

### `actions.h` / `joystick.` / `keys.h` / `modifiers.h` / `mouse.h`
These files contain all the enumerations that are necessary to use the interaction-based callbacks.

### `ogl_headers.h`
This file includes everything that is necessary to use OpenGL in the application code.  If your application doesn't want the GLAD based OpenGL bindings, you should not include this file but use your own bindings instead.

### `ShaderManager` / `ShaderProgram`
These two classes handle the compilation of OpenGL shaders.  The ShaderManager has a `addShaderProgram` function to create a new `ShaderProgram` that contains a vertex and a fragment shader source code.  The `ShaderManager` also manages the shader programs that it creates and will destroy them in the tear down phase of the application.  If the application needs to dispose of a shader program ahead of time, it has a `removeShaderProgram` function for this.

### `TextureManager`
The `TextureManager` loads and manages OpenGL textures.  The `loadTexture` function loads, creates, and uploads an OpenGL texture and returns the object id of that texture.  That id can later be used in a call to `removeTexture` to dispose of the texture.  All textures that are contained in the `TextureManager` when the application terminates are automatically freed.

### `Error`
The error class is used to notify the application when an unrecoverable error has occurred.  These errors range from error when reading the XML configuration file, problems when initializing the network component, or problems with the windowing system.  Each error contains a unique error code, with each error code described in more detail [here](errors.md).

### `Font`, `FontManager`, `freetype.h`
These classes and files are used to load, manage, and render fonts.  The `FontManager`'s `addFont` method can be used to load individual fonts whose `name` can be used to access them through the `font` function, which returns a pointer to a `Font` object used in the `print` function in the `freetype.h` file to actually render text using the specific font.  The font used in the `addFont` method should be a name of a font that is installed in the systems global Font directory.  The `print` function takes, among others, the `Font` object used to render the text, and the `x` and `y` coordinates in screen space where the text should be rendered.

### `Log`
The static `Log` class is used to render debug messages, informational text, warning messages, or errors to the console and optionally to a logging file on disk.  The static `Debug`, `Warning`, `Info`, and `Error` functions can be used by the application in the same way.  The class also supports the registration of a custom log callback that will be called whenever such a log message is queued.

# Advanced features

### `config.h`
This file defines the cluster setup that is used to initialize SGCT.  The basic behavior is to use the `readconfig.h` to read an XML file which will create such a configuration, but directly accessing this file makes it possible to programmatically create these configurations.
 
### `Image`
The `Image` class can be used by applications to load image files from disk or write image files to disk.  The class supports reading PNGs, JPEGs, and TGA file formats.

### `Network` / `NetworkManager`
The `NetworkManager` can be used to create new `Network` connections that can be used by the application for applications-specific tasks.  Please note that it is *not* necessary to manually create any of the network connections used in a clustered environment.  SGCT will create and manage those connections automatically.

### `OffscreenBuffer`
The `OffscreenBuffer` class can be used as a render target for application-specific rendering code.  If the application, for example, requires a post processing step, it might be useful to render the application into an offscreen buffer first and then apply the post-processing before blitting the rendering into the SGCT provided buffer.

### `profiling.h`
SGCT comes with a profiling library called [Tracy](https://bitbucket.org/wolfpld/tracy), which is made available through this header file.  Describing the steps needed to use Tracy would exceed this documentation, so you are referred to its [documentation](https://bitbucket.org/wolfpld/tracy/downloads/tracy.pdf) for more information.

### `Settings`
These global settings can be queried and set by the application to programmatically influence the way SGCT operates.  Please note that some of these settings need to be set before creating the `Engine` class as they are already used in that initial step of the pipeline.  When in doubt, you are referred to the Doxygen documentation.

### `Tracker`, `TrackingDevice`, `TrackingManager`
These classes interface with the VRPN interface and make it possible to use external tracking information to steer client applications.

### `ClusterManager`
This singleton class contains information about all the created `Node`s and `User`s in the cluster.  Its main use it the `thisNode` function which allows the client application to access settings of the node that it is running on.  This can be used to, for example, query the local IP address, the number of `Window`s, or other settings.  The `ClusterManager` contains `Node` instances for **all** nodes in the cluster, but all `Node`s except that one that the specific application is running on are not fully initialized and should only be used in special situations.

As a clarification imaging that the application was loaded with a configuration that includes 3 nodes, 1 server and 2 clients.  On all three nodes, the `ClusterManager` contains three `Node` instances, but only one of which is fully realized on each computer, the other two nodes representing the other computers are not fully initialized.

### `Node`
A `Node` class represents the server node and all cliebnt nodes and is mainly a container for its IP address, the port at which the `Node` can be reached and a list of all `Window`s that are associated with the specific node.  Only the current node that is running is fully initialized and has `Window`s to access.

### `Window`
The `Window` class represents a single window that was created on the current node.  The window is automatically created by the operating system, but most settings can be changed by the application afterwards.  See the Doxygen documentation of the individual functions for more information.  Additionally, each `Window` keep track of it `Viewport`s that a window is subdivided in.  The most common use case is that a `Window` has exactly one `Viewport` that is covering the entire window, but it is possible to tile a window with different viewports if such a behavior is desired.
