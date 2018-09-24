#!/bin/bash

mkdir bin
cd src
make clean
make
make clean
make -f Makefile.oi
cd ..
