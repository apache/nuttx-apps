system/composite
^^^^^^^^^^^^^^^^^^

  This logic adds a NXH command to control a USB composite device.  The only
  supported composite is CDC/ACM serial with a USB mass storage device.

  Required overall configuration:

  CONFIG_USBDEV=y           - USB device support
  CONFIG_USBDEV_COMPOSITE=y - USB composite device support
  CONFIG_COMPOSITE_IAD=y    - Interface associate descriptor needed

  CONFIG_CDCACM=y           - USB CDC/ACM serial device support
  CONFIG_CDCACM_COMPOSITE=y - USB CDC/ACM serial composite device support
  CONFIG_CDCACM_IFNOBASE=0  - CDC/ACM interfaces start with number 0
  CONFIG_CDCACM_STRBASE=4   - Base of string numbers (not really needed)
  CONFIG_CDCACM_EPINTIN=1   - Endpoint numbers must be unique
  CONFIG_CDCACM_EPBULKIN=2
  CONFIG_CDCACM_EPBULKOUT=3

  CONFIG_USBMSC             - USB mass storage device support
  CONFIG_USBMSC_COMPOSITE=y - USB mass storage composite device support
  CONFIG_USBMSC_IFNOBASE=2  - USB mass storage interfaces start with number 2
  CONFIG_USBMSC_STRBASE=4   - Base of string numbers (needed)
  CONFIG_USBMSC_EPBULKOUT=4 - Endpoint numbers must be unique
  CONFIG_USBMSC_EPBULKIN=5

  CONFIG_NSH_BUILTIN_APPS
    This add-on can be built as two NSH "built-in" commands if this option
    is selected: 'conn' will connect the USB composite device; 'msdis'
    will disconnect the USB composite device.

  Configuration options unique to this add-on:

  CONFIG_SYSTEM_COMPOSITE_DEBUGMM
    Enables some debug tests to check for memory usage and memory leaks.

  CONFIG_SYSTEM_COMPOSITE_NLUNS
    Defines the number of logical units (LUNs) exported by the USB storage
    driver.  Each LUN corresponds to one exported block driver (or partition
    of a block driver).  May be 1, 2, or 3.  Default is 1.
  CONFIG_SYSTEM_COMPOSITE_DEVMINOR1
    The minor device number of the block driver for the first LUN. For
    example, N in /dev/mmcsdN.  Used for registering the block driver. Default
    is zero.
  CONFIG_SYSTEM_COMPOSITE_DEVPATH1
    The full path to the registered block driver.  Default is "/dev/mmcsd0"
  CONFIG_SYSTEM_COMPOSITE_DEVMINOR2 and CONFIG_SYSTEM_COMPOSITE_DEVPATH2
    Similar parameters that would have to be provided if CONFIG_SYSTEM_COMPOSITE_NLUNS
    is 2 or 3.  No defaults.
  CONFIG_SYSTEM_COMPOSITE_DEVMINOR3 and CONFIG_SYSTEM_COMPOSITE_DEVPATH2
    Similar parameters that would have to be provided if CONFIG_SYSTEM_COMPOSITE_NLUNS
    is 3.  No defaults.

  CONFIG_SYSTEM_COMPOSITE_TTYUSB - The minor number of the USB serial device.
    Default is zero (corresponding to /dev/ttyUSB0 or /dev/ttyACM0).  Default is zero.
  CCONFIG_SYSTEM_COMPOSITE_SERDEV - The string corresponding to
    CONFIG_SYSTEM_COMPOSITE_TTYUSB.  The default is "/dev/ttyUSB0" (for the PL2303
    emulation) or "/dev/ttyACM0" (for the CDC/ACM serial device).
  CONFIG_SYSTEM_COMPOSITE_BUFSIZE - The size of the serial I/O buffer in
    bytes.  Default 256 bytes.

  If CONFIG_USBDEV_TRACE is enabled (or CONFIG_DEBUG and CONFIG_DEBUG_USB), then
  the add-on code will also manage the USB trace output.  The amount of trace output
  can be controlled using:

  CONFIG_SYSTEM_COMPOSITE_TRACEINIT
    Show initialization events
  CONFIG_SYSTEM_COMPOSITE_TRACECLASS
    Show class driver events
  CONFIG_SYSTEM_COMPOSITE_TRACETRANSFERS
    Show data transfer events
  CONFIG_SYSTEM_COMPOSITE_TRACECONTROLLER
    Show controller events
  CONFIG_SYSTEM_COMPOSITE_TRACEINTERRUPTS
    Show interrupt-related events.
