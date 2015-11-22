#!/usr/bin/env bash

RESULT_DIR=$PWD
TEST_DIR="/media/sf_MacHome/UbuntuResearch/CharcoalTesting/Testing/MicroTests"

echo $RESULT_DIR

RESULT_FILE="$RESULT_DIR/mem_limit.txt"
echo "NUMBERS!" > $RESULT_FILE

# Memory limit threads
cd "$TEST_DIR/Spawn"
# make clean

# make memory_limit_pthread_simple
# ../../Build/memory_limit_pthread_simple >> $RESULT_FILE

make memory_limit_activities
# ../../Build/memory_limit_activities >> $RESULT_FILE
# ../../Build/memory_limit_activities 62108864 >> $RESULT_FILE FAILED
# ../../Build/memory_limit_activities 61108864 >> $RESULT_FILE WORKED
# ../../Build/memory_limit_activities 61708864 >> $RESULT_FILE FAILED
# ../../Build/memory_limit_activities 61408864 >> $RESULT_FILE FAILED
../../Build/memory_limit_activities 61208864 >> $RESULT_FILE

# Spawn test
# make spawn_pthread
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_pthread 20 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_pthread 20 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_pthread 20 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_pthread 20 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_pthread 20 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# make spawn_activities
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_activities 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_activities 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_activities 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_activities 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/spawn_activities 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# Context switch
# cd "$TEST_DIR/BucketBrigade"
# make clean

# make bucket_brigade_pthread
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_pthread 22 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_pthread 22 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_pthread 22 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_pthread 22 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_pthread 22 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# make bucket_brigade
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# make bucket_brigade_libuv
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_libuv 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_libuv 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_libuv 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_libuv 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/bucket_brigade_libuv 27 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# Context switch
# cd "$TEST_DIR/JustCalling"
# make clean

# make rec
# /usr/bin/time -o TIME_TEMP ../../Build/rec 35 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec 35 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec 35 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec 35 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec 35 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# make rec_crcl
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl 31 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl 31 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl 31 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl 31 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl 31 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# make rec_crcl_no
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 4 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 4 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 4 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 4 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 4 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 8 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 8 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 8 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 8 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/rec_crcl_no 30 8 >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# strcmp
# cd "$TEST_DIR/strcmp"
# make clean

# make baseline_strcmp
# /usr/bin/time -o TIME_TEMP ../../Build/baseline_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/baseline_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/baseline_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/baseline_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/baseline_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE

# make yield_block_strcmp
# /usr/bin/time -o TIME_TEMP ../../Build/yield_block_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/yield_block_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/yield_block_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/yield_block_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
# /usr/bin/time -o TIME_TEMP ../../Build/yield_block_strcmp >> $RESULT_FILE
# cat TIME_TEMP >> $RESULT_FILE
