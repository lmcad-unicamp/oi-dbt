#!/bin/bash

mkdir bin
make
mv susan bin
make clean
make -f Makefile.oi
mv susan bin/susan.oi
