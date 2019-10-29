#!/bin/sh
cd "$(dirname "$0")"
./network -port 27000 --server &
./network -port 27000 -address 127.0.0.1 &