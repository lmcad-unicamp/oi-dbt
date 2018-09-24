#!/bin/bash

mkdir bin
make clean
make
mv fft bin
make clean
make -f Makefile.oi
mv fft.oi bin
