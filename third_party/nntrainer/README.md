# Building nntrainer for walrus

**This document and wasi-nn support is only intended for linux environments.**

*[Docker](https://www.docker.com/) is required to build nntrainer*

Since nntrainer is only available for specific ubuntu versions through apt it is built inside a docker container and then copied onto the system.

There is a script in the same directory as this file, called `build_nntrainer.sh`. You might need sudo to run the script.
It creates a docker image from the Dockerfile that is also found in the same directory as this file, then it builds nntrainer and copies the necesarry .so files to into the current folder.

After the .so files are copied you need to add the absolute directory they are located at to the `LD_LIBRARY_PATH` environment variables.

You can test if it works from the project root directory with the following:
```
walrus --mapdirs ./test/wasi-nn/ ./ --args test/wasi-nn/simple_fc_model.wasm test/wasi-nn/fc_model.ini
```

# Exploring build files

If you would like to access the build files you can do so by running a docker image, created by the `build_nntrainer.sh` script with the following command:
`docker run -it build_nntrainer`

# Supported model formats

- NNtrainer .ini files
