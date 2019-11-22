#!/bin/sh
cd "$(dirname "$0")"
./domeImageViewer -config two_nodes.xml -local 0 &
./domeImageViewer -config two_nodes.xml -local 1 --slave &
