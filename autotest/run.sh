#!/bin/bash

benchmark=(
  "bitcnts"
  "cjpeg"
  "djpeg"
  "crc"
  "dijkstra_large"
  "fft"
  "fft"
  "lame"
  "rawcaudio"
  "rawdaudio"
  "rijndael"
  "rijndael"
  "search_large"
  "sha"
  "susan"
  "susan"
  "susan"
  )

param=(
  "1125000"
  "-dct int -progressive -opt -outfile outputs/output_large_encode.jpeg inputs/input_large.ppm"
  "-dct int -ppm -outfile outputs/output_large_decode.ppm inputs/input_large.jpg"
  "inputs/large.pcm"
  "inputs/input.dat"
  "8 32768"
  "8 32768 -i"
  "inputs/large.wav outputs/output_large.mp3"
  "inputs/large.pcm"
  "inputs/large.adpcm"
  "inputs/input_large.asc outputs/output_large.enc e 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"
  "inputs/output_large.enc outputs/output_large.dec d 1234567890abcdeffedcba09876543211234567890abcdeffedcba0987654321"
  ""
  "inputs/input_large.asc"
  "input_large.pgm output_large.smoothing.pgm -s"
  "input_large.pgm output_large.edges.pgm -e"
  "input_large.pgm output_large.corners.pgm -c"
  )

NUM_OF_EXECS=10

NATIVE_TIMES[0]=0

rm -r outputs
mkdir outputs
rm native.times
for i in $(seq 0 15); do
  echo "Natively running: \"${benchmark[$i]}\" \"${param[$i]}\""
  rt=0
  execs=0
  for j in $(seq 1 $NUM_OF_EXECS); do
    my-time ./bin/${benchmark[$i]} ${param[$i]} > outputs/${benchmark[$i]}.$i.out 2>&1 && let rt=$rt+$(cat outputs/${benchmark[$i]}.$i.out | tail -n1) && let execs=$execs+1 || echo "Something went wrong!"
  done;
  echo "\"${benchmark[$i]}\"; \"${param[$i]}\"; $(( $rt/$execs ))" >> native.times
  NATIVE_TIMES[$i]=$(( $rt/$execs ))
done;

rm dbt.times
for i in $(seq 0 15); do
  for RFT in "mret2" "net" "net-r" "netplus" "netplus-c" "netplus-e-r" "netplus-e-r-c"; do
    echo "DBT running with $RFT: \"${benchmark[$i]}\" \"${param[$i]}\""
    rt=0
    execs=0
    for j in $(seq 1 $NUM_OF_EXECS); do
      my-time oi-dbt -bin ./bin/${benchmark[$i]}.oi -args "${param[$i]}" > outputs/${benchmark[$i]}.$i.out 2>&1 && let rt=$rt+$(cat outputs/${benchmark[$i]}.$i.out | tail -n1) && let execs=$execs+1 || echo "Something went wrong!"
    done;
    echo "\"${benchmark[$i]}\"; \"${param[$i]}\"; $RFT; $(( 100 * $(( $rt/$execs )) / ${NATIVE_TIMES[$i]} ))" | sed 's/..$/.&/' >> dbt.times
  done;
done;
