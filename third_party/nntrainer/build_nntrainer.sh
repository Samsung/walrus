#!/bin/bash
docker inspect build_nntrainer &> /dev/null
if [ $? -ne 0 ]; then
  docker builder build ./ -t build_nntrainer
  docker create --name build_nntrainer build_nntrainer
fi

docker cp build_nntrainer:/src/nntrainer/build/nntrainer/libnntrainer.so ./
docker cp build_nntrainer:/src/nntrainer/build/api/ccapi/libccapi-nntrainer.so ./
