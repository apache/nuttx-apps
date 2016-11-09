Build instuctions:

  At the time of 'make', you must have a valid Traveler configuration instantiated
  in the NuttX directory.  This is because the build will depend on certain
  configurations (such as color format).

    1. cd nuttx/tools
       ./configure.sh sim/traveler (for example)
    2. cd ..
       tools/sethost.sh -w or -l
       make context

  Then you can use xmfmk to create the Makefile and build the tool:

    3. cd apps/graphics/traveler/tools/tcledit
    4. Review Imakefile.  You will probabaly to to change the APPDIR and TOPDIR paths
       a minimum.  These are the paths to where you have clones the apps/ repository
       and the nuttx/ repositories, respectively.
    5. xmfmk
    6. make



