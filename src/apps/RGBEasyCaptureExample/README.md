# RGBEasyCaptureExample
RGBEasyCaptureExample is a an application example which uses RGBEasy API from Datapath to capture input on the master or a client from Datapath capture cards.

It is based on SAMPLE5.c of the Datapath Vision SDK, an example that shows transfer between PCI-buses from capture card to pro GPU (Quadro or Firepro).
It has been adapted to work with an SGCT example app.
Also the concept of ganging was added, and has been tested with Datapath DP2 + 2x1 setup, meaning an SBS (side-by-side) texture.
The example does change the SBS layout to an TB (TopBottom) image during rendering.
Does low-latency 4096x4096x60 Hz capturing is achieved, with 2 inputs of 4096x2048x60 Hz each.
As captures seems to require running on the main thread, all threading was removed, as the rendering in the sample used a background thread for the OpenGL rendering.

Example capturing datapath dual link:
RGBEasyCaptureExample.exe -config single.xml -host localhost

Keyboard keys:
D - Fulldome mode
P - Plane mode
I - Toggle show info
S - Toggle show stats 