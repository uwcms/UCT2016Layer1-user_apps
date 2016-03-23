#!/bin/sh

source ./env.sh

folder=$1
start_bx=$2

mkdir -p $folder

./request_io_capture $start_bx && ./download_input_capture $folder $start_bx && ./download_output_capture $folder $start_bx && ./download_tower_mask $folder

