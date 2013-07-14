README
======

Using NuttX Zmodem with a Linux Host
====================================

    Sending Files from the Target to the Linux Host PC
    --------------------------------------------------
    The NuttX Zmodem commands have been verified against the rzsz programs
    running on a Linux PC.  To send a file to the PC, first make sure that
    the serial port is configured to work with the board:

      $ sudo stty -F /dev/ttyS0 57600
      $ sudo stty -F /dev/ttyS0

    Start rz on the Linux host:

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

    Receiving Files on the Target from the Linux Host PC
    ----------------------------------------------------
    To send a file to the target, first make sure that the serial port on the
    host is configured to work with the board:

      $ sudo stty -F /dev/ttyS0 57600
      $ sudo stty -F /dev/ttyS0

    Start rz on the on the target:

      nsh> rz -d /dev/ttyS1

    Then use the sz command on Linux to send the file to the target:

      $ sudo sz <filename> t </dev/ttyS0 >/dev/ttyS0

    Where <filename> is the file that you want to send.

    The resulting file will be found where you have configured the Zmodem
    "sandbox" via CONFIG_SYSTEM_ZMODEM_MOUNTPOINT.

    You can add the az -v option multiple times, each increases the level
    of debug output.  If you want to capture the Linux rz output, then
    re-direct stderr to a log file by adding 2>az.log to the end of the
    rz command.

    If you don't have the az command on your Linux box, the package to
    install rzsz (or possibily lrzsz).
