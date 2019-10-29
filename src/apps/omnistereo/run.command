#!/bin/sh
cd "$(dirname "$0")"
./omnistereo -config test.xml -turnmap turnmap.jpg -sepmap sepmap.png
