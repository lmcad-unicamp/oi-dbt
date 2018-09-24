#!/bin/bash

mkdir bin
make
mv bitcnts bin
make -f Makefile.oi
mv bitcnts.oi bin
