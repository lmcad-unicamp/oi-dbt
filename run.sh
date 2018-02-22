#!/bin/bash

SRC_PATH=./test
RES_PATH=~/results_2
mkdir -p ${RES_PATH}

GREEN='\033[0;32m'
NC='\033[0m' # No Color

declare -a rfts=("net")
#declare -a rfts=("net" "mret2" "lef" "lei" "netplus")
declare -a hots=("1" "5" "10" "50" "75" "100" "200" "350" "500" "1000")
TIMES=7

#set -x

for file in "${SRC_PATH}"/*.c ;
do
  IFS='/' read -r -a array <<< "$file"
  IFS='.' read -r -a array <<< "${array[2]}"

  echo -e ${GREEN}'Compiling file: '${file} ${NC}
  /home/napoli/Documents/OpenISA/oitools/oi-toolchain/bin/clang -O2 ${file} -o ${SRC_PATH}/bin/${array[0]}

  for rft in "${rfts[@]}";
  do
    for hot in "${hots[@]}";
    do
      echo -e ${GREEN}'Executing' ${array[0]}  'with RFT: '${rft}' and hotness: '${hot}' ('${i}')'${NC}
      for i in $(seq 1 $TIMES);
      do
        #build/oi-dbt -bin ${SRC_PATH}/bin/bubblesort -rft ${rft} -hot ${hot}

        build/oi-dbt -bin ${SRC_PATH}/bin/${array[0]} -rft ${rft} -hot ${hot} -report ${RES_PATH}/${array[0]}_${rft}_${hot}_${i}
      done
      echo -e ${GREEN}'Finalized execution of' ${array[0]} ${NC}
    done
  done
done
