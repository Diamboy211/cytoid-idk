#!/bin/bash
CXX=g++
CFLAGS="-g -std=c++20"
SRC=src
BUILD=build

set -x
${CXX} ${CFLAGS} -o main main.cc music.cc bg.cc chart.cc notes.cc
./main | ffmpeg -y -f s16le -ar 48000 -ac 1 -i pipe:0 out.ogg
ffmpeg -y -i out.pbm bg.png
zip diamboy.4fprac.cytoidlevel level.json wtf.json out.ogg bg.png
set +x
