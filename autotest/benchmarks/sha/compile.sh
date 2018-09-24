#!/bin/bash

mkdir bin
make clean
make
mv sha bin
make clean
make -f Makefile.oi
mv sha bin/sha.oi
