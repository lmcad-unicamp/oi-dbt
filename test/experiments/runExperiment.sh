#!/bin/bash

oiclang=~/dev/mestrado/openisa/oi-toolchain/bin/clang

compile() {
  $oiclang -O3 -lm -w $1 -o a2.out 
  clang-4.0 -O2 -lm -w $1 -o native2

  # SBT
  clang -target mipsel-unknown-linux -emit-llvm -c --sysroot=/home/vanderson/dev/mestrado/openisa/newlib/newlib-2.1.0/newlib/libc/ -O2 -mllvm -disable-llvm-optzns $1 -o test.bc > /dev/null
  opt -O2 test.bc -o test.bc
  llc -relocation-model=static -O2 -march=mipsel test.bc -o test.s > /dev/null
  llvm-mc -triple mipsel-unknown-linux -assemble -filetype=obj test.s -o test-oi.o > /dev/null
  static-bt -target=x86 -optimize -oneregion -stacksize 4000 test-oi.o -o=test-oi.bc > /dev/null
  llc -stats -relocation-model=static -O2 -march=x86 -mattr=avx2 test-oi.bc -o test-oi.s > /dev/null
  clang-4.0 -target x86_64-unknown-linux-gnu -m32 -g test-oi.s -o test-oi-x86 -lm > /dev/null
  sbtpass2 test-oi-x86
}

executeSBT() {
  echo "executing oi-sbt:"
#  for i in $(seq 1 $3); do
    echo -ne "$1 O2 ";
    echo $(perf stat -e instructions:u,cycles:u -r20 ./test-oi-x86 > /dev/null);
    echo "$1.o2...done";
#  done;
}

executeDBT() {
#
#  echo "executing oi in o0 with $1:"
#  for i in $(seq 1 $3); do
#    echo -ne "$2 $1 O0\t" >> dbtTime.csv
#    oi-dbt -bin a0.out -rft $1 2> /dev/null | cut -d':' -f2 | sed 's/Global//g' | sed 's/Compilation//g'\
#      | sed '/^\s*$/d' | tail -n 12 | awk '{printf "%s ", $0} END {printf "\n"}' >> dbtTime.csv
#    echo "$2.$1.o0...done $i/$3"
#  done;

  echo "executing native O2:"
  for i in $(seq 1 $3); do
    echo -ne "$2 O2 " >> nativeTime.csv
    oi-int -bin ./a2.out -interpret 2>&1 | tail -n1 | sed "s/TotalInst://g" >> nativeTime.csv
    echo "$2.o2...done $i/$3"
  done;

#  echo "executing oi in o2 with $1:"
#  for i in $(seq 1 $3); do
#    echo -ne "$2 $1 O2\t" >> dbtTime.csv
#    oi-dbt -bin a2.out -rft $1 2> /dev/null | cut -d':' -f2 | sed 's/Global//g' | sed 's/Compilation//g'\
#      | sed '/^\s*$/d' | tail -n 12 | awk '{printf "%s ", $0} END {printf "\n"}' >> dbtTime.csv
#    echo "$2.$1.o2...done $i/$3"
#  done;
}

run_all_tests() {
  list_of_tests=$(ls *.c)

  for test in "n-body.c" "pi.c"; do #$list_of_tests; do
    echo -e "\e[1m>> Running test: $test\e[0m"
    if ! compile $test; then
      echo -e "\e[31mFailed during compilation\e[0m"
      clean;
      continue
    fi

    for rft in "net"; do # "netplus" "lei" "lef"; do
      executeSBT $test;
    done;
  done;
}

rm nativeTime.csv
rm dbtTime.csv
run_all_tests;
rm native0 native2 a2.out a0.out
echo ">>> Done"
