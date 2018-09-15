Build instuctions
=================

  The Traveler is based on a world file (.wld).  The most important component
  of the file are a set of plane list files (.pll).  There are three: One for
  each of the X, Y, and Z planes.

  tcledit is a world file editor for the Traveler.  You should be able to build
  it under Linux or Cygwin.  It needs X11 and Tcl/Tk.

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

    ./tcledit [-D <directory>] [-o <outfilename>] <infilename>

  Where <infilename> is the original world file name which will be overwritten
  unless <outfilename> is provided.  Optionally, switch to <directory> before
  opening <infilenamea>.

  NOTE: The default traveler world file is apps/graphics/traverler/world/transfrm.wld.
  The file contains relative paths so you may have to CD in to the directory first
  like:

    ./tcledit -D ../../world transfrm.wld

  On Cywgin, the correct name of the program will be tcledit.exe and must also
  remember to start the X11 server before trying run the applications.

  Saying that the UI is difficult to use would probably be an understatement.
  When you start tcledit, four windows appear:  Three X11 graphics windows and
  one Tcl/Tk edit window.  The four graphic windows present a view at the
  currently selected X, Y, and planes with a grid and positioning lines.  This
  gives an accurate rather incomprehensible view into the 3-dimensional world.

  At the top of Tcl/Tk window are three sliders that can be quickly used to
  generally position yourself in the world.  As you move a slider, the
  position indicator moves in the corresponding plane view window.  You can
  set position more precisely with the X, Y, and Z position data entry fields.

  The Tcl/Tk edit window also has controls to manage the plane at the selected
  position:  Add X, Y, or Z plane, Save data, Zoom in or out, etc.

  Chances are if you don't know where the planes are defined in the world, you
  won't even be able to find them.  Hint:  position to X=704, Y=704, and Z=576.
  There you should see something in the default world.  Trying zooming 8x or
  more.  The plane is shown in light blue; the edges of othogonal planes are
  shown in even lighter blue.
