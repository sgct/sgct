#!/bin/sh
cd "$(dirname "$0")"
./networkExample_opengl3 -config ../../config/single.xml -port 27000 --server &
./networkExample_opengl3 -config ../../config/single.xml -port 27000 -address 127.0.0.1 &