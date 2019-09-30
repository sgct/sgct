#!/bin/sh
cd "$(dirname "$0")"
./networkExample -config ../../config/single.xml -port 27000 --server &
./networkExample -config ../../config/single.xml -port 27000 -address 127.0.0.1 &