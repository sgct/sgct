# Commandline arguments
Commandline arguments are parsed through the `parseArguments` function contained in the `commandline.h` file.  The commandline arguments that are accepted by this function are listed below:

 > `-config` \[ string \]
 >  > Sets the XML configuration file that should be loaded by the application to use as the cluste system.  This path should be either absolute, or relative to the current working directory.

 > `-help`
 >  > Displays the help message listing all available commandline arguments supported by SGCT

 > `-local` \[ integer \]
 >  > Forces the node configuration to be `localhost`.  The index that is provided is the index that is used for indexing multiple node settings in the cluster setup.  Index `0` relates to `127.0.0.1`, index `1` is `127.0.0.2`, etc.

 > `-client`
 >  > Sets the node to run as a client.  This is only available when running as a local node, as otherwise the IP address is used to set the client and server setup.

 > `-debug`
 >  > Set the notification level of the Log to include `Debug` level messages

 > `-firm-sync`
 >  > Enables a firm frame synchronization using whatever method that is available, overwriting the settings that are provided in the cluster configuration

 > `-loose-sync`
 >  > Disables the firm frame synchronization, overwriting the settings that are provided in the cluster configuration

 > `-ignore-sync`
 >  > Disables the frame synchronization entirely, which will completely decouple the rendering nodes.  This should only be set if you are absolutely sure you know what you are doing

 > `-notify` \[ error warning info debug  \]
 >  > Sets the notification level of the Log to include only the messages of the chosen log level or higher.  Setting this value to `debug` achieves the same as setting the `-debug` commandline flag

 > `-capture-jpg`
 >  > Sets the capture format for screenshots to output JPGs files instead of the default PNG format.

 > `-capture-tga`
 >  > Sets the capture format for screenshots to output TGA files instead of the default PNG format.

 > `-export-correction-meshes`
 >  > If this value is provided, any warping mesh that is loaded for a viewport will be exported as an Waveform `obj` file while processing.  This allows debugging of the warping meshes for tools that only support loading of `obj` files.

 > `-number-capture-threads` \[ integer \]
 >  > Sets the maximum number of threads that should be used for frame capturing

 > `-screenshot-path` \[ string \]
 >  > Sets the file path for the screenshots location

 > `-screenshot-prefix` \[ string \]
 >  > Sets the prefix used for the screenshots taken by this application.  The default value for the prefix is "SGCT"

 > `-add-node-name-in-screenshot`
 >  > If set, screenshots will contain the name of the node in multi-node configurations.  The default value is that the numerical identifier for the node is not used in the screenshots.

 > `-omit-node-name-in-screenshot`
 >  > If set, screenshots will not contain the name of the window if multiple windows exist.  The default is that the name of the window is included.
