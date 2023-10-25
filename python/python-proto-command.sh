#!/bin/sh
# Standard /usr/include/... path
PROTO=remoteram/protobuf/remoteram.proto
# repository path
PROTO=../src/remoteram/protobuf/remoteram.proto

RP=../python/

mkdir -p $RP
python -m grpc_tools.protoc -I. --python_out=$RP --pyi_out=$RP --grpc_python_out=$RP remoteram/protobuf/remoteram.proto
