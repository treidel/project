#!/bin/bash

# setup the variables for where things are located or destined
PROTOC=protoc
DEST_DIR=`pwd`
SRC_DIR=`pwd`/../proto

# call the protocol buffer compiler
${PROTOC} --proto_path=${SRC_DIR} --python_out=${DEST_DIR} ${SRC_DIR}/v1.proto

