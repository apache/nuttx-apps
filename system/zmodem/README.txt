README
======

Contents
========

  o Buffering Notes
    - Hardware Flow Control
    - RX Buffer Size
    - Buffer Recommendations
  o Using NuttX Zmodem with a Linux Host
    - Sending Files from the Target to the Linux Host PC
    - Receiving Files on the Target from the Linux Host PC
  o Building the Zmodem Tools to Run Under Linux
  o Status

Buffering Notes
===============

  Hardware Flow Control
  ---------------------
  Hardware flow control must be enabled in serial drivers in order to
  prevent data overrun.  However, in the most NuttX serial drivers, hardware
  flow control only protects the hardware RX FIFO:  Data will not be lost in
  the hardware FIFO but can still be lost when it is taken from the FIFO.
  We can still overflow the serial driver's RX buffer even with hardware
  flow control enabled! That is probably a bug.  But the workaround solution
  that I have used is to use lower data rates and a large serial driver RX
  buffer.

  Those measures should be unnecessary if buffering and hardware flow
  control are set up and working correctly.

  RX Buffer Size
  --------------
  The Zmodem protocol supports a message that informs the file sender of
  the maximum size of dat that you can buffer (ZRINIT).  However, my
  experience is that the Linux sz ignores this setting and always sends file
  data at the maximum size (1024) no matter what size of buffer you report.
  That is unfortunate because that, combined with the possibilities of data
  overrun mean that you must use quite large buffering for Zmodem file
  receipt to be reliable (none of these issues effect sending of files).

  Buffer Recommendations
  ----------------------
  Based on the limitations of NuttX hardware flow control and of the Linux
  sz behavior, I have been testing with the following configuration
  (assuming UART1 is the Zmodem device):

    1) This setting determines that maximum size of a data packet frame:

       CONFIG_SYSTEM_ZMODEM_PKTBUFSIZE=1024

    2) Input Buffering.  If the input buffering is set to a full frame, then
       data overflow is less likely.

       CONFIG_UART1_RXBUFSIZE=1024

    3) With a larger driver input buffer, the Zmodem receive I/O buffer can be
       smaller:

       CONFIG_SYSTEM_ZMODEM_RCVBUFSIZE=256

    4) Output buffering.  Overrun cannot occur on output (on the NuttX side)
       so there is no need to be so careful:

       CONFIG_SYSTEM_ZMODEM_SNDBUFSIZE=512
       CONFIG_UART1_TXBUFSIZE=256

Using NuttX Zmodem with a Linux Host
====================================

    Sending Files from the Target to the Linux Host PC
    --------------------------------------------------
    The NuttX Zmodem commands have been verified against the rzsz programs
    running on a Linux PC.  To send a file to the PC, first make sure that
    the serial port is configured to work with the board (Assuming you are
    using 9600 baud for the data transfers -- high rates may result in data
    overruns):

      $ sudo stty -F /dev/ttyS0 9600     # Select 9600 BAUD
      $ sudo stty -F /dev/ttyS0 crtscts  # Enables CTS/RTS handshaking *
      $ sudo stty -F /dev/ttyS0          # Show the TTY configuration

      * Only is hardware flow control is enabled.

    Start rz on the Linux host (using /dev/ttyS0 as an example):

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

    Then on the target (using /dev/ttyS1 as an example).

      > sz -d /dev/ttyS1 <filename>

    Where filename is the full path to the file to send (i.e., it begins
    with the '/' character).  /dev/ttyS1 or whatever device you select
    *MUST* support Hardware flow control in order to throttle therates of
    data transfer to fit within the allocated buffers.

    Receiving Files on the Target from the Linux Host PC
    ----------------------------------------------------
    NOTE:  There are issues with using the Linux sz command with the NuttX
    rz command. See "Status" below.  It is recommended that you use the
    NuttX sz command on Linux as described in the next paragraph.

    To send a file to the target, first make sure that the serial port on the
    host is configured to work with the board (Assuming that you are using
    9600 baud for the data transfers -- high rates may result in data
    overruns):

      $ sudo stty -F /dev/ttyS0 9600     # Select 9600 (or other) BAUD
      $ sudo stty -F /dev/ttyS0 crtscts  # Enables CTS/RTS handshaking *
      $ sudo stty -F /dev/ttyS0          # Show the TTY configuration

     * Only is hardware flow control is enabled.

    Start rz on the on the target.  Here, in this example, we are using
    /dev/ttyS1 to perform the transfer

      nsh> rz -d /dev/ttyS1

    /dev/ttyS1 or whatever device you select *MUST* support Hardware flow
    control in order to throttle therates of data transfer to fit within the
    allocated buffers.

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

Building the Zmodem Tools to Run Under Linux
============================================

  Build support has been added so that the NuttX Zmodem implementation can be executed on a Linux host PC.  This can be done by

    - Change to the apps/systems/zmodem directory
    - Make using the special makefile, Makefile.host

  NOTES:

  1. TOPDIR and APPDIR must be defined on the make command line:  TOPDIR is
     the full path to the nuttx/ directory;  APPDIR is the full path to the
     apps/ directory.  For example, if you installed nuttx at
     /home/me/projects/nuttx and apps at /home/me/projects/apps, then the
     correct make command line would be:

       make -f Makefile.host TOPDIR=/home/me/projects/nuttx APPDIR=/home/me/projects/apps

  2. Add CONFIG_DEBUG_FEATURES=1 to the make command line to enable debug output
  3. Make sure to clean old target .o files before making new host .o files.

  This build is has been verified as of 2013-7-16 using Linux to transfer
  files with an Olimex LPC1766STK board.  It works great and seems to solve
  all of the problems found with the Linux sz/rz implementation.

Status
======
    2013-7-15: Testing against the Linux rz/sz commands.

      I have tested with the configs/olimex-lpc1766stk configuration.  I
      have been able to send large and small files with the target sz
      command. I have been able to receive small files, but there are
      problems receiving large files using the Linux sz command:  The
      Linux SZ does not obey the buffering limits and continues to send
      data while rz is writing the previously received data to the file
      and the serial driver's RX buffer is overrun by a few bytes while
      the write is in progress. As a result, when it reads the next
      buffer of data, a few bytes may be missing.  The symptom of this
      missing data is a CRC check failure.

      Either (1) we need a more courteous host application, or (2) we
      need to greatly improve the target side buffering capability!
      Either (1) we need a more courteous host application, or (2) we
      need to greatly improve the target side buffering capability!

      My thought now is to implement the NuttX sz and rz commands as
      PC side applications as well.  Matching both sides and obeying
      the handshaking will solve the issues.  Another option might be
      to fix the serial driver hardware flow control somehow.

    2013-7-16. More Testing against the Linux rz/sz commands.

      I have verified that with debug off and at lower serial BAUD
      (2400), the transfers of large files succeed without errors.  I do
      not consider this a "solution" to the problem.  I also found that
      the LPC17xx hardware flow control caused strange hangs; Zmodem
      works better with hardware flow control disabled on the LPC17xx.

      At this lower BAUD, RX buffer sizes could probably be reduced; Or
      perhaps the BAUD coud be increased.  My thought, however, is that
      tuning in such an unhealthy situation is not the approach:  The
      best thing to do would be to use the matching NuttX sz on the Linux
      host side.

    2013-7-16. More Testing against the NuttX rz/sz on Both Ends.

      The NuttX sz/rz commands have been modified so that they can be
      built and executed under Linux.  In this case, there are no
      transfer problems at all in either direction and with large or
      small files.  This configuration could probably run at much higher
      serial speeds and with much smaller buffers (although that has not
      been verified as of this writing).

