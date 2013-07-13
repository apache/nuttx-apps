README
======

Using NuttX Zmodem with a Linux Host
====================================

    The NuttX Zmodem commands have been verified against the rzsz programs
    running on a Linux PC.  To send a file to the PC, first make sure that
    the serial port is configured to work with the board:

      $ sudo stty -F /dev/ttyS0 57600
      $ sudo stty -F /dev/ttyS0

    start rz on the Linux host:

      $ sudo rz </dev/ttyS0 >/dev/ttyS0

    You can add the rz -v option multiple times, each increases the level
    of debug output.  If you want to capture the Linux rz output, then
    re-direct stderr to a log file by adding 2>rz.log to the end of the
    rz command.

    NOTE: The NuttX Zmodem does sends rz\n when it starts in compliance with
    the Zmodem specification.  On Linux this, however, seems to start some
    other, incompatible version of rz.  You need to start rz manually to
    make sure that the correct version is selected.  You can tell when this
    evil rz/sz has inserted itself because you will see the '^' (0x5e)
    character replacing the standard Zmodem ZDLE character (0x19) in the
    binary data stream.

    If you don't have the rz command on your Linux box, the package to
    install rzsz (or possibily lrzsz).

    Then on the target:

      > sz -d /dev/ttyS1 <filename>

    Where filename is the full path to the file to send (i.e., it begins
    with the '/' character).

 