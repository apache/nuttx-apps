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
  4. xmfmk
  5. make


