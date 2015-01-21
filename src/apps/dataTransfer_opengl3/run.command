#!/bin/sh
cd "$(dirname "$0")"
./dataTransfer_opengl3 -config two_nodes.xml -local 0 &
./dataTransfer_opengl3 -config two_nodes.xml -local 1 --slave &
