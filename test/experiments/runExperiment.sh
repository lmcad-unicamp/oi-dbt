#!/bin/bash

oiclang=~/dev/mestrado/openisa/oi-toolchain/bin/clang

compile() {
  $oiclang -O0 $1 -o a0.out 
  $oiclang -O2 $1 -o a2.out 
  clang -O2 $1 -o native2
  clang -O0 $1 -o native0
}

executeDBT() {
  echo "executing native O0:"
  for i in $(seq 1 $3); do
    echo -ne "$2 O0 " >> nativeTime.csv
    perf stat -e instructions:u ./native0 2>&1 | tail -n 4 | head -n 1 | sed 's/\.//g' \
      | sed 's/ //g' | sed 's/instructions:u//g' >> nativeTime.csv
    echo  "$2.o0...done $i/$3"
  done;

  echo "executing native O2:"
  for i in $(seq 1 $3); do
    echo -ne "$2 O2 " >> nativeTime.csv
    perf stat -e instructions:u ./native2 2>&1 | tail -n 4 | head -n 1 | sed 's/\.//g' \
      | sed 's/ //g' | sed 's/instructions:u//g' >> nativeTime.csv
    echo "$2.o2...done $i/$3"
  done;

  echo "executing oi in o0 with $1:"
  for i in $(seq 1 $3); do
    echo -ne "$2 $1 O0\t" >> dbtTime.csv
    oi-dbt -bin a0.out -rft $1 2> /dev/null | cut -d':' -f2 | sed 's/Global//g' | sed 's/Compilation//g'\
      | sed '/^\s*$/d' | tail -n 12 | awk '{printf "%s ", $0} END {printf "\n"}' >> dbtTime.csv
    echo "$2.$1.o0...done $i/$3"
  done;

  echo "executing oi in o2 with $1:"
  for i in $(seq 1 $3); do
    echo -ne "$2 $1 O2\t" >> dbtTime.csv
    oi-dbt -bin a2.out -rft $1 2> /dev/null | cut -d':' -f2 | sed 's/Global//g' | sed 's/Compilation//g'\
      | sed '/^\s*$/d' | tail -n 12 | awk '{printf "%s ", $0} END {printf "\n"}' >> dbtTime.csv
    echo "$2.$1.o2...done $i/$3"
  done;
}

run_all_tests() {
  list_of_tests=$(ls *.c)

  for test in $list_of_tests; do
    echo -e "\e[1m>> Running test: $test\e[0m"
    if ! compile $test; then
      echo -e "\e[31mFailed during compilation\e[0m"
      clean;
      continue
    fi

    for rft in "net" "mret2" "netplus"; do
      executeDBT $rft $test 10;
    done;
  done;
}

rm nativeTime.csv
rm dbtTime.csv
run_all_tests;
rm native0 native2 a2.out a0.out
echo ">>> Done"
