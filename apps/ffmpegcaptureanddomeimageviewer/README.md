# FFmpegCaptureAndDomeImageViewer
FFmpegCaptureAndDomeImageViewer is a an application example which combines the FFmpegCapture example with the domeImageViewer example with the added functionality of chroma keying on the captured input. This example can be used as a basic but useful dome presentation tool.

Arguments:

-video <device name>
-host <ip/name of the node/master which captures input>
-option <key> <val>
-flip
-plane <azimuth> <elevation> <roll>

To obtain video device names in windows use:
ffmpeg -list_devices true -f dshow -i dummy

for mac:
ffmpeg -f avfoundation -list_devices true -i ""
 
to obtain device properties in windows use:
ffmpeg -f dshow -list_options true -i video=<device name>

For options look at: http://ffmpeg.org/ffmpeg-devices.html

Example capturing datapath dual link:
FFmpegCaptureAndDomeImageViewer.exe -config fisheye.xml  -host localhost -video "Datapath VisionDVI-DL Video 01" -option pixel_format bgr24 -option framerate 60 -flip

Keyboard keys:
D - Fulldome mode
P - Plane mode
I - Toggle show info
S - Toggle show stats 