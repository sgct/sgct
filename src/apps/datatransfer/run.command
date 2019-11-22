#!/bin/sh
cd "$(dirname "$0")"
./dataTransfer -config two_nodes.xml -local 0 &
./dataTransfer -config two_nodes.xml -local 1 --slave &
