#!/bin/bash

oiclang=~/dev/mestrado/openisa/oi-toolchain/bin/clang

to_test="net"

compile() {
  $oiclang -O0 $1 -o a0.out 
  $oiclang -O1 $1 -o a1.out 
  clang -O1 $1 -o native
}

execute() {
  echo "Executing native:"
  ./native > /dev/null
  n=$?

  echo "Executing OI in O0 with interpreter:"
  oi-dbt -bin a0.out -interpret > /dev/null
  aux=$?
  if [[ "$n" -ne "$aux" ]]; then return 1; fi;

#  echo "Executing OI in O1 with interpreter:"
#  oi-dbt -bin a1.out -interpret > /dev/null
#  aux=$?
#  if [[ "$n" -ne "$aux" ]]; then return 1; fi;

  echo "Executing OI in O0 with DBT:"
  oi-dbt -bin a0.out > /dev/null
  aux=$?
  if [[ "$n" -ne "$aux" ]]; then return 1; fi;

#  echo "Executing OI in O1 with DBT:"
#  oi-dbt -bin a1.out > /dev/null 
#  aux=$?
#  if [[ "$n" -ne "$aux" ]]; then return 1; fi;
}

clean() {
  rm a0.out a1.out native
}

run_all_tests() {
  list_of_tests=$(ls *.c)

  oks=0
  total=0
  for test in $list_of_tests; do
    let total=$total+1
    echo -e "\e[1m>> Running test: $test\e[0m"
    if ! compile $test; then
      echo -e "\e[31mFailed during compilation\e[0m"
      clean;
      continue
    fi
    if ! execute; then
      echo -e "\e[31mFailed during execution\e[0m"
      clean;
      continue
    fi
    echo -e ">> \e[34m[OK] $test\e[0m"
    clean;
    let oks=$oks+1
  done;
  echo -e "\e[1m>>> Passed $oks/$total tests\e[0m"
}

echo ">>> Running Black Boxes tests"

if [ $# -eq 1 ]; then
  to_test=$1
fi;

run_all_tests;
echo ">>> Done"
