#!/bin/bash

mkdir bin
cd lame3.70
make clean
make
mv lame ../bin
make clean
make -f Makefile.oi
mv lame ../bin/lame.oi
cd ..
