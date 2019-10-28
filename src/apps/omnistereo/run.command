#!/bin/sh
cd "$(dirname "$0")"
./OmniStereoTest -config test.xml -turnmap turnmap.jpg -sepmap sepmap.png
