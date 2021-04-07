## Prerequisites

You must be using a linux-based operating system (Ubuntu, Mint, Arch, etc) and have the following installed:

* opencv 4
* v4l2-loopback-dkms
* libboost-all-dev
* pkg-config
* CMake
* [Tensorflow C API](https://www.tensorflow.org/install/lang_c) 

## Building

In the root repo directory, run `cmake .` to generate a makefile. Then simply run `make`, which will produce a binary called `whiteboard_pal` in the root repo directory. The generated makefile contains other targets, for example `make clean` which will clean all intermediate / cached build artifacts. To see more targets, look at the generated Makefile.

## Overview

in `include/whiteboard_pal` is where all definitions that multiple files need should go. This is where opencv headers are imported, and at the moment only certain modules are imported. If you need to use antoher, simply add it to the `OPENCV_HDRS` definition at the top of `include/whiteboard_pal/main.hpp`.

`src/models.cpp` is where CV / ML code are supposed to go. At the moment, the functions defined there are merely stubs and the pipeline still has yet to be fully ported to C++. While I do that, feel free to re-implement your models and call them in `src/main.cpp` as you see fit for testing purposes.
