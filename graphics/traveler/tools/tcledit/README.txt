Build instuctions
=================

  tcledit is a world editor for the traveler.  You should be able to build it
  under Linux or Cygwin.  It needs X11 and Tcl/Tk.

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
    6a. make

  If you want to create a debug-able version of the library, do:

    6b. make DEBUG_LEVEL=1

  Then you can use xmfmk to create the Makefile and build the tool:

    7. cd apps/graphics/traveler/tools/tcledit
    8. Review Imakefile.  You will probabaly to to change the APPDIR and TOPDIR paths
       a minimum.  These are the paths to where you have clones the apps/ repository
       and the nuttx/ repositories, respectively.
    9. xmfmk
   10a. make tcledit

  If you want to create a debug-able version of tcledit, do:

   10b. make tcledit DEBUG_LEVEL=1

  On Cygwin, the make target will be tcledit.exe, not tcledit.

Usage
=====

  .  /tcledit [-D <directory>] [-o <outfilename>] <infilename>

  Where <infilename> is the original world file name which will be overwritten
  unless <outfilename> is provided.  Optionally, switch to <directory> before
  opening <infilenamea>.

  NOTE: The default traveler world file is apps/graphics/traverler/world/transfrm.wld.
  The file contains relative paths so you may have to CD in to the directory first
  like:

    ./tcledit -D ../../world transfrm.wld

  On Cywgin, the correct name of the program will be tcledit.exe and must also
  remember to start the X11 server before trying run the applications.
