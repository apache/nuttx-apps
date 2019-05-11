README File for the SPI Tool
============================

The I2C tool provides a way to debug SPI related problems.  This README file
will provide usage information for the SPI tools.

CONTENTS
========

  o System Requirements
    - SPI Driver
    - Configuration Options
  o Help
  o Common Line Form
  o Common Command Options
    - "Sticky" Options
    - Environment variables
    - Common Option Summary
  o Command summary
    - bus
    - dev
    - get
    - set
    - verf
  o I2C Build Configuration
    - NuttX Configuration Requirements
    - I2C Tool Configuration Options

System Requirements
===================

The SPI tool is designed to be implemented as a NuttShell (NSH) add-on.  Read
the apps/nshlib/README.txt file for information about add-ons.

Configuration Options
---------------------
CONFIG_NSH_BUILTIN_APPS - Build the tools as an NSH built-in command
CONFIG_SPITOOL_MINBUS   - Smallest bus index supported by the hardware (default 0).
CONFIG_SPITOOL_MAXBUS   - Largest bus index supported by the hardware (default 3)
CONFIG_SPITOOL_DEFFREQ  - Default frequency (default: 40000000)
CONFIG_SPITOOL_DEFMODE  - Default mode, where;
                           0 = CPOL=0, CHPHA=0
                           1 = CPOL=0, CHPHA=1
                           2 = CPOL=1, CHPHA=0
                           3 = CPOL=1, CHPHA=1
CONFIG_SPITOOL_DEFWIDTH - Default bit width (default 8)
CONFIG_SPITOOL_DEFWORDS - Default number of words to exchange (default 1)

HELP
====

he SPI tools supports some help output.  That help output can be view
by entering either:

  nsh> spi help

or

  nsh> spi ?

Here is an example of the help output.  I shows the general form of the
command line, the various SPI commands supported with their unique command
line options, and a more detailed summary of the command SPI command
options.

nsh> Usage: spi <cmd> [arguments]
Where <cmd> is one of:

  Show help     : ?
  List busses   : bus
  SPI Exchange  : exch [OPTIONS] [<hex senddata>]
  Show help     : help

Where common "sticky" OPTIONS include:
  [-b bus] is the SPI bus number (decimal).  Default: 0 Current: 2
      [-f freq] SPI frequency.  Default: 4000000 Current: 4000000
  [-m mode] Mode for transfer.  Default: 0 Current: 0
  [-u udelay] Delay after transfer in uS.  Default: 0 Current: 0
  [-w width] Width of bus.  Default: 8 Current: 8
  [-x count] Words to exchange  Default: 1 Current: 4

NOTES:
o An environment variable like $PATH may be used for any argument.
o Arguments are "sticky".  For example, once the SPI address is
  specified, that address will be re-used until it is changed.

WARNING:
o The SPI commands may have bad side effects on your SPI devices.
  Use only at your own risk.

COMMAND LINE FORM
=================

The SPI is started from NSH by invoking the 'spi' command from the NSH
command line. The general form of the 'spi' command is:

  spi <cmd> [arguments]

Where <cmd> is a "sub-command" and identifies one SPI operation supported
by the tool.  [arguments] represents the list of arguments needed to perform
the SPI operation.  Those arguments vary from command to command as
described below.  However, there is also a core set of common OPTIONS
supported by all commands.  So perhaps a better representation of the
general SPI command would be:

  i2c <cmd> [OPTIONS] [arguments]

Where [OPTIONS] represents the common options and and arguments represent
the operation-specific arguments.

COMMON COMMAND OPTIONS
======================

"Sticky" Options
----------------
In order to interact with SPI devices, there are a number of SPI parameters
that must be set correctly.  One way to do this would be to provide to set
the value of each separate command for each SPI parameter.  The SPI tool
takes a different approach, instead:  The SPI configuration can be specified
as a (potentially long) sequence of command line arguments.

These arguments, however, are "sticky."  They are sticky in the sense that
once you set the SPI parameter, that value will remain until it is reset
with a new value (or until you reset the board).

Environment Variables
---------------------
NOTE also that if environment variables are not disabled (by
CONFIG_DISABLE_ENVIRON=y), then these options may also be environment
variables.  Environment variables must be preceded with the special
character $.  For example, PWD is the variable that holds the current
working directory and so $PWD could be used as a command line argument.  The
use of environment variables on the I2C tools command is really only useful
if you wish to write NSH scripts to execute a longer, more complex series of
SPI commands.

Common Option Summary
---------------------

[-b bus] is the SPI bus number (decimal).  Default: 0
  Which SPI bus to commiuncate on. The bus must have been initialised
  as a character device in the config in the form /dev/spiX (e.g. /dev/spi2).

  The valid range of bus numbers is controlled by the configuration settings
  CONFIG_SPITOOL_MINBUS and CONFIG_SPITOOL_MAXBUS.

  The bus numbers are small, decimal numbers.

[-m mode] SPI Mode for transfer.
  Which of the available SPI modes is to be used. Options are;
                           0 = CPOL=0, CHPHA=0
                           1 = CPOL=0, CHPHA=1
                           2 = CPOL=1, CHPHA=0
                           3 = CPOL=1, CHPHA=1

 [-u udelay] Delay after transfer in uS.  Default: 0
  Any extra delay to be provided after the transfer. Not normally needed
  from the command line.

[-x count] Words to exchange  Default: 1
  The number of words to be transited over the bus. For sanitys sake this is
  limited to a relatively small number (40 by default). Any data on the
  command line is sent first, padded by 0xFF's while any remaining data
  are received.

[-w width] is the data width (varies according to target).  Default: 8

  Various SPI devices support different data widths. This option is untested.

[-f freq] I2C frequency.  Default: 4000000 Current: 4000000

  The [-f freq] sets the frequency of the SPI device. The default is very conservative.

COMMAND SUMMARY
===============

List buses: bus [OPTIONS]
--------------------------

This command will simply list all of the configured SPI buses and indicate
which are supported by the driver and which are not:

  BUS   EXISTS?
  Bus 1: YES
  Bus 2: NO

The valid range of bus numbers is controlled by the configuration settings
CONFIG_SPITOOL_MINBUS and CONFIG_SPITOOL_MAXBUS.

Exchange data: exch [OPTIONS] <Optional TX Data>
------------------------------------------------

This command triggers an SPI transfer, returning the data back from the far end.
As an example (with MOSI looped back to MISO);

nsh>spi exch -b 2 -x 4 aabbccdd
Received: AA BB CC DD
nsh> 

Note that the TXData are always specified in hex, and are always two digits each,
case insensitive.

I2C BUILD CONFIGURATION
=======================

NuttX Configuration Requirements
--------------------------------
The SPI tools requires the following in your NuttX configuration:

1. Application configuration.

   Using 'make menuconfig', select the SPI tool.  The following
   definition should appear in your .config file:

     CONFIG_SYSTEM_SPI=y

2. Device-specific SPI driver support must be enabled:

     CONFIG_SPI_DRIVER=y

   The SPI tool will then use the SPI character driver to access the SPI
   bus.  These devices will reside at /dev/spiN where N is the I2C bus
   number.

   NOTE 1: The SPI driver ioctl interface is defined in
   include/nuttx/spi/spi.h.
