apps/interpreters README file
=============================

This apps/ directory is set aside to hold interpreters that may be
incorporated into NuttX.

ficl
----

  This is DIY port of Ficl (the "Forth Inspired Command Language").  See
  http://ficl.sourceforge.net/.  It is a "DIY" port because the Ficl source
  is not in that directory, only an environment and instructions that will
  let you build Ficl under NuttX.  The rest is up to you.

pcode
-----

  At present, only the NuttX Pascal add-on is supported.  This NuttX add-on
  must be downloaded separately (or is available in an GIT snapshot in the
  misc/pascal directory).

  This Pascal add-on must be installed into the NuttX apps/ directory.  After
  unpacking the Pascal add-on package, an installation script and README.txt
  instructions can be found at pascal/nuttx.

  INSTALL.sh -- The script that performs the operation.  Usage:

     ./INSTALL.sh [-16|-32] <install-dir>

      If you are using this standard NuttX apps/ package, the correct
      location for the <install-dir> is apps/interpreters.  That is
      where the examples and build logic will expect to find the pcode
      sub-directory.

    Example:

      ./INSTALL.sh -16 $PWD/../../../apps/interpreters

    After installation, the NuttX apps/interpresters directory will contain
    the following files

      pcode
      |-- Makefile
      |-- include
      |   `-- Common header files
      |-- libboff
      |   `-- Pascal object format (POFF) library
      `--insn
          |-- include
          |   `-- model-specific header files
          `-- prun
              `-- model-specific source files

  pashello

    There is a simple Pascal example at apps/examples/pashello.  This is the
    standard "Hello, World!" example written in Pascal and interpreted from
    Pascal P-Code at runtime.  To use this example, place the following in
    your defonfig file:

      CONFIG_EXAMPLES_PASHELLO=y
      CONFIG_INTERPRETERS_PCODE=y

prun

  This directory holds some simple, convenience functions to simplify and
  standardize the interaction with the P-Code library.
