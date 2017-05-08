#!/bin/bash

oiclang=~/dev/mestrado/openisa/oi-toolchain/bin/clang


compile() {
  $oiclang -O0 $1 -o a0.out 
  $oiclang -O1 $1 -o a1.out 
  $oiclang -O2 $1 -o a2.out 
  clang -O1 $1 -o native
}

executeDBT() {
  to_test=$1
  echo "executing native:"
  ./native > /dev/null
  n=$?

  echo "executing oi in o0 with $to_test:"
  oi-dbt -bin a0.out -rft $to_test > /dev/null
  aux=$?
  if [[ "$n" -ne "$aux" ]]; then return 1; fi;

  echo "executing oi in o1 with dbt $to_test:"
  oi-dbt -bin a1.out -rft $to_test> /dev/null
  aux=$?
  if [[ "$n" -ne "$aux" ]]; then return 1; fi;

  echo "executing oi in o2 with dbt $to_test:"
  oi-dbt -bin a2.out -rft $to_test > /dev/null
  aux=$?
  if [[ "$n" -ne "$aux" ]]; then return 1; fi;
}

executeInterpret() {
  echo "executing native:"
  ./native > /dev/null
  n=$?

  echo "executing oi in o0 with interpreter:"
  oi-dbt -bin a0.out -interpret > /dev/null
  aux=$?
  if [[ "$n" -ne "$aux" ]]; then return 1; fi;

  echo "executing oi in o1 with interpreter:"
  oi-dbt -bin a1.out -interpret > /dev/null
  aux=$?
  if [[ "$n" -ne "$aux" ]]; then return 1; fi;

  echo "executing oi in o2 with interpreter:"
  oi-dbt -bin a2.out -interpret > /dev/null
  aux=$?
  if [[ "$n" -ne "$aux" ]]; then return 1; fi;
}

clean() {
  rm a2.out native
}

run_all_tests() {
  list_of_tests=$(ls *.c)

  oks=0
  total=0
  failed=0
  for test in $list_of_tests; do
    echo -e "\e[1m>> Running test: $test\e[0m"
    if ! compile $test; then
      echo -e "\e[31mFailed during compilation\e[0m"
      clean;
      continue
    fi

    let total=$total+1
    if ! executeInterpret; then
      echo -e "\e[31mFailed during interpretation\e[0m"
    else
      echo -e ">> \e[34m[OK] $test\e[0m"
      let oks=$oks+1
    fi

    for rft in "net" "mret2"; do
      let total=$total+1
      if ! executeDBT $rft; then
        echo -e "\e[31mFailed during DBT ($rft)\e[0m"
      else
        echo -e ">> \e[34m[OK] $test ($rft)\e[0m"
        let oks=$oks+1
      fi
    done;
    clean;
  done;
  echo -e "\e[1m>>> Passed $oks/$total tests\e[0m"
}

echo ">>> Running Black Boxes tests"

if [ $# -eq 1 ]; then
  to_test=$1
fi;

run_all_tests;
echo ">>> Done"
