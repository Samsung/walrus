# WALRUS: WebAssembly Lightweight RUntime

This project aims to provide a lightweight WebAssembly runtime engine. It now fully supports WebAssembly specs with an simple interpter, but we plan to optimize interpreting as well as adopting JIT compiler for better performance.

## Cloning

Clone as normal, but don't forget to get the submodules as well:

```console
$ git clone --recursive https://github.com/Samsung/walrus
$ cd walrus
$ git submodule update --init
```

This will fetch the testsuite and gtest repos, which are needed for some tests.

## Building using CMake

You'll need [CMake](https://cmake.org). You can then run CMake, the normal way:

```console
$ mkdir build
$ cd build
$ cmake ..
$ cmake --build .
```

This will produce build files using CMake's default build generator. Read the
CMake documentation for more information.
