#!/bin/bash

mkdir bin
make
mv search_small search_large bin
make clean
make -f Makefile.oi
mv search_small bin/search_small.oi
mv search_large bin/search_large.oi
