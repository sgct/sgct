#!/bin/sh
cd "$(dirname "$0")"
./touchExample -config two_nodes.xml -local 0 &
./touchExample -config two_nodes.xml -local 1 --slave &
