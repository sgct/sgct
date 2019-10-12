#!/bin/sh
cd "$(dirname "$0")"
./networkExample -port 27000 --server &
./networkExample -port 27000 -address 127.0.0.1 &