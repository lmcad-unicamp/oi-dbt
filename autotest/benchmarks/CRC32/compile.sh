#!/bin/bash

mkdir bin
make
mv crc bin
make -f Makefile.oi
mv crc.oi bin
