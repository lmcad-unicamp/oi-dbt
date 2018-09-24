#!/bin/bash

rm -r bin
mkdir bin

cd benchmarks
for bench in $(ls); do
  cd $bench;
  ./compile.sh
  mv bin/* ../../bin
  cd ..;
done;

