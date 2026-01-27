FROM ubuntu:22.04 AS build_nntrainer
VOLUME ["/data"]
WORKDIR /src

RUN apt-get update && apt-get install --yes software-properties-common
RUN add-apt-repository ppa:nnstreamer && apt update && apt install --yes nntrainer tensorflow2-lite-dev
RUN apt-get install --yes gcc-12 g++-12 git 
RUN apt-get install --yes meson ninja-build
RUN apt-get install --yes gcc g++ pkg-config libopenblas-dev libiniparser-dev libjsoncpp-dev libcurl3-dev tensorflow2-lite-dev nnstreamer-dev libglib2.0-dev libgstreamer1.0-dev libgtest-dev ml-api-common-dev flatbuffers-compiler ml-inference-api-dev
RUN apt-get install --yes protobuf-compiler libprotobuf-dev

RUN git clone --recurse-submodules --shallow-submodules https://github.com/nntrainer/nntrainer.git

WORKDIR /src/nntrainer
RUN meson build
RUN ninja -C build
RUN cp -r /src /data
