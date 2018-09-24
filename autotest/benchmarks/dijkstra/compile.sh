#!/bin/bash

mkdir bin
make
mv dijkstra_small dijkstra_large bin
make -f Makefile.oi
mv dijkstra_small.oi dijkstra_large.oi bin
