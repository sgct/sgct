#!/bin/sh
cd "$(dirname "$0")"
./OmniStereoTest_opengl3 -config test.xml -turnmap turnmap.jpg -sepmap sepmap.png
