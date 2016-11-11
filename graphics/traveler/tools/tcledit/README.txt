Build instuctions
=================

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

  Build the world library:

    5. cd apps/graphics/traveler/tools/libwld
    6. make

  Then you can use xmfmk to create the Makefile and build the tool:

    7. cd apps/graphics/traveler/tools/tcledit
    8. Review Imakefile.  You will probabaly to to change the APPDIR and TOPDIR paths
       a minimum.  These are the paths to where you have clones the apps/ repository
       and the nuttx/ repositories, respectively.
    9. xmfmk
   10. make tcledit

Usage
=====

   ./tcledit [-o <outfilename>] <infilename>

   Where <infilename> is the original world file name which will be overwritten
   unless <outfilename> is provided.
