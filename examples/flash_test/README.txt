
Install Program
===============

    Source: NuttX
    Author: Ken Pettit
    Date: April 24, 2013

This application performs a SMART flash block device test.  This test performs a sector allocate, read, write, free and garbage collection test on a SMART MTD block device.  This test can be built only as an NSH command

NOTE:  This test uses internal OS interfaces and so is not available in the NUTTX kernel build

Usage:
    flash_test mtdblock_device

Additional options:

    --force                     to replace existing installation
