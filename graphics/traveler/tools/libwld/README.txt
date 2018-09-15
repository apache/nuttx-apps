Build instuctions:

  At the time of 'make', you must have a valid Traveler configuration instantiated
  in the NuttX directory.  This is because the build will depend on certain
  configurations (such as color format).

    1. cd nuttx/tools
       ./configure.sh sim/traveler (for example)
    2. cd ..
       tools/sethost.sh -w or -l
       make context

  Prepare some header files.  This is necessary because we must use ALL of the
  toolchain header files except for a few files from nuttx (for example,
  config.h).

    3. cd apps/graphics/traveler/tools/nuttx
    4. make TOPDIR=<nuttx directory>

  Then you can build the world library:

    5. cd apps/graphics/traveler/tools/libwld
    6a. make

  If you want to create a debug-able version of the library, do:

    6b. make DEBUG_LEVEL=1

