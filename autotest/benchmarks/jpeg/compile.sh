#!/bin/bash

mkdir bin
cd jpeg-6a
make clean
make -j4
mv cjpeg djpeg ../bin/
make clean
make -f Makefile.oi -j4
mv cjpeg ../bin/cjpeg.oi
mv djpeg ../bin/djpeg.oi
make clean 
cd ..
