#!/bin/sh
cd "$(dirname "$0")"
./FFmpegCaptureAndDomeImageViewer -config two_nodes.xml -local 0 &
./FFmpegCaptureAndDomeImageViewer -config two_nodes.xml -local 1 --slave &
