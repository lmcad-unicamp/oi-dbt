#!/bin/bash

mkdir bin
make clean
make
mv rijndael bin
make clean
make -f Makefile.oi
mv rijndael bin/rijndael.oi
