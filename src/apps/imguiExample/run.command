#!/bin/sh
cd "$(dirname "$0")"
./imguiExample -config two_nodes.xml -local 0 &
./imguiExample -config two_nodes.xml -local 1 --slave &
