README
======

  Performs a file-based test on a SMART (or any) filesystem. Validates
  seek, append and seek-with-write operations.

    * CONFIG_TESTING_SMART_TEST=y

  Dependencies:

    * CONFIG_NSH_BUILTIN_APPS=y: This test can be built only as an NSH
      command

    Source: NuttX
    Author: Ken Pettit
    Date: April 24, 2013

  Performs a file-based test on a SMART (or any) filesystem. Validates seek,
  append and seek-with-write operations.

  Usage:

    flash_test mtdblock_device

  Additional options:

    --force                     to replace existing installation
