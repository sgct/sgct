This page contains documentation and samples of various configuration files.

# General layout
The format for the SGCT configuration files is XML with a single [Cluster](#cluster) tag that can contain 1 or more [Node](#node) tags, a [User](#user) tag, an optional [Settings](#settings) tag, an optional [Capture](#capture) tag, and an optional [Tracker](#tracker) tag.  Each of the tags is described on this page further below.


## Examples
This section contains two almost minimal examples showing on a small variety of configuration options.  Check the `config` folder in SGCT for more examples.

Here is a minimal example of a single node, single window configuration.  This file creates a single node on `localhost` with a single window that has a size of 1280 by 720 pixels with a camera of 80 degrees horizontal field-of-view and approximately 50.5 degrees vertical field of view.
```xml
<?xml version="1.0" ?>
<Cluster masterAddress="localhost">
  <Node address="localhost" port="20401">
    <Window fullScreen="false">
      <Size x="1280" y="720" />
      <Viewport>
        <Pos x="0.0" y="0.0" />
        <Size x="1.0" y="1.0" />
          <PlanarProjection>
            <FOV down="25.267007923362" left="40.0" right="40.0" up="25.267007923362" />
            <Orientation heading="0.0" pitch="0.0" roll="0.0" />
          </PlanarProjection>
      </Viewport>
    </Window>
  </Node>
  <User eyeSeparation="0.06">
    <Pos x="0.0" y="0.0" z="4.0" />
  </User>
</Cluster>
```
The following example is a configuration that creates two nodes, both running on the local machine, the difference being that their field-of-views are offset.
```xml
<?xml version="1.0" ?>
<Cluster masterAddress="127.0.0.1">
  <Node address="127.0.0.1" port="20401">
    <Window fullScreen="false">
      <Pos x="0" y="300" />
      <Size x="640" y="360" />
      <Viewport>
        <Pos x="0.0" y="0.0" />
        <Size x="1.0" y="1.0" />
          <PlanarProjection>
            <FOV down="25.267007923362" left="40.0" right="40.0" up="25.267007923362" />
            <Orientation heading="-20.0" pitch="0.0" roll="0.0" />
          </PlanarProjection>
      </Viewport>
    </Window>
  </Node>
  <Node address="127.0.0.2" port="20402">
    <Window fullScreen="false">
      <Pos x="640" y="300" />
      <Size x="640" y="360" />
      <Viewport>
        <Pos x="0.0" y="0.0" />
        <Size x="1.0" y="1.0" />
          <PlanarProjection>
            <FOV down="25.267007923362" left="40.0" right="40.0" up="25.267007923362" />
            <Orientation heading="20.0" pitch="0.0" roll="0.0" />
          </PlanarProjection>
      </Viewport>
    </Window>
  </Node>
  <User eyeSeparation="0.06">
    <Pos x="0.0" y="0.0" z="4.0" />
  </User>
</Cluster>
```

The final example is a fisheye rendering which demonstrates a more sophisticated rendering setup.
```xml
<?xml version="1.0" ?>
<Cluster masterAddress="localhost">
  <Node address="localhost" port="20401">
    <Window fullScreen="false">
      <Stereo type="none" />
      <Size x="512" y="512" />
      <Viewport name="fisheye">
        <Pos x="0.0" y="0.0" />
        <Size x="1.0" y="1.0" />
        <FisheyeProjection fov="180" quality="medium" tilt="27.0">
          <Background r="0.1" g="0.1" b="0.1" a="1.0" />
        </FisheyeProjection>
      </Viewport>
    </Window>
  </Node>
  <User eyeSeparation="0.06">
    <Pos x="0.0" y="0.0" z="0.0" />
  </User>
</Cluster>
```


# Element types
The rest of the documentation contains information about the different types of XML nodes that can be added to the configuration files.  Some node types and attributes are maked as *optional* which means that they do not have to be present and their default behavior depends on the particular option and is usually mentioned in the description.


## Cluster
The cluster XML node has attributes that determine the behavior of the overall clusters.  This node is required to be present in the XML file and has to be the root of the document.

### Attributes
`masterAddress` \[ string \]
 > Contains the address of the node that acts as the server for this cluster.  This means that one of the `Node` elements described in this configuration file *has* to have an address that corresponds to this `masterAddress`.  This value can be either an IP address or a DNS name, which will be resolved at application startup.

`setThreadAffinity` *optional* \[ integer >=0 \]
 > Forces the thread affinity for the main thread of the application.  On Windows, this is achieved using the [SetThreadAffinityMask](https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-setthreadaffinitymask).  The default value is that no thread affinity is set for the application.

`debugLog` *optional* \[ boolean \]
 > Determines whether the logging should include `Debug` level log messages.  The default value is `false`, such that only `Info` level log messages or above are added to the console.  Log messages that are not logged are discarded.

`externalControlPort` *optional* \[ integer > 0 \]
 > If this value is set, a socket will be opened at the provided port.  Messages being sent to that port will trigger a call to the callback function `externalDecode`.  If such a callback does not exist, the incoming messages are ignored.  The default behavior is that no such external port is opened.  Please note that operating systems have restricted behavior when trying to open ports lower than a fixed limt.  For example, Unix does not allow non-elevated users to open ports < 1024.

`firmSync` *optional* \[ boolean \]
 > Determines whether the server should frame lock and wait for all client nodes or not.  The default for this is `false`.

### Children
 - [`Capture`](#capture) \[ 0-1 \] 
 - [`Node`](#node) \[ 1 - inf \]
 - [`Scene`](#scene) \[ 0-1 \]
 - [`Settings`](#settings) \[ 0-1 \]
 - [`Tracker`](#tracker) \[ 0 - inf \]
 - [`User`](#user) \[ 1 - inf \]


## Capture
The capture node contains information relevant to capturing screenshots from an SGCT application. 

### Attributes
`path` *optional* \[ string \]
 > Sets the same path for `monoPath`, `leftPath`, and `rightPath`.  If any of these three are also specified, they will overwrite the value provided in this function.

`monoPath` *optional* \[ string \]
 > Sets the path used when creating screenshots in the cases when no stereo mode is enabled.  The default value is `SGCT` relative to the working directory.  This value is ignored if the configuration uses any type of stereoscopic rendering method.

`leftPath` *optional* \[ string \]
 > Sets the path used for the screenshots of the left eye in the cases when a stereo configuration is used.  This value is ignored if the configuration does not enable any type of stereo rendering.  The default value is `SGCT` relative to the working directory.

`rightPath` *optional* \[ string \]
 > Sets the path used for the screenshots of the right eye in the cases when a stereo configuration is used.  This value is ignored if the configuration does not enable any type of stereo rendering.  The default value is `SGCT` relative to the working directory.

`format` *optional* \[ (png, PNG, tga, TGA, jpg, or JPG) \]
 > Sets the screenshot format that should be used for the screenshots taken of the application.  The default value is `PNG`.


## Node
This XML node defines a single computing node that is contained in the described cluster.  In general this corresponds to a single computer, but it is also possible to create multiple nodes on a local machine by using the `127.0.0.x` IP address with `x` from `0` to `255`.  It is not possible to create multiple nodes on the same *remote* computer.

### Attributes
`address` \[ string \]
 > The IP address or the DNS name of the node.  If the `address` is a DNS name, the name resolution is delegated to the operating system and might include additional network traffic to the DNS host.  If the node ought to be the local machine either `127.0.0.x` with `x` from `0` to `255`, or `localhost` can be used.

`port` \[ integer > 0 \]
 > The port at which this node is available at.  Since the server has to open bidirectional sockets to all of the client nodes, the `port` for each `Node` has to be mutually exclusive, meaning that no two `Node`s can have the same `port`.  Please note that operating systems have restricted behavior when trying to open ports lower than a fixed limt.  For example, Unix does not allow non-elevated users to open ports < 1024.

`dataTransferPort` *optional* \[ integer > 0 \]
 > If this value is set, a socket connection is opened at the provided port that can be used by the application to transmit data between the server and the clients using the `dataTransfer*` callbacks and `transferData` function of the `NetworkManager`.  If no value is specified this function will not work and the callbacks will never be called.  Please note that operating systems have restricted behavior when trying to open ports lower than a fixed limt.  For example, Unix does not allow non-elevated users to open ports < 1024.

`swapLock` *optional* \[ boolean \]
 > Determines whether this node should be part of an Nvidia swap group and should use the swap barrier.  Please note that this feature only works on Windows and requires Nvidia Quadro cards + G-Sync synchronization cards.  For more information on swap groups, see [here](https://www.nvidia.com/content/dam/en-zz/Solutions/design-visualization/quadro-product-literature/Quadro_GSync_install_guide_v4.pdf).  The default value is `false`.

### Children
 - [`Window`](#window) \[ 1 - inf \]


## Scene
This node determines an overall orientation of the scene.  It consists of an `Offset`, an `Orientation`, and `Scale`;  all of which is included in the projection matrix that is passed to the rendering function callback of the specific application.  This node can be used to customize the rendering for a specific rendering window.  A common use-case in planetariums, for example, is to account for a tilt in the display system by providing an `Orientation` with the same pitch as the planetarium surface.  This makes it possible to reuse the same application between the planetarium dome and fixed setups without the need for special care.

### Children
`Offset` \[ 0 - 1 \]
 > A linear offset of the scene center.  Must define three float attributes `x`, `y`, and `z`.  The default value is `x=0`, `y=0`, `z=0`.

`Orientation` \[ 0 - 1 \]
 > Describes a fixed orientation of the global scene.  This can be provided either as Euler angles or as a quaternion.  The two modes *cannot* be mixed.  The following descibes the different attributes that can be used for the orientation.  Please note that *all* attributes for the chosen method have to be specified.
 > 
 > Euler angles:
 >  - `pitch` or `elevation`
 >  - `yaw`, `heading`, or `elevation`
 >  - `roll` or `bank`
 > 
 > Quaternion:
 >  - `x`
 >  - `y`
 >  - `z`
 >  - `w`
 > 
 >  Example:  `<Orientation pitch="20.0" elevation="-12.0" roll="0.0" />`

`Scale` \[ 0 - 1 \]
 > A scaling factor for the entire scene.  The default value is `1.0`.


## Settings
This node controls global settings that affect the overall behavior of the SGCT library that are not limited just to a single window.

### Attributes
`DepthBufferTexture` *optional* \[ boolean \]
 > If this value is set to `true` and a non-linear projection method if provided in a window, SGCT will also provide a buffer containing the re-projected depth values of the non-linear projection.  This value defaults to `false`.

`NormalTexture` *optional* \[ boolean \]
 > If this value is set to `true` and a non-linear projection method if provided in a window, SGCT will also provide a buffer containing the re-projected normals values of the non-linear projection.  This value defaults to `false`.

`PositionTexture` *optional* \[ boolean \]
 > If this value is set to `true` and a non-linear projection method if provided in a window, SGCT will also provide a buffer containing the re-projected positions of the non-linear projection.  This value defaults to `false`.

`Precision` *optional* \[ 16 or 32 \]
 > Determines the floating point precision for the normal and position textures if they are enabled.  Setting this value if `NormalTexture` and `PositionTexture` are disabled does not have any effect.  This value defaults to `32`.

### Children
`Display` \[ 0 - 1 \]
 > Contains settings specific for the global handling of display-related settings.
 > 
 > `swapInterval` *optional* \[ integer \]
 >  > Determines the swap interval for the application.  This determines the amount of V-Sync that should occur for the application.  The two most common values for this are `0` for disabling V-Sync and `1` for regular V-Sync.  The number provided determines the number of screen updates to wait before swapping the backbuffers and returning.  For example on a 60Hz monitor, `swapInterval="1"` would lead to a maximum of 60Hz frame rate, `swapInterval="2"` would lead to a maximum of 30Hz frame rate.  The default value is `0`.
 >
 > `refreshRate` *optional* \[ integer \]
 >  > Determines the desired refresh rate for full-screen windows of this configuration.  This value is disabled for windowed mode windows.  The default value is the highest possible refresh rate.
 > 
 > `exportWarpingMeshes` *optional* \[ boolean \]
 >  > If this value is set to `true`, any warping mesh that is loaded for a viewport will be exported as an Waveform `obj` file while processing.  This allows debugging of the warping meshes for tools that only support loading of `obj` files.  The default value is `false`.


## Tracker
### Attributes
`name` \[ string \]
 > The name of the tracker group.

### Children
 - [`Device`](#device) \[ 0 - inf \]

`Offset` \[ 0 - 1 \]
 > A linear offset of the class of trackers.  Must define three float attributes `x`, `y`, and `z`.  The default value is `x=0`, `y=0`, `z=0`.

`Orientation` \[ 0 - 1 \]
 > Describes a fixed orientation of this class of trackers.  This can be provided either as Euler angles or as a quaternion.  The two modes *cannot* be mixed.  The following descibes the different attributes that can be used for the orientation.  Please note that *all* attributes for the chosen method have to be specified.  If the `Matrix` attribute is also specified, it will overwrite the value specified here.
 > 
 > Euler angles:
 >  - `pitch` or `elevation`
 >  - `yaw`, `heading`, or `elevation`
 >  - `roll` or `bank`
 > 
 > Quaternion:
 >  - `x`
 >  - `y`
 >  - `z`
 >  - `w`
 > 
 >  Example:  `<Orientation pitch="20.0" elevation="-12.0" roll="0.0" />`

`Scale` \[ 0 - 1 \]
 > A scaling factor for this class of trackers.  The default value is `1.0`.

`Matrix` \[ 0 - 1 \]
> A generic transformation matrix that is applied to all trackers in this group.  This value will overwrite the value specified in `Orientation`.  The attributes used for the matrix are named `x0`, `y0`, `z0`, `w0`, `x1`, `y1`, `z1`, `w2`, `x2`, `y2`, `z2`, `w2`, `x3`, `y3`, `z3`, `w3` and are used in this order to initialize the matrix in a column-major order.  All 16 of these values have to be present in this attribute and have to be floating point values.
> 
> `transpose` *optional* \[ boolean \]
>  > If this value is present and `true` the values provided are interpreted as being in row-major order, rather than column-major order.  The default is `false`, making the matrix column-major.


## User
This XML node specifies a user position and parameters.  In most cases, only a single unnamed user in necessary.  However, in more general cases, it is possible to assign `User`s to specific [Viewports](#viewport) to provide a more finegrained control over the rendering that occurrs in that viewport.

### Attributes
`name` *special* \[ string \]
> Specifies the name of this user.  Each user needs to have a unique name, but there also *has* to be exactly one user present that has an empty name which is used as the default user.

`eyeSeparation` *optional* \[ float >= 0 \]
> Determines the eye separation used for stereoscopic viewports.  If no viewports in the configuration are using stereo, this setting is ignored.

### Children
`Pos` \[ 0 - 1 \]
 > A linear offset of the user position.  Must define three float attributes `x`, `y`, and `z`.  The default value is `x=0`, `y=0`, `z=0`.

`Orientation` \[ 0 - 1 \]
 > Describes a fixed orientation for the viewing direction of this user.  This can be provided either as Euler angles or as a quaternion.  The two modes *cannot* be mixed.  The following descibes the different attributes that can be used for the orientation.  Please note that *all* attributes for the chosen method have to be specified.  If the `Matrix` attribute is also specified, it will overwrite the value specified here.
 > 
 > Euler angles:
 >  - `pitch` or `elevation`
 >  - `yaw`, `heading`, or `elevation`
 >  - `roll` or `bank`
 > 
 > Quaternion:
 >  - `x`
 >  - `y`
 >  - `z`
 >  - `w`
 > 
 >  Example:  `<Orientation pitch="20.0" elevation="-12.0" roll="0.0" />`

`Matrix` \[ 0 - 1 \]
> A generic transformation matrix that is applied to the orientation of this user.  This value will overwrite the value specified in `Orientation`.  The attributes used for the matrix are named `x0`, `y0`, `z0`, `w0`, `x1`, `y1`, `z1`, `w2`, `x2`, `y2`, `z2`, `w2`, `x3`, `y3`, `z3`, `w3` and are used in this order to initialize the matrix in a column-major order.  All 16 of these values have to be present in this attribute and have to be floating point values.
> 
> `transpose` *optional* \[ boolean \]
>  > If this value is present and `true` the values provided are interpreted as being in row-major order, rather than column-major order.  The default is `false`, making the matrix column-major.

`Tracking` \[ 0 - 1 \]
> Provides information about whether this user should be tracked using a VRPN-based tracker.  This child node contains two attributes with information about the tracker that this user should be associated with.
> 
> `tracker` \[ string \]
> > The name of the tracker group that this user should be linked with.  This name must be a name of a [Tracker](#tracker) that is specified in this configuration.
> 
> `device` \[ string \]
> > The name of the device in the tracker group that should be used to control the tracking for this user.  The specified device has to be a [Device](#device) that was specified in the [Tracker](#tracker) `tracker`.


## Window
This XML specifies a single window that is used to render content into.  There can be an arbitrary(*-ish*) number of windows for each node and they all will be created and initialized at start time.  Each window has at least one [Viewport](#viewport) that specifies exactly where in the window the rendering occurs with which parameters.

### Attributes
`name` *optional* \[ string \]
> The name of the window.  This is also used as the title of the window if window decorations are enabled.  The default name for a window if this value is not specified is "SGCT Node: %i (%s)" with `%i` = the address of this node and `%s` either "server" or "client", depending on whether the current node is the server or the client in the cluster.

`tags` *optional* \[ string \]
> A comma-separated list of tags that are associated with this window.  The tags themselves don't have any meaning inside of SGCT, but can be used by the application code as a filter.  One common use-case is to tag one of the windows as a "GUI" window, to restrict input to only that window, for example.  The default value for this attribute is an empty string.

`bufferBitDepth` *optional* \[ 8, 16, 16f, 32f, 16i, 32i, 16ui, or 32ui \]
> Sets the bit depth and format of the color texture that is used as the render backend for this entire window.  The parameters passed into this attribute are converted to the following OpenGL parameters (internal color format and data type) to the texture creation:
> 
> - `8`: `GL_RGBA8`, `GL_UNSIGNED_BYTE`
> - `16`: `GL_RGBA16`, `GL_UNSIGNED_SHORT`
> - `16f`: `GL_RGBA16F`, `GL_HALF_FLOAT`
> - `32f`: `GL_RGBA32F`, `GL_FLOAT`
> - `16i`: `GL_RGBA16I`, `GL_SHORT`
> - `32i`: `GL_RGBA32I`, `GL_INT`
> - `16ui`: `GL_RGBA16UI`, `GL_UNSIGNED_SHORT`
> - `32ui`: `GL_RGBA32UI`, `GL_UNSIGNED_INT`
>
> The default value for this attribute is `8`.

`fullscreen` *optional* \[ boolean \]
> Determines whether the window should be created as an exclusive fullscreen window.  The `Size` of this window will be used to set the screen resolution if this value is `true`.  See also the `monitor` attribute to determine which monitor should be used as the target for the fullscreen window.  The default value is `false`

`floating` *optional* \[ boolean \]
> Indicates whether the window is floating, meaning that it is rendered by the operating system always on top of other windows.  The default value is `false`.

`alwaysRender` *optional* \[ boolean \]
> Determines whether the content of the window should continue to render even if the window is not visible on screen.  Normally, the operating system will not invalidate a window when it is hidden (see `isHidden` attribute) and this attribute can be used to overwrite that behavior.  The default behavior is `false`.

`isHidden` *optional* \[ boolean \]
> Determines whether this window should be visible on screen or used as an offscreen rendering target.  If a window is hidden, you should also set `alwaysRender` to `true`, or otherwise the rendering might not occur as expected.  The default for this attribute is `false`.

`dbuffered` *optional* \[ boolean \]
> Sets the buffering to single buffering (if `false`) or double buffering or quad buffering (if `true`).  The default is `true`.

`msaa` *optional* \[ integer >= 0 \]
> Regulates whether multisample antialiasing is used for the window and how many subsamples should be used for the antialiasing.  If the value is set to `0`, MSAA is disabled.  MSAA operates by rendering the scene at a higher resolution using multiple samples per pixel and combining these samples to reduce aliasing.  It produces good-looking results, but it increases the rendering time for the scene.  The maximum number of samples depends on the GPU that is used to start the application, but is usually around 32.  The default is `0`, disabling MSAA.

`hasAlpha` *optional* \[ boolean \]
> Determines whether screenshots created from this window should include an alpha channel or not.  If this value is `false`, the resulting image type is `RGB`, otherwise `RGBA`.  The default is `false`.

`fxaa` *optional* \[ boolean \]
> Determines whether fast approximate antialiasing is used for the contents of this window.  This antialiasing is a postprocessing that does not significantly increase rendering time, but the results are not as good as `msaa`.  The default is `false`.

`border` *optional* \[ boolean \]
> Enables or disables the window decorations.  Window decorations are the title bar that contains the name of the window and buttons to close, maximize or minimize a window.  On some operating systems, the window decorations also include a border around a window and potentially shadow effects, all of which can be disabled with this attribute.  The default is `true`.

`draw2D` *optional* \[ boolean \]
> Determines whether the `draw2D` callback should be called for viewports in this window.  The default value is `true`.

`draw3D` *optional* \[ boolean \]
> Determines whether the `draw` callback should be called for viewports in this window.  The default value is `true`.

`blitPreviousWindow` *optional* \[ boolean \]
> If this value is set to `true`, the contents of the previous window are blitted (=copied) into this window before calling its own rendering.  A common use-case for this are GUI windows that want to show the 3D rendering but not take the performance hit of rendering an expensive scene twice.  Instead of rendering the 3D scene, a GUI window would set `draw3D` to `false` and this attribute to `true`, meaning that the contents of the previous window are copied and then the 2D UI will be rendered on top of the blitted content.  The previous window is the one that is specified just before this window in the XML file itself.  This also means that the first window in the configuration file can not has this value set to `true`.  The default value is `false`.

`monitor` *optional* \[ integer >= -1 \]
> Determines which monitor should be used for the exclusive fullscreen in case `fullscreen` is set to `true`.  Monitors in the system are zero-based and range between 0 and the number of monitors - 1.  For this attribute, the special value `-1` can be used to denote that the primary monitor should be used, regardless of its index.  The default value is `-1`.

`mpcdi` *optional* \[ string \]
> If this value is set to a path that contains an MPCDI file that describes camera parameters and warping and blending masks, these values are used to initialize the contents of this window instead of providing explicit viewport information.  The default value is that no MPCDI is used.

### Children
`Stereo` \[ 0 - 1 \] 
> Determines whether the contents of this window should be rendered stereoscopically and which stereoscopic rendering method should be used.  The only allowed attribute for this node is the `type`, which determines the type of stereo rendering.  It has to be one of:
> 
> - `none`:  No stereo rendering is performed.  This is the same as if this entire node was not specified.
> - `active`:  Using active stereo using quad buffering.  This is only a valid option for systems that support quad buffering.
> - `checkerboard`:  Using a checkerboard pattern for stereoscopy.
> - `checkerboard_inverted`:  Using the same pattern as `checkerboard`, but with the left and right eyes inverted
> - `anaglyph_red_cyan`:  Applying color filters to the left and right eyes such that red-cyan anaglyph glasses can be used to view the stereo content
> - `anaglyph_amber_blue`:  Applying color filters to the left and right eyes such that amber-blue anaglyph glasses can be used to view the stereo content
> - `anaglyph_wimmer`:  ¯\\_\(ツ\)_/¯
> - `vertical_interlaced`:  A stereo format in which the left and right eye images are interlaced vertically, meaning that each row of the final image is either left or right, switching each row.
> - `vertical_interlaced_inverted`:  The same as `vertical_interlaced`, but with the left and right eye flipped.
> - `dummy`:  A dummy stereo mode to test streoscopic rendering without needing extra equipment.  In this stereo mode, the left and the right eye images are rendered on top of each other without any other processing.
> - `side_by_side`:  The resolution of the window is split into a left half and a right half, with each eye being rendered into its half.  This is a common stereo format for 3D TVs.
> - `side_by_side_inverted`:  The same as `side_by_side`, but the left and right images are flipped.
> - `top_bottom`:  The same as `side_by_side`, but instead of separating the window horizontally, the window is split vertically, with the left eye being rendered in the top half of the window and the right image being rendered in the bottom half.
> - `top_bottom_inverted`:  The same as `top_bottom_inverted`, but with the left and right eyes flipped.
> 
> The default value is `none`.

`Pos` \[ 0 - 1 \]
> Sets the position of the window on the overall desktop space provided by the operating system.  This node must have `x` and `y` floating point attributes that specify the x-y location of the window.  Please note that these values also can be negative on some operating systems.  On Windows, for example, the top left corner of the primary monitor is (0,0) in this coordinate system, but there can be additional monitors to the left or the top of the primary monitor, which would require negative numbers.  The default value is `x=0` and `y=0`.

`Size` \[ 0 - 1 \]
> Sets the size of the window in pixels.  This node must have `x` and `y` floating point attributes that determine that size of the window.  The default value is `x=640` and `y=480`.

`Res` \[ 0 - 1 \]
> Sets the size of the internal framebuffer that is used to render the contents of the window. In a lot of cases, this resolution is the same resolution as the size of the window, but it is a useful tool when creating images that are larger than a window would be support on an operating system.  Some operating systems restrict windows to be no larger than what can fit on a specific monitor.  This node must have `x` and `y` floating point attributes that determine that size of the window.  By default the resolution of the framebuffer is equal to the size of the window.

- [Viewport](#viewport) \[ 1 - inf \]


## Device
This node specifies a single tracking device that belongs to a specific tracker group.  

### Attributes
`name` \[ string \]
> Specifies the name of the device so that it can be referenced by a [User](#user) or can be accessed programmatically by the application.

### Children
`Sensor` \[ 0 - inf \]
 > This node represents a tracked sensor that provides orientation and position information.
 > 
 > `vrpnAddress` \[ string \]
 >  > The VRPN address of this sensor
 > 
 > `id` \[ integer \]
 >  > The sensor id for this device.  This information is not used by SGCT directly but can be used by the application to distinguish different sensors if multiple sensors are specified in the configuration file

`Buttons` \[ 0 - inf \]
> This node represents a group of toggle buttons that can be triggered through VRPN.
> 
> `vrpnAddress` \[ string \]
> > The VRPN address of this button group
> 
> `count` \[ integer \]
> > The number of buttons that are advertised and received through the Device

`Axes` \[ 0 - inf \]
> This node represents a number of independent 1D axes that are updated through VRPN.
> 
> `vrpnAddress` \[ string \]
> > The VRPN address of this group of axes
> 
> `count` \[ integer \]
> > The number of axes that are advertised

`Offset` \[ 0 - 1 \]
 > A linear offset that is added to the entire device.  Must define three float attributes `x`, `y`, and `z`.  The default value is `x=0`, `y=0`, `z=0`.

`Orientation` \[ 0 - 1 \]
 > Describes a fixed orientation for the device.  This can be provided either as Euler angles or as a quaternion.  The two modes *cannot* be mixed.  The following descibes the different attributes that can be used for the orientation.  Please note that *all* attributes for the chosen method have to be specified.  If the `Matrix` attribute is also specified, it will overwrite the value specified here.
 > 
 > Euler angles:
 >  - `pitch` or `elevation`
 >  - `yaw`, `heading`, or `elevation`
 >  - `roll` or `bank`
 > 
 > Quaternion:
 >  - `x`
 >  - `y`
 >  - `z`
 >  - `w`
 > 
 >  Example:  `<Orientation pitch="20.0" elevation="-12.0" roll="0.0" />`

`Matrix` \[ 0 - 1 \]
> A generic transformation matrix that is applied to this device.  This value will overwrite the value specified in `Orientation`.  The attributes used for the matrix are named `x0`, `y0`, `z0`, `w0`, `x1`, `y1`, `z1`, `w2`, `x2`, `y2`, `z2`, `w2`, `x3`, `y3`, `z3`, `w3` and are used in this order to initialize the matrix in a column-major order.  All 16 of these values have to be present in this attribute and have to be floating point values.
> 
> `transpose` *optional* \[ boolean \]
>  > If this value is present and `true` the values provided are interpreted as being in row-major order, rather than column-major order.  The default is `false`, making the matrix column-major.


## Viewport
This node describes a single viewport inside a [Window](#window).  Every window can contain an arbitrary number of viewports that are all rendered independently.  The viewports are positioned inside the window using a normalized coordinate system.

### Attributes
`user` *optional* \[ string \]
 > The name of the [User](#user) that this viewport should be linked to.  If a viewport is linked to a user that has a sensor, the positions of the sensor will be automatically reflected in the user position that is used to render this viewport.  The default is that no user is linked with this viewport.

`overlay` *optional* \[ string \]
 > This attribute is a path to an overlay texture that is rendered on top of the viewport after the applications rendering is finished.  This can be used to add logos or other static assets on top of an application.  The default is that no overlay is rendered.

`mask` *optional* \[ string \]
 > This value is a path to a texture that is used as a mask to remove parts of the rendered image.  The image that is provided in this should be a binary black-white image which is applied by SGCT after the application loading is finished.  All parts where the `mask` image is black will be removed.  The default is that no mask is applied.

`BlackLevelMask` *optional* \[ string \]
 > The file referenced in this attribute is used as a postprocessing step for this viewport.  The image should be a grayscale image, where each pixel will be multiplied with the resulting image from the application in order to perform a black level adaptation.  If a pixel is completely white, the resulting pixel is the same as the applications output, if a pixel is black, the resulting pixel will be back, if it is 50% grey, the resolution pixel will be half brightness.  The default is that no black level mask is applied.

`mesh` *optional* \[ string \]
 > Determines a warping mesh file that is used to warp the resulting image.  The application's rendering will always be rendered into a rectangular framebuffer, which is then mapped as a texture to the geometry provided by this file.  This makes it possible to create non-linear or curved output geometries from a regular projection by providing the proper geometry of the surface that you want to project on.

`tracked` *optional* \[ boolean \]
 > Determines whether the field-of-view frustum used for this viewport should be tracking changes to the window configuration.  If this value is set to `false`, 

`eye` *optional* \[ center, left, or right \]
 > Forces this viewport to be rendered with a specific eye, using the corresponding [User](#user)s eye separation to compute the correct frustum.

### Children
`Pos` \[ 0 - 1 \]
 > Specifies the position of the viewport inside its parent [Window](#window).  The coordinates for `x` and `y`, which must both be specified in this node, are usually between 0 and 1, but are not restricted.  Parts of the viewport that are outside this range would lie outside the bounds of the window and are clipped.  Viewports are free to overlap and the viewports are rendered top to bottom into the window and can overwrite previous results.

`Size` \[ 0 - 1 \]
 > Specifies the size of this viewport inside its parent [Window](#window).  The coordinate for `x` and `y`, which must both be specified in this node, are between 0 and 1, but are not restricted.  Parts of the viewport that are outside this range would lie outside the bounds of the window and are clipped.  Viewports are free to overlap and the viewports are rendered top to bottom into the window and can overwrite previous results.

Following are the different kinds of projections that are currently supported.  Exactly one of the following projections has to be present in the viewport.
 - [PlanarProjection](#planarprojection) \[ special \]
 - [FisheyeProjection](#fisheyeprojection) \[ special \]
 - [SphericalMirrorProjection](#sphericalmirrorprojection) \[ special \]
 - [SpoutOutputProjection](#spoutoutputprojection) \[ special \]
 - [Projectionplane](#projectionplane) \[ special \]


## PlanarProjection
This projection node describes a projection for the [Viewport](#viewport) that is a flat projection described by simple frustum, which may be asymmetric.

### Children
`FOV` \[ 1 \]
> This element describes the field of view used the camera in this planar projection.
> 
> `down` \[ float \]
> > The angle (in degrees) that is covered by the camera between the central point and the bottom border of the of the viewport.  The `down` and `up` angles added together are the vertical field of view of the viewport.
> 
> `up` \[ float \]
> > The angle (in degrees) that is covered by the camera between the central point and the top border of the of the viewport.  The `down` and `up` angles added together are the vertical field of view of the viewport.
> 
> `left` \[ float \]
> > The angle (in degrees) that is covered by the camera between the central point and the left border of the of the viewport.  The `left` and `right` angles added together are the vertical field of view of the viewport.
> 
> `right` \[ float \]
> > The angle (in degrees) that is covered by the camera between the central point and the right border of the of the viewport.  The `left` and `right` angles added together are the vertical field of view of the viewport.
> 
> `distance` *optional* \[ float \]
> > The distance (in meters) at which the virtual render plane is placed.  This value is only important when rendering this viewport using stereocopy as the `distance` and the [User](#user)s `eyeSeparation` are used to compute the change in frustum between the left and the right eyes.

`Offset` \[ 0 - 1 \]
 > A linear offset in meters that is added to the virtual image plane.  Must define three float attributes `x`, `y`, and `z`.  The default value is `x=0`, `y=0`, `z=0`.

`Orientation` \[ 0 - 1 \]
 > Describes a fixed orientation for the virtual image plane.  This can be provided either as Euler angles or as a quaternion.  The two modes *cannot* be mixed.  The following descibes the different attributes that can be used for the orientation.  Please note that *all* attributes for the chosen method have to be specified.  If the `Matrix` attribute is also specified, it will overwrite the value specified here.
 > 
 > Euler angles:
 >  - `pitch` or `elevation`
 >  - `yaw`, `heading`, or `elevation`
 >  - `roll` or `bank`
 > 
 > Quaternion:
 >  - `x`
 >  - `y`
 >  - `z`
 >  - `w`
 > 
 >  Example:  `<Orientation pitch="20.0" elevation="-12.0" roll="0.0" />`


## FisheyeProjection
This node describes a fisheye projection that is used to render into its parent [Viewport](#viewport).  By default, a fisheye rendering is covering 180 degrees field of view and has a 1:1 aspect ratio, though these parameters can be changed with the attributes provided in this node.  This projection type counts as a non-linear projection, which requires 4-6 render passes of the application, which means that the application might render significantly slower when using these kind of projections.  However, the application does not need to be aware of the projection as this abstract is handled internally and the applications `draw` method is only called multiple times per frame with different projection methods that are used to create the full fisheye projection.

Depending on the field of view, a cube map is created consisting of 4-6 cube maps that are reprojected in a post-processing into a fisheye of the desired field-of-view.

### Attributes
`fov` *optional* \[ float > 0 \]
> Describes the field of view that is covered by the fisheye projection in degrees.  The resulting image will always be a circle, and this value determines how much of a field of view is covered by this circle.  Please note specifically that this also includes field-of-view settings >180, in which a larger distortion is applied to the image.  The default value is 180.

`quality` *optional* \[ low, medium, high, 256, 512, 1k, 1024, 1.5k, 1536, 2k, 2048, 4k, 4096, 8k, 8192, 16k, 16384 \]
> Determines the pixel resolution of the cube map faces that are reprojected to create the fisheye rendering.  The higher resolution these cube map faces have, the better quality the resulting fisheye rendering, but at the expense of increased rendering times.  The named values are corresponding:
> 
> - `low`: 256
> - `medium`: 512
> - `high`: 1024
> - `1k`: 1024
> - `1.5k`: 1536
> - `2k`: 2048
> - `4k`: 4096
> - `8k`: 8192
> - `16k`: 16384
> 
> The default value is 512.

`interpolation` *optional* \[ linear or cubic \]
> Determines the interpolation method that is used when reprojecting the cube maps into the final fisheye rendering.  The default value is "linear".

`diameter` *optional* \[ float > 0 \]
> Sets the diameter in meters for the "sphere" that the fisheye is reprojected based on.  This value is only used for stereoscopic rendering to compute the frustum offset for the [User](#user)s `eyeSeparation`.  The default value is 14.8.

`tilt` *optional*\[ float \]
> Determines the tilt of the "up vector" of the fisheye.  With a tilt of 0, the center of the fisheye image is the apex of the half-sphere that is used to reproject the cube map.  A tilted fisheye rendering is useful when projecting on a tilted planetarium dome.  The default value is 0.

`keepAspectRatio` *optional* \[ boolean \]
 > Determines whether the application should try to maintain the original aspect ratio when resizing the window or whether the field of view should be recalculated based on the window's new aspect ratio.  The default value is `true`.

### Children
`Crop` \[ 0 - 1 \]
 > This node can be used to crop the fisheye after the post processing has occurred.  This might be useful for domes running a single projector with a fisheye lens.  Normally a projector has a 16:9, 16:10, or 4:3 aspect ratio, but the fiehye output has a 1:1 aspect ratio.  This circle can be squared by cropping the 1:1 aspect ratio fisheye image down to the aspect ratio of the projector that is used.
 > 
 > `left` *optional* \[ 0 < float < 1 \]
 > > The ratio of the image that is cropped from the left.  If the value is 0, the image is not cropped at all from this side, if it is 1, the entire image is cropped. However, this cropping value must not be larger than the `1 - right` cropping value as these value might not overlap.  The default value is 0.
 > 
 > `right` *optional* \[ 0 < float < 1 \]
 > > The ratio of the image that is cropped from the right.  If the value is 0, the image is not cropped at all from this side, if it is 1, the entire image is cropped. However, this cropping value must not be larger than the `1 - left` cropping value as these value might not overlap.  The default value is 0.
 > 
 > `bottom` *optional* \[ 0 < float < 1 \]
 > > The ratio of the image that is cropped from the bottom.  If the value is 0, the image is not cropped at all from this side, if it is 1, the entire image is cropped. However, this cropping value must not be larger than the `1 - top` cropping value as these value might not overlap.  The default value is 0.
 > 
 > `top` *optional* \[ 0 < float < 1 \]
 > > The ratio of the image that is cropped from the top.  If the value is 0, the image is not cropped at all from this side, if it is 1, the entire image is cropped. However, this cropping value must not be larger than the `1 - bottom` cropping value as these value might not overlap.  The default value is 0.

`Offset` \[ 0 - 1 \]
 > A linear offset in meters that is added to the virtual planes used to project the fisheye rendering.  This can be used for off-axis projections.  Must define three float attributes `x`, `y`, and `z`.  The default value is `x=0`, `y=0`, `z=0`.

`Background` \[ 0 - 1 \]
 > This value determines the color that is used for the parts of the image that are not covered by the spherical fisheye image.  The alpha component of this color has to be provided even if the final render target does not contain an alpha channel.  All attributes `r`, `g`, `b`, and `a` must be defined and be between 0 and 1.  The default color is (0.3, 0.3, 0.3, 1.0).


## SphericalMirrorProjection
This node is used to create a projection used for Paul Bourke's spherical mirror setup (see [here](http://paulbourke.net/dome/)), which makes it possible to use an off-the-shelf projector to create a planetarium-like environment by bouncing the image of a shiny metal mirror.  Please note that this is not the only way to produce these kind of images.  Depending on your setup and availability of warping meshes, it might suffice to use the [FisheyeProjection](#fisheyeprojection) node type instead and add a single mesh to the parent [Viewport](#viewport) instead.  The `config` folder in SGCT contains an example of this using a default 16x9 warping mesh.  This projection type specifically deals with the case where you have four different meshes, one for the bottom, top, left, and right parts of the image.

### Attributes
`quality` *optional* \[ low, medium, high, 256, 512, 1k, 1024, 1.5k, 1536, 2k, 2048, 4k, 4096, 8k, 8192, 16k, 16384 \]
> Determines the pixel resolution of the cube map faces that are reprojected to create the spherical mirror projection rendering.  The higher resolution these cube map faces have, the better quality the resulting spherical mirror projection rendering, but at the expense of increased rendering times.  The named values are corresponding:
> 
> - `low`: 256
> - `medium`: 512
> - `high`: 1024
> - `1k`: 1024
> - `1.5k`: 1536
> - `2k`: 2048
> - `4k`: 4096
> - `8k`: 8192
> - `16k`: 16384
> 
> The default value is 512.

`tilt` *optional*\[ float \]
> Determines the tilt of the "up vector" of the spherical mirror projection.  With a tilt of 0, the center of the spherical mirror image is the apex of the half-sphere that is used to reproject the cube map.  The default value is 0.

### Children
`Background` \[ 0 - 1 \]
 > This value determines the color that is used for the parts of the image that are not covered by the spherical mirror image.  The alpha component of this color has to be provided even if the final render target does not contain an alpha channel.  All attributes `r`, `g`, `b`, and `a` must be defined and be between 0 and 1.  The default color is (0.3, 0.3, 0.3, 1.0).

`Geometry` \[ 1 \]
> Describes the warping meshes used for the spherical mirror projection.  All four warping meshes have to be present.
> 
> `bottom` \[ string \]
> > The path to the warping mesh that is loaded for the bottom part of the spherical mirror projection
> 
> `top` \[ string \]
> > The path to the warping mesh that is loaded for the bottom part of the spherical mirror projection
> 
> `left` \[ string \]
> > The path to the warping mesh that is loaded for the bottom part of the spherical mirror projection
> 
> `right` \[ string \]
> > The path to the warping mesh that is loaded for the bottom part of the spherical mirror projection


## SpoutOutputProjection
This projection method provides the ability to share individual cube map faces or a fully reprojected image using the [Spout](https://spout.zeal.co/) library.  This library only supports the Windows operating system, so this projection will only work on Windows machines.  Spout's functionality is the abilty to shared textures between different applications on the same machine, making it possible to render images using SGCT and making them available to other real-time applications on the same machine for further processing.  Spout uses a text name for accessing which texture should be used for sharing.  The SpoutOutputProjection has three different output types, outputting each cube map face, sharing a fisheye image, or sharing an equirectangular projection, as determined by the `mapping` attribute.

### Attributes
`quality` *optional* \[ low, medium, high, 256, 512, 1k, 1024, 1.5k, 1536, 2k, 2048, 4k, 4096, 8k, 8192, 16k, 16384 \]
> Determines the pixel resolution of the cube map faces.  The named values are corresponding:
> 
> - `low`: 256
> - `medium`: 512
> - `high`: 1024
> - `1k`: 1024
> - `1.5k`: 1536
> - `2k`: 2048
> - `4k`: 4096
> - `8k`: 8192
> - `16k`: 16384
> 
> The default value is 512.

`mapping` *optional* \[ fisheye, equirectangular, or cubemap \]
> Determines the type of sharing that occurs with this projection and thus how many and which texture is shared via Spout.  The default value is "cubemap".

`mappingSpoutName` *optional* \[ string \]
> Sets the name of the texture if the `mapping` type is "fisheye" or "equirectangular".  If the `mapping` is "cubemap", this value is ignored.

### Children
`Background` \[ 0 - 1 \]
 > This value determines the color that is used for the parts of the image that are not covered by the spherical mirror image.  The alpha component of this color has to be provided even if the final render target does not contain an alpha channel.  All attributes `r`, `g`, `b`, and `a` must be defined and be between 0 and 1.  The default color is (0.3, 0.3, 0.3, 1.0).

`Channels` \[ 0 - 1 \]
> Determines for the "cubemap" `mapping` type, which cubemap faces should be rendered and shared via spout.  The available boolean attributes are `Right`, `zLeft`, `Bottom`, `Top`, `Left`, `zRight`.  By default all 6 cubemap faces are enabled.

`Orientation` \[ 0 - 1 \]
 > Describes a fixed orientation for the cube, whose faces are either shared or used to reproject into the fisheye or equirectangular image.  This can be provided either as Euler angles or as a quaternion.  The two modes *cannot* be mixed.  The following descibes the different attributes that can be used for the orientation.  Please note that *all* attributes for the chosen method have to be specified.  If the `Matrix` attribute is also specified, it will overwrite the value specified here.
 > 
 > Euler angles:
 >  - `pitch` or `elevation`
 >  - `yaw`, `heading`, or `elevation`
 >  - `roll` or `bank`
 > 
 > Quaternion:
 >  - `x`
 >  - `y`
 >  - `z`
 >  - `w`
 > 
 >  Example:  `<Orientation pitch="20.0" elevation="-12.0" roll="0.0" />`


## Projectionplane
This projection method is based on providing three corner points that are used to construct a virtual image plane.  The lower left, upper left, and upper right points have to be provided as children of this node in this exact order.  Each child needs to have float attributes `x`, `y`, and `z`
