#!/bin/sh
cd "$(dirname "$0")"
./touch -config two_nodes.xml -local 0 &
./touch -config two_nodes.xml -local 1 --slave &
