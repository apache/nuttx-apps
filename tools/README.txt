NxWidgets/tools README File
===========================

addobjs.sh
----------

  $0 will add all object (.o) files in directory to an archive.

  Usage: tools/addobjs.sh [OPTIONS] <lib-path> <obj-dir>

  Where:
    <lib-path> is the full, absolute path to the library to use
    <obj-dir> is full path to the directory containing the object files to be added
  OPTIONS include:
    -p Prefix to use.  For example, to use arm-elf-ar, add '-p arm-elf-'
    -w Use Windows style paths instead of POSIX paths
    -d Enable script debug
    -h Show this usage information

bitmap_converter.py
-------------------

  This script converts from any image type supported by Python imaging library to
  the RLE-encoded format used by NxWidgets.

  RLE (Run Length Length) is a very simply encoding that compress quite well
  with certain kinds of images:  Images that that have many pixels of the
  same color adjacent on a row (like simple graphics).  It does not work well
  with photographic images.

  But even simple graphics may not encode compactly if, for example, they have
  been resized.  Resizing an image can create hundreds of unique colors that
  may differ by only a bit or two in the RGB representation.  This "color
  smear" is the result of pixel interpolation (and might be eliminated if
  your graphics software supports resizing via pixel replication instead of
  interpolation).

  When a simple graphics image does not encode well, the symptom is that
  the resulting RLE data structures are quite large.  The pallette structure,
  in particular, may have hundreds of colors in it.  There is a way to fix
  the graphic image in this case.  Here is what I do (in fact, I do this
  on all images prior to conversion just to be certain):

  - Open the original image in GIMP.
  - Select the option to select the number of colors in the image.
  - Pick the smallest number of colors that will represent the image
    faithfully.  For most simple graphic images this might be as few as 6
    or 8 colors.
  - Save the image as PNG or other lossless format (NOT jpeg).
  - Then generate the image.

indent.sh
---------

  This script uses the Linux 'indent' utility to re-format C source files
  to match the coding style that I use.  It differs from my coding style in that

  - I normally put the trailing */ of a multi-line comment on a separate line,
  - I usually align things vertically (like '='in assignments.

install.sh
----------

  Install a unit test in the NuttX source tree"

  USAGE: tools/install.sh <apps-directory-path> <test-sub-directory>

  Where:
    <apps-directory-path> is the full, absolute path to the NuttX apps/ directory
    <test-sub-directory> is the name of a sub-directory in the UnitTests directory

zipme.sh
--------

  Pack up the NxWidgets tarball for release.

  USAGE:  tools/zipme.sh <version>
