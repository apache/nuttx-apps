# Wasm build system in NuttX

## Overview

The files in this directory are used to build the Wasm module in NuttX:
* CMakeLists.txt - The project configuration file for all Wasm targets
* WASI-SDK.cmake - Provides the toolchain definition and utility functions for the Wasm build

Since the Wasm module is built with dedicated toolchain and flags, the main NuttX build system will treat the Wasm module as an external project, but share the same source tree and configuration (.config file).

## Design goals

* **Consistency**: The Wasm module can be built with the same build system as NuttX, and share the same source tree and configuration.
* **Flexibility**: Can be built with CMake or Makefile, and can be integrated into the NuttX build system easily (until Makefile based build system is deprecated)
* **Maintainability**: The Wasm module build system should be simple and less dependent on the NuttX build system.
* **Portability**: The Wasm module can be built on any platform with CMake and WASI-SDK installed.

## Build process

Each Wasm target (such as examples/hello_wasm) will have its own CMakeLists.txt file, which will be included in the main CMakeLists.txt file in this directory.

Each target will have its own build directory (Wasm) inside the NuttX build directory, such as:
```
* apps
* nuttx
* cmake_build_dir
  * other NuttX native targets
  * Wasm
    * examples
      * hello_wasm
        * hello_wasm.wasm
        * other build files
    * benchmarks
      * coremark
        * coremark.wasm
    * netutils
        * cjson
          * libcJSON.a
    * libWasm.a
    * other build files
```

Each target can be built with private source files, or shared source files with other targets. The shared source files will be built into a static global library (libWasm.a) or a custom library and linked to the Wasm targets.

Each target will be visible to other targets, so that the module level CMakelists.txt can define the dependencies between targets.

## Limitations

Now the Wasm module is targeted to wasm32-wasi, instead of legacy custom build with NuttX sysroot.
So the source files should not call any NuttX APIs directly, and should not include any NuttX header files.
It will limit the usage that some applications inside apps directory can not be built as Wasm targets directly.But still
can be used for many POSIX compatible applications.
