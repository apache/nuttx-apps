# Examples

### Selecting Examples

The examples directory contains several sample applications that can be linked
with NuttX. The specific example is selected in the
`boards/<arch-name>/<chip-name>/<board-name>/configs/<config>/defconfig` file
via the `CONFIG_EXAMPLES_xyz` setting where `xyz` is the name of the example.
For example:

```conf
CONFIG_EXAMPLES_HELLO=y
```

Selects the `examples/hello` _Hello, World!_ example.

### Built-In Functions

Some of the examples may be built as _built-in_ functions that can be executed
at run time (rather than as NuttX _main_ programs). These _built-in_ examples
can be also be executed from the NuttShell (NSH) command line. In order to
configure these built-in NSH functions, you have to set up the following:

- `CONFIG_NSH_BUILTIN_APPS` – Enable support for external registered, _named_
  applications that can be executed from the NSH command line (see
  `apps/README.md` for more information).

## `adc` Read from ADC

A mindlessly simple test of an ADC devices. It simply reads from the ADC device
and dumps the data to the console forever.

This test depends on these specific ADC/NSH configurations settings (your
specific ADC settings might require additional settings).

- `CONFIG_ADC` – Enabled ADC support.
- `CONFIG_NSH_BUILTIN_APPS` – Build the ADC test as an NSH built-in function.
  Default: Built as a standalone program.

Specific configuration options for this example include:

- `CONFIG_EXAMPLES_ADC_DEVPATH` – The default path to the ADC device. Default:
  `/dev/adc0`.
- `CONFIG_EXAMPLES_ADC_NSAMPLES` – This number of samples is collected and the
  program terminates. Default: Samples are collected indefinitely.
- `CONFIG_EXAMPLES_ADC_GROUPSIZE` – The number of samples to read at once.
  Default: `4`.

## `ajoystick` Analog Joystick

This is a simple test of the analog joystick driver. See details about this
driver in `nuttx/include/nuttx/input/ajoystick.h`.

Configuration Pre-requisites:

- `CONFIG_AJOYSTICK` – The analog joystick driver.

Example Configuration:
- `CONFIG_EXAMPLES_AJOYSTICK` – Enabled the analog joystick example.
- `CONFIG_EXAMPLES_AJOYSTICK_DEVNAME` – Joystick device name. Default
  `/dev/adjoy0`.
- `CONFIG_EXAMPLES_AJOYSTICK_SIGNO` – Signal used to signal the test
  application. Default `13`.

## `alarm` RTC Alarm

A simple example that tests the alarm IOCTLs of the RTC driver.

Dependencies:

- `CONFIG_RTC_DRIVER` –  RTC driver must be initialized to allow user space
  access to the RTC.
- `CONFIG_RTC_ALARM` – Support for RTC alarms must be enabled.

Configuration:

- `CONFIG_EXAMPLES_ALARM` – Enable the RTC driver alarm test.
- `CONFIG_EXAMPLES_ALARM_PROGNAME` –  This is the name of the program that will
  be used when the NSH ELF program is installed.
- `CONFIG_EXAMPLES_ALARM_PRIORITY` – Alarm daemon priority.
- `CONFIG_EXAMPLES_ALARM_STACKSIZE` – Alarm daemon stack size.
- `CONFIG_EXAMPLES_ALARM_DEVPATH` – RTC device path (`/dev/rtc0`).
- `CONFIG_EXAMPLES_ALARM_SIGNO` – Alarm signal.

## `apa102` Rainbow on `APA102` LED Strip

Rainbow example for `APA102` LED Strip.

## `bastest` Bas BASIC Interpreter

This directory contains a small program that will mount a ROMFS file system
containing the BASIC test files extracted from the Bas `2.4` release. See
`examples/bastest/README.md` for licensing and usage information.

- `CONFIG_EXAMPLES_BASTEST_DEVMINOR` – The minor device number of the ROMFS
  block driver. For example, the `N` in `/dev/ramN`. Used for registering the
  RAM block driver that will hold the ROMFS file system containing the BASIC
  files to be tested. Default: `0`.

- `CONFIG_EXAMPLES_BASTEST_DEVPATH` – The path to the ROMFS block driver device.
  This must match `EXAMPLES_BASTEST_DEVMINOR`. Used for registering the RAM
  block driver that will hold the ROMFS file system containing the BASIC files
  to be tested. Default: `/dev/ram0`.

## `bridge` Network Bridge

A simple test of a system with multiple networks. It simply echoes all UDP
packets received on network `1` and network `2` to network `2` and network `1`,
respectively. Interface `1` and interface may or may not lie on the same
network.

- `CONFIG_EXAMPLES_BRIDGE` – Enables the simple UDP bridge test.

There identical configurations for each of the two networks, `NETn` where `n`
refers to the network being configured `n={1,2}`. Let `m` refer to the other
network.

- `CONFIG_EXAMPLES_BRIDGE_NETn_IFNAME` – The register name of the network `n`
  device. Must match the previously registered driver name and must not be the
  same as other network device name, `CONFIG_EXAMPLES_BRIDGE_NETm_IFNAME`.
- `CONFIG_EXAMPLES_BRIDGE_NETn_RECVPORT` – Network `n` listen port number.
- `CONFIG_EXAMPLES_BRIDGE_NETn_SNDPORT` – Network `2` send port number.
- `CONFIG_EXAMPLES_BRIDGE_NETn_IOBUFIZE` – Size of the network `n` UDP
  send/receive I/O buffer.
- `CONFIG_EXAMPLES_BRIDGE_NETn_STACKSIZE` – Network `n` daemon stacksize.
- `CONFIG_EXAMPLES_BRIDGE_NETn_PRIORITY` – Network `n` daemon task priority.

If used as a NSH add-on, then it is assumed that initialization of both networks
was performed externally prior to the time that this test was started.
Otherwise, the following options are available:

- `CONFIG_EXAMPLES_BRIDGE_NETn_NOMAC` – Select of the network `n` hardware does
  not have a built-in MAC address. If selected, the MAC address. provided by
  `CONFIG_EXAMPLES_BRIDGE_NETn_MACADDR` will be used to assign the MAC address
  to the network n device.
- `CONFIG_EXAMPLES_BRIDGE_NETn_DHCPC` – Use DHCP Client to get the network n IP
  address.
- `CONFIG_EXAMPLES_BRIDGE_NETn_IPADDR` – If `CONFIG_EXAMPLES_BRIDGE_NETn_DHCPC`
  is not selected, then this is the fixed IP address for network `n`.
- `CONFIG_EXAMPLES_BRIDGE_NETn_DRIPADDR` – Network `n` default router IP address
  (Gateway).
- `CONFIG_EXAMPLES_BRIDGE_NETn_NETMASK` – Network `n` mask.

## `buttons` Read GPIO Buttons

To be provided.

## `can` CAN Device Test

If the CAN device is configured in loopback mode, then this example can be used
to test the CAN device in loop back mode. It simple sinces a sequence of CAN
messages and verifies that those messages are returned exactly as sent.

This test depends on these specific CAN/NSH configurations settings (your
specific CAN settings might require additional settings).

- `CONFIG_CAN` – Enables CAN support.
- `CONFIG_CAN_LOOPBACK` – A CAN driver may or may not support a loopback mode
  for testing. The STM32 CAN driver does support loopback mode.
- `CONFIG_NSH_BUILTIN_APPS` – Build the CAN test as an NSH built-in function.
  Default: Built as a standalone program.

Specific configuration options for this example include:

- `CONFIG_EXAMPLES_CAN_DEVPATH` – The path to the CAN device. Default:
  `/dev/can0`.
- `CONFIG_EXAMPLES_CAN_NMSGS` – This number of CAN message is collected and the
  program terminates. Default: messages are sent and received indefinitely.

The default behavior assumes loopback mode. Messages are sent, then read and
verified. The behavior can be altered for other kinds of testing where the test
only sends or received (but does not verify) can messages.

- `CONFIG_EXAMPLES_CAN_READONLY` – Only receive messages.
- `CONFIG_EXAMPLES_CAN_WRITEONLY` – Only send messages.

## `canard`

Example application for `canutils/libcarnard`.

## `cctype`

Verifies all possible inputs for all functions defined in the header file
`cctype`.

## `chat` AT over TTY

Demonstrates AT chat functionality over a TTY device. This is useful with AT
modems, for example, to establish a `pppd` connection (see the related `pppd`
example). Moreover, some AT modems – such as ones made by u-blox – have an
internal TCP/IP stack, often with an implementation of TLS/SSL. In such cases
the chat utility can be used to configure the internal TCP/IP stack, establish
socket connections, set up security (e.g., download base64-encoded certificates
to the modem), and perform data exchange through sockets over the TTY device.

Useful configuration parameters:

- `CONFIG_EXAMPLES_CHAT_PRESET[0..3]` – preset chat scripts.
- `CONFIG_EXAMPLES_CHAT_TTY_DEVNODE` – TTY device node name.
- `CONFIG_EXAMPLES_CHAT_TIMEOUT_SECONDS` – default receive timeout.

## `configdata`

This is a Unit Test for the MTD configuration data driver.

## `cpuhog` Keep CPU Busy

Attempts to keep the system busy by passing data through a pipe in loop back
mode. This may be useful if you are trying run down other problems that you
think might only occur when the system is very busy.

## `dac` Write to DAC

This is a tool for writing values to DAC device.

## `dhcpd` DHCP Server

This examples builds a tiny DHCP server for the target system.

**Note**: For test purposes, this example can be built as a host-based DHCPD
server. This can be built as follows:

```bash
cd examples/dhcpd
make -f Makefile.host TOPDIR=<nuttx-directory>
```

NuttX configuration settings:

- `CONFIG_NET=y` – of course.
- `CONFIG_NET_UDP=y` – UDP support is required for DHCP (as well as various
  other UDP-related configuration settings).
- `CONFIG_NET_BROADCAST=y` – UDP broadcast support is needed.
- `CONFIG_NETUTILS_NETLIB=y` – The networking library is needed.
- `CONFIG_EXAMPLES_DHCPD_NOMAC` – (May be defined to use software assigned MAC)

See also `CONFIG_NETUTILS_DHCPD_*` settings described elsewhere and used in
`netutils/dhcpd/dhcpd.c`. These settings are required to described the behavior
of the daemon.

## `discover` UDP Discover Daemon

This example exercises `netutils/discover` utility. This example initializes and
starts the UDP discover daemon. This daemon is useful for discovering devices in
local networks, especially with DHCP configured devices. It listens for UDP
broadcasts which also can include a device class so that groups of devices can
be discovered. It is also possible to address all classes with a kind of
broadcast discover.

This example will automatically be built as an NSH built-in if
`CONFIG_NSH_BUILTIN_APPS` is selected. Otherwise, it will be a standalone
program with entry point `discover_main`.

NuttX configuration settings:

- `CONFIG_EXAMPLES_DISCOVER_DHCPC` – DHCP Client.
- `CONFIG_EXAMPLES_DISCOVER_NOMAC` – Use canned MAC address.
- `CONFIG_EXAMPLES_DISCOVER_IPADDR` – Target IP address.
- `CONFIG_EXAMPLES_DISCOVER_DRIPADDR` – Router IP address.
- `CONFIG_EXAMPLES_DISCOVER_NETMASK` – Network Mask.

## `djoystick` Discrete Joystick

This is a simple test of the discrete joystick driver. See details about this
driver in `nuttx/include/nuttx/input/djoystick.h`.

Configuration Pre-requisites:

- `CONFIG_DJOYSTICK` – The discrete joystick driver.

Example Configuration:

- `CONFIG_EXAMPLES_DJOYSTICK` – Enabled the discrete joystick example.
- `CONFIG_EXAMPLES_DJOYSTICK_DEVNAME` – Joystick device name. Default
  `/dev/djoy0`.
- `CONFIG_EXAMPLES_DJOYSTICK_SIGNO` – Signal used to signal the test
  application. Default `13`.


## `dsptest` DSP

This is a Unit Test for the NuttX DSP library. It use Unity testing framework.

Dependencies:

```conf
CONFIG_LIBDSP=y
CONFIG_LIBDSP_DEBUG=y
CONFIG_TESTING_UNITY=y
```

Optional configuration:

- `CONFIG_TESTING_UNITY_OUTPUT_COLOR` – enable colored output.

## `elf` ELF loader

This example builds a small ELF loader test case. This includes several test
programs under `examples/elf` tests. These tests are build using the relocatable
ELF format and installed in a ROMFS file system. At run time, each program in
the ROMFS file system is executed. Requires `CONFIG_ELF`. Other configuration
options:

- `CONFIG_EXAMPLES_ELF_DEVMINOR` – The minor device number of the ROMFS block
  driver. For example, the `N` in `/dev/ramN`. Used for registering the RAM
  block driver that will hold the ROMFS file system containing the ELF
  executables to be tested. Default: `0`.

- `CONFIG_EXAMPLES_ELF_DEVPATH` – The path to the ROMFS block driver device.
  This must match `EXAMPLES_ELF_DEVMINOR`. Used for registering the RAM block
  driver that will hold the ROMFS file system containing the ELF executables to
  be tested. Default: `/dev/ram0`.

**Notes**:

1. `CFLAGS` should be provided in `CELFFLAGS`. RAM and FLASH memory regions may
   require long allcs. For ARM, this might be:

   ```makefile
   CELFFLAGS = $(CFLAGS) -mlong-calls
   ```

   Similarly for C++ flags which must be provided in `CXXELFFLAGS`.

2. Your top-level `nuttx/Make.defs` file must also include an appropriate
   definition, `LDELFFLAGS`, to generate a relocatable ELF object. With GNU LD,
   this should include `-r` and `-e main` (or `_main` on some platforms).

   ```makefile
   LDELFFLAGS = -r -e main
   ```

   If you use GCC to link, you make also need to include `-nostdlib` or
   `-nostartfiles` and `-nodefaultlibs`.

3. This example also requires `genromfs`. `genromfs` can be build as part of the
   nuttx toolchain. Or can built from the `genromfs` sources that can be found
   in the NuttX tools repository (`genromfs-0.5.2.tar.gz`). In any event, the
   `PATH` variable must include the path to the genromfs executable.

4. ELF size: The ELF files in this example are, be default, quite large because
   they include a lot of _build garbage_. You can greatly reduce the size of the
   ELF binaries are using the `objcopy --strip-unneeded` command to remove
   un-necessary information from the ELF files.

5. Simulator. You cannot use this example with the NuttX simulator on Cygwin.
   That is because the Cygwin GCC does not generate ELF file but rather some
   Windows-native binary format.

   If you really want to do this, you can create a NuttX x86 buildroot toolchain
   and use that be build the ELF executables for the ROMFS file system.

6. Linker scripts. You might also want to use a linker scripts to combine
   sections better. An example linker script is at
   `nuttx/binfmt/libelf/gnu-elf.ld`. That example might have to be tuned for
   your particular linker output to position additional sections correctly. The
   GNU LD `LDELFFLAGS` then might be:

   ```makefile
   LDELFFLAGS = -r -e main -T$(TOPDIR)/binfmt/libelf/gnu-elf.ld
   ```

## `fb` Framebuffer

A simple test of the framebuffer character driver.

## `flash_test` SMART Flash

This example performs a SMART flash block device test. This test performs a
sector allocate, read, write, free and garbage collection test on a SMART MTD
block device.

- `CONFIG_EXAMPLES_FLASH_TEST=y` – Enables the FLASH Test.

Dependencies:

- `CONFIG_MTD_SMART=y` – SMART block driver support.
- `CONFIG_BUILD_PROTECTED=n` and `CONFIG_BUILD_KERNEL=n` – This test uses
  internal OS interfaces and so is not available in the NUTTX kernel builds.

## `flowc` Serial Hardware Flow Control

A simple test of serial hardware flow control.

## `ft80x` FT80x GUI Chip

This examples has ports of several FTDI demos for the FTDI/BridgeTek FT80x GUI
chip. As an example configuration, see
`nuttx/boards/arm/stm32/viewtool-stm32f107/configs/ft80x/defconfig`.

## `ftpc` FTP Client

This is a simple FTP client shell used to exercise the capabilities of the FTPC
library (`apps/netutils/ftpc`).

From NSH, the startup command sequence is as follows. This is only an example,
your configuration could have different mass storage devices, mount paths, and
FTP directories:

```
nsh> mount -t vfat /dev/mmcsd0 /tmp  # Mount the SD card at /tmp
nsh> cd /tmp                         # cd into the /tmp directory
nsh> ftpc xx.xx.xx.xx[:pp]           # Start the FTP client
nfc> login <name> <password>         # Log into the FTP server
nfc> help                            # See a list of FTP commands
```

where `xx.xx.xx.xx` is the IP address of the FTP server and `pp` is an optional
port number.

**Note**: By default, FTPC uses `readline` to get data from `stdin`. So your
defconfig file must have the following build path:

```conf
CONFIG_SYSTEM_READLINE=y
```

**Note**: If you use the ftpc task over a telnet NSH connection, then you should
set the following configuration item:

```conf
CONFIG_EXAMPLES_FTPC_FGETS=y
```

By default, the FTPC client will use `readline()` to get characters from the
console. Readline includes and command-line editor and echos characters received
in stdin back through `stdout`. Neither of these behaviors are desire-able if
Telnet is used.

You may also want to define the following in your configuration file. Otherwise,
you will have not feedback about what is going on:

```conf
CONFIG_DEBUG_FEATURES=y
CONFIG_DEBUG_INFO=y
CONFIG_DEBUG_FTPC=y
```

## `ftpd` FTP daemon

This example exercises the FTPD daemon at `apps/netuils/ftpd`. Below are
configurations specific to the FTPD example (the FTPD daemon itself may require
other configuration options as well).

- `CONFIG_EXAMPLES_FTPD` – Enable the FTPD example.
- `CONFIG_EXAMPLES_FTPD_PRIO` – Priority of the FTP daemon. Default:
  `SCHED_PRIORITY_DEFAULT`.
- `CONFIG_EXAMPLES_FTPD_STACKSIZE` – Stack size allocated for the FTP daemon.
  Default: `2048`.
- `CONFIG_EXAMPLES_FTPD_NONETINIT` – Define to suppress configuration of the
  network by `apps/examples/ftpd`. You would need to suppress network
  configuration if the network is configuration prior to running the example.

NSH always initializes the network so if `CONFIG_NSH_NETINIT` is defined, so is
`CONFIG_EXAMPLES_FTPD_NONETINIT` (se it does not explicitly need to be defined
in that case):

- `CONFIG_NSH_BUILTIN_APPS` – Build the FTPD daemon example test as an NSH
  built-in function. By default the FTPD daemon will be built as a standalone
  application.

If `CONFIG_EXAMPLES_FTPD_NONETINIT` is not defined, then the following may be
specified to customized the network configuration:

- `CONFIG_EXAMPLES_FTPD_NOMAC` – If the hardware has no MAC address of its own,
  define this `=y` to provide a bogus address for testing.
- `CONFIG_EXAMPLES_FTPD_IPADDR` – The target IP address. Default `10.0.0.2`.
- `CONFIG_EXAMPLES_FTPD_DRIPADDR` – The default router address. Default:
  `10.0.0.1`.
- `CONFIG_EXAMPLES_FTPD_NETMASK` – The network mask. Default: `255.255.255.0`.

Other required configuration settings: Of course TCP networking support is
required. But here are a couple that are less obvious:

- `CONFIG_DISABLE_PTHREAD` – `pthread` support is required.

Other FTPD configuration options they may be of interest:

- `CONFIG_FTPD_VENDORID` – The vendor name to use in FTP communications.
  Default: `NuttX`.
- `CONFIG_FTPD_SERVERID` – The server name to use in FTP communications.
  Default: `NuttX FTP Server`.
- `CONFIG_FTPD_CMDBUFFERSIZE` – The maximum size of one command. Default: `512`
  bytes.
- `CONFIG_FTPD_DATABUFFERSIZE` – The size of the I/O buffer for data transfers.
  Default: `2048` bytes.
- `CONFIG_FTPD_WORKERSTACKSIZE` – The stacksize to allocate for each FTP daemon
  worker thread. Default: `2048` bytes.

The following netutils libraries should be enabled in your `defconfig` file:

```conf
CONFIG_NETUTILS_NETLIB=y
CONFIG_NETUTILS_TELNED=y
```

## `gpio` GPIO Read and Write

A simple `test/example` of the NuttX GPIO driver.

## `hello` Hello World

This is the mandatory, _Hello, World!!_ example. It is little more than
`examples/null` with a single `printf` statement. Really useful only for
bringing up new NuttX architectures.

- `CONFIG_NSH_BUILTIN_APPS` – Build the _Hello, World_ example as an NSH
  built-in application.

## `helloxx` Hello World in C++

This is C++ version of the _Hello, World!!_ example. It is intended only to
verify that the C++ compiler is functional, that basic C++ library support is
available, and that class are instantiated correctly.

NuttX configuration prerequisites:

- `CONFIG_HAVE_CXX` – Enable C++ Support.

Optional NuttX configuration settings:

- `CONFIG_HAVE_CXXINITIALIZE` – Enable support for static constructors (may not
  be available on all platforms).

NuttX configuration settings specific to this example:

- `CONFIG_NSH_BUILTIN_APPS` – Build the helloxx example as a _built-in_ that can
  be executed from the NSH command line.

Also needed:

- `CONFIG_HAVE_CXX=y`

And you may have to tinker with the following to get libxx to compile properly:

- `CCONFIG_ARCH_SIZET_LONG=y` or `=n`.

The argument of the `new` operators should take a type of `size_t`. But `size_t`
has an unknown underlying. In the nuttx `sys/types.h` header file, `size_t` is
typed as `uint32_t` (which is determined by architecture-specific logic). But
the C++ compiler may believe that `size_t` is of a different type resulting in
compilation errors in the operator. Using the underlying integer type Instead of
`size_t` seems to resolve the compilation issues.

## `hidkbd` USB Host HID keyboard

This is a simple test to `debug/verify` the USB host HID keyboard class driver.

- `CONFIG_EXAMPLES_HIDKBD_DEFPRIO` – Priority of _waiter_ thread. Default: `50`.
- `CONFIG_EXAMPLES_HIDKBD_STACKSIZE` – Stacksize of _waiter_ thread. Default
  `1024`.
- `CONFIG_EXAMPLES_HIDKBD_DEVNAME` – Name of keyboard device to be used.
  Default: `/dev/kbda`.
- `CONFIG_EXAMPLES_HIDKBD_ENCODED` –  Decode special key press events in the
  user buffer. In this case, the example coded will use the interfaces defined
  in `include/nuttx/input/kbd_codec.h` to decode the returned keyboard data.
  These special keys include such things as up/down arrows, home and end keys,
  etc. If this not defined, only 7-bit printable and control ASCII characters
  will be provided to the user. Requires `CONFIG_HIDKBD_ENCODED` and
  `CONFIG_LIB_KBDCODEC`.

## `igmp` Trivial IGMP

This is a trivial test of the NuttX IGMP capability. It present it does not do
much of value – Much more is needed in order to verify the IGMP features!

- `CONFIG_EXAMPLES_IGMP_NOMAC` – Set if the hardware has no MAC address; one
  will be assigned.
- `CONFIG_EXAMPLES_IGMP_IPADDR` – Target board IP address.
- `CONFIG_EXAMPLES_IGMP_DRIPADDR` – Default router address.
- `CONFIG_EXAMPLES_IGMP_NETMASK` – Network mask.
- `CONFIG_EXAMPLES_IGMP_GRPADDR` – Multicast group address.
- `CONFIG_EXAMPLES_NETLIB` – The networking library is needed.

## `i2cchar` Transfer Through I2C

A mindlessly simple test of an I2C driver. It reads an write garbage data to the
I2C transmitter and/or received as fast possible.

This test depends on these specific I2S/AUDIO/NSH configurations settings (your
specific I2S settings might require additional settings).

- `CONFIG_I2S` – Enabled I2S support.
- `CONFIG_AUDIO` – Enabled audio support.
- `CONFIG_DRIVERS_AUDIO` – Enable audio device support.
- `CONFIG_AUDIO_I2SCHAR` – Enabled support for the I2S character device.
- `CONFIG_NSH_BUILTIN_APPS` – Build the I2S test as an NSH built-in function.
  Default: Built as a standalone program.

Specific configuration options for this example include:

- `CONFIG_EXAMPLES_I2SCHAR` – Enables the I2C test.

- `CONFIG_EXAMPLES_I2SCHAR_DEVPATH` – The default path to the ADC device.
  Default: `/dev/i2schar0`.

- `CONFIG_EXAMPLES_I2SCHAR_TX` – This should be set if the I2S device supports a
  transmitter.

- `CONFIG_EXAMPLES_I2SCHAR_TXBUFFERS` – This is the default number of audio
  buffers to send before the TX transfers terminate. When both TX and RX
  transfers terminate, the task exits (and, if an NSH builtin, the `i2schar`
  command returns). This number can be changed from the NSH command line.

- `CONFIG_EXAMPLES_I2SCHAR_TXSTACKSIZE` – This is the stack size to use when
  starting the transmitter thread. Default `1536`.

- `CONFIG_EXAMPLES_I2SCHAR_RX` – This should be set if the I2S device supports a
  transmitter.

- `CONFIG_EXAMPLES_I2SCHAR_RXBUFFERS` – This is the default number of audio
  buffers to receive before the RX transfers terminate. When both TX and RX
  transfers terminate, the task exits (and, if an NSH builtin, the `i2schar`
  command returns). This number can be changed from the NSH command line.

- `CONFIG_EXAMPLES_I2SCHAR_RXSTACKSIZE` – This is the stack size to use when
  starting the receiver thread. Default `1536`.

- `CONFIG_EXAMPLES_I2SCHAR_BUFSIZE` – The size of the data payload in one audio
  buffer. Applies to both TX and RX audio buffers.

- `CONFIG_EXAMPLES_I2SCHAR_DEVINIT` – Define if architecture-specific I2S device
  initialize is available. If defined, the platform specific code must provide a
  function `i2schar_devinit()` that will be called each time that this test
  executes. Not available in the kernel build mode.

## `ina219` Current/Power Monitor `INA219`

This is a simple infinite loop that polls the `INA219` sensor and displays the
measurements.

## `ipforward` IP Forwarding Using TUN

A simple test of IP forwarding using TUN devices. This can be used on any
platform, but was intended for use on the simulation platform because it
performs a test of IP forwarding without the use of hardware.

## `json` cJSON

This example exercises the cJSON implementation at `apps/netutils/json`. This
example contains logic taken from the cJSON project:

http://sourceforge.net/projects/cjson/

The example corresponds to SVN revision `r42` (with lots of changes for NuttX
coding standards). As of `r42`, the SVN repository was last updated on
`2011-10-10` so I presume that the code is stable and there is no risk of
maintaining duplicate logic in the NuttX repository.

## `leds` Toggle LEDs

This is a simple test of the board LED driver at
`nuttx/drivers/leds/userled_*.c`.

## `lis2csh_reader` `LIS3DSH` Accelerometer

A simple reader example for the `LIS3DSH` acceleration sensor as found on
STM32F4Discovery rev. C.

## `hts221_reader` `HTS221` Humidity Sensor

A simple reader example for the `HTS221` humidity sensor.

## `lsm303_reader` `LSM303` Accelerometer/Magnetometer

A simple reader example for the `LSM303` acc-mag sensor.

## `lsm6dsl_reader` `LSM6DSL` Accelerometer/Gyroscope

A simple reader example for the `LSM6DSL` acc-gyro sensor.

## `media`

The media test simply writes values onto the media hidden behind a character
driver and verifies that the media can be successfully written and read. This
low level test is useful in the early phases of the bringup of a new block or
mtd driver because it avoids the complexity of a file system.

This test uses a character driver and cannot directly access block or mtd
drivers. This test is suitable for use EEPROM character drivers (see
`nuttx/drivers/eeprom`), or with block drivers wrapped as character drivers (see
`nuttx/drivers/bch`)

```c
int ret = bchdev_register(<path-to-block-dirver>,
                          <path-to-character-driver>, false);
```

MTD drivers need an additional wrapper layer, the FTL wrapper must first be used
to convert the MTD driver to a block device:

```c
int ret = ftl_initialize(<N>, mtd);
ret = bchdev_register(/dev/mtdblock<N>, <path-to-character-driver>,
                      false);
```

## `module` Loadable Module

This example builds a small loadable module test case. This includes a character
driver under `examples/module/drivers`. This driver is built using the
relocatable ELF format and installed in a ROMFS file system. At run time, the
driver module is loaded and exercised. Requires `CONFIG_MODULE`. Other
configuration options:

- `CONFIG_EXAMPLES_ELF_DEVMINOR` – The minor device number of the ROMFS block
  driver. For example, the `N` in `/dev/ramN`. Used for registering the RAM
  block driver that will hold the ROMFS file system containing the ELF
  executables to be tested. Default: `0`.

- `CONFIG_EXAMPLES_ELF_DEVPATH` – The path to the ROMFS block driver device.
  This must match `EXAMPLES_ELF_DEVMINOR`. Used for registering the RAM block
  driver that will hold the ROMFS file system containing the ELF executables to
  be tested. Default: `/dev/ram0`.

**Notes**:

1. `CFLAGS` should be provided in `CMODULEFLAGS`. RAM and FLASH memory regions
   may require long allcs. For ARM, this might be:

   ```makefile
   CMODULEFLAGS = $(CFLAGS) -mlong-calls
   ```

   Similarly for C++ flags which must be provided in `CXXMODULEFLAGS`.

2. Your top-level `nuttx/Make.defs` file must also include an appropriate
   definition, LDMODULEFLAGS, to generate a relocatable ELF object. With GNU LD,
   this should include `-r` and `-e <entry point>`.

   ```makefile
   LDMODULEFLAGS = -r -e module_initialize
   ```

   If you use GCC to link, you make also need to include `-nostdlib` or
   `-nostartfiles` and `-nodefaultlibs`.

3. This example also requires `genromfs`. `genromfs` can be build as part of the
   nuttx toolchain. Or can built from the `genromfs` sources that can be found
   in the NuttX tools repository (`genromfs-0.5.2.tar.gz`). In any event, the
   PATH variable must include the path to the `genromfs` executable.

4. ELF size: The ELF files in this example are, be default, quite large because
   they include a lot of _build garbage_. You can greatly reduce the size of the
   ELF binaries are using the `objcopy --strip-unneeded` command to remove
   un-necessary information from the ELF files.

5. Simulator. You cannot use this example with the NuttX simulator on Cygwin.
   That is because the Cygwin GCC does not generate ELF file but rather some
   Windows-native binary format.

   If you really want to do this, you can create a NuttX x86 `buildroot`
   toolchain and use that be build the ELF executables for the ROMFS file
   system.

6. Linker scripts. You might also want to use a linker scripts to combine
   sections better. An example linker script is at
   `nuttx/libc/modlib/gnu-elf.ld`. That example might have to be tuned for your
   particular linker output to position additional sections correctly. The GNU
   LD `LDMODULEFLAGS` then might be:

   ```makefile
   LDMODULEFLAGS = -r -e module_initialize -T$(TOPDIR)/libc/modlib/gnu-elf.ld
   ```

## `modbus` FreeModbus

This is a port of the FreeModbus Linux demo. It derives from the demos/LINUX
directory of the FreeModBus version `1.5.0` (June 6, 2010) that can be
downloaded in its entirety from
http://developer.berlios.de/project/showfiles.php?group_id=6120.

- `CONFIG_EXAMPLES_MODBUS_PORT`, Default `0` (for `/dev/ttyS0`).
- `CONFIG_EXAMPLES_MODBUS_BAUD`, Default B`38400`.
- `CONFIG_EXAMPLES_MODBUS_PARITY`, Default `MB_PAR_EVEN`.
- `CONFIG_EXAMPLES_MODBUS_REG_INPUT_START`, Default `1000`.
- `CONFIG_EXAMPLES_MODBUS_REG_INPUT_NREGS`, Default `4`.
- `CONFIG_EXAMPLES_MODBUS_REG_HOLDING_START`, Default `2000`.
- `CONFIG_EXAMPLES_MODBUS_REG_HOLDING_NREGS`, Default `130`.

The FreeModBus library resides at `apps/modbus`. See `apps/modbus/README.txt`
for additional configuration information.

## `mount` Mount Filesystem

This contains a simple test of filesystem mountpoints.

- `CONFIG_EXAMPLES_MOUNT_DEVNAME` – The name of the user-provided block device
  to mount. If `CONFIG_EXAMPLES_MOUNT_DEVNAME` is not provided, then a RAM disk
  will be configured.

- `CONFIG_EXAMPLES_MOUNT_NSECTORS` – The number of _sectors_ in the RAM disk
  used when `CONFIG_EXAMPLES_MOUNT_DEVNAME` is not defined.

- `CONFIG_EXAMPLES_MOUNT_SECTORSIZE` – The size of each sectors in the RAM disk
  used when `CONFIG_EXAMPLES_MOUNT_DEVNAME` is not defined.

- `CONFIG_EXAMPLES_MOUNT_RAMDEVNO` – The RAM device minor number used to mount
  the RAM disk used when `CONFIG_EXAMPLES_MOUNT_DEVNAME` is not defined. The
  default is zero (meaning that `/dev/ram0` will be used).

## `mtdpart` MTD Partition Test

This examples provides a simple test of MTD partition logic.

- `CONFIG_EXAMPLES_MTDPART` – Enables the MTD partition test example.

- `CONFIG_EXAMPLES_MTDPART_ARCHINIT` – The default is to use the RAM MTD device
  at `drivers/mtd/rammtd.c`. But an architecture-specific MTD driver can be used
  instead by defining `CONFIG_EXAMPLES_MTDPART_ARCHINIT`. In this case, the
  initialization logic will call `mtdpart_archinitialize()` to obtain the MTD
  driver instance.

- `CONFIG_EXAMPLES_MTDPART_NPARTITIONS` – This setting provides the number of
  partitions to test. The test will divide the reported size of the MTD device
  into equal-sized sub-regions for each test partition. Default: `3`.

When `CONFIG_EXAMPLES_MTDPART_ARCHINIT` is not defined, this test will use the
RAM MTD device at `drivers/mtd/rammtd.c` to simulate FLASH. The size of the
allocated RAM drive will be: `CONFIG_EXMPLES_RAMMTD_ERASESIZE *
CONFIG_EXAMPLES_MTDPART_NEBLOCKS`.

* `CONFIG_EXAMPLES_MTDPART_ERASESIZE` – This value gives the size of one erase
  block in the MTD RAM device. This must exactly match the default configuration
  in `drivers/mtd/rammtd.c`!

* `CONFIG_EXAMPLES_MTDPART_NEBLOCKS` – This value gives the number of erase
  blocks in MTD RAM device.

## `mtdrwb` MTD Read-ahead and Write Buffering

This examples provides a simple test of MTD Read-Ahead/Write buffering logic.

- `CONFIG_EXAMPLES_MTDRWB` – Enables the MTD R/W buffering test example.

- `CONFIG_EXAMPLES_MTDRWB_ARCHINIT` – The default is to use the RAM MTD device
  at `drivers/mtd/rammtd.c`. But an architecture-specific MTD driver can be used
  instead by defining `CONFIG_EXAMPLES_MTDRWB_ARCHINIT`. In this case, the
  initialization logic will call `mtdrwb_archinitialize()` to obtain the MTD
  driver instance.

When `CONFIG_EXAMPLES_MTDRWB_ARCHINIT` is not defined, this test will use the
RAM MTD device at `drivers/mtd/rammtd.c` to simulate FLASH. The size of the
allocated RAM drive will be: `CONFIG_EXMPLES_RAMMTD_ERASESIZE *
CONFIG_EXAMPLES_MTDRWB_NEBLOCKS`

- `CONFIG_EXAMPLES_MTDRWB_ERASESIZE` – This value gives the size of one erase
  block in the MTD RAM device. This must exactly match the default configuration
  in `drivers/mtd/rammtd.c`!

- `CONFIG_EXAMPLES_MTDRWB_NEBLOCKS` – This value gives the number of erase
  blocks in MTD RAM device.

## `netpkt` `AF_PACKET` _Raw_ Sockets

A test of `AF_PACKET`, _raw_ sockets. Contributed by Lazlo Sitzer.

## `netloop` Network loopback device

This is a simple test of the netwok loopback device. `examples/nettest` can also
be configured to provide (better) test of local loopback transfers. This version
derives from `examples/poll` and is focused on testing `poll()` with loopback
devices.

- `CONFIG_EXAMPLES_NETLOOP=y` – Enables the nettest example.

Dependencies:

- `CONFIG_NET_LOOPBACK` – Requires local loopback support.
- `CONFIG_NET_TCP` – Requires TCP support with the following:
   - `CONFIG_NET_TCPBACKLOG`
   - `CONFIG_NET_TCP_WRITE_BUFFERS`
- `CONFIG_NET_IPv4` – Currently supports only IPv4.

## `nettest` Client/Server Over TCP

This is a simple network test for verifying client- and server- functionality in
a TCP/IP connection.

- `CONFIG_EXAMPLES_NETTEST=y` – Enables the nettest example.
- `CONFIG_EXAMPLES_NETLIB=y` – The networking library in needed.

Configurations:

- Server on target hardware; client on host.
- Client on target hardware; server on host.
- Server and Client on different targets.
- Loopback configuration with both client and server on the same target.

See also `examples/tcpecho`.

## `nrf24l01_term` NRF24L01 Wireless Connection

These is a simple test of NRF24L01-based wireless connectivity. Enabled with:

- `CONFIG_EXAMPLES_NRF24L01TERM`

Options:

- `CONFIG_NSH_BUILTIN_APPS` – Built as an NSH built-in applications.

## `nx`

This directory contains a simple test of a subset of the NX APIs defined in
`include/nuttx/nx/nx.h`. The following configuration options can be selected:

- `CONFIG_NSH_BUILTIN_APPS` – Build the NX example as a _built-in_ that can be
  executed from the NSH command line
- `CONFIG_EXAMPLES_NX_BGCOLOR` – The color of the background. Default depends on
  `CONFIG_EXAMPLES_NX_BPP`.
- `CONFIG_EXAMPLES_NX_COLOR1` – The color of window 1. Default depends on
  `CONFIG_EXAMPLES_NX_BPP`.
- `CONFIG_EXAMPLES_NX_COLOR2` – The color of window 2. Default depends on
  `CONFIG_EXAMPLES_NX_BPP`.
- `CONFIG_EXAMPLES_NX_TBCOLOR` – The color of the toolbar. Default depends on
  `CONFIG_EXAMPLES_NX_BPP`.
- `CONFIG_EXAMPLES_NX_FONTID` – Selects the font (see font ID numbers in
  `include/nuttx/nx/nxfonts.h`).
- `CONFIG_EXAMPLES_NX_FONTCOLOR` – The color of the fonts. Default depends on
  `CONFIG_EXAMPLES_NX_BPP`.
- `CONFIG_EXAMPLES_NX_BPP` – Pixels per pixel to use. Valid options include `2`,
  `4`, `8`, `16`, `24` and `32`. Default is `32`.
- `CONFIG_EXAMPLES_NX_RAWWINDOWS` – Use raw windows; Default is to use pretty,
  framed NXTK windows with toolbars.
- `CONFIG_EXAMPLES_NX_STACKSIZE` – The stacksize to use when creating the NX
  server. Default `2048`.
- `CONFIG_EXAMPLES_NX_CLIENTPRIO` – The client priority. Default: `100`
- `CONFIG_EXAMPLES_NX_SERVERPRIO` – The server priority. Default: `120`
- `CONFIG_EXAMPLES_NX_LISTENERPRIO` – The priority of the event listener thread.
  Default `80`.
- `CONFIG_EXAMPLES_NX_NOTIFYSIGNO` – The signal number to use with
  `nx_eventnotify()`. Default: `4`.

The example also has the following settings and will generate an error if they
are not as expected:

```conf
CONFIG_DISABLE_MQUEUE=n
CONFIG_DISABLE_PTHREAD=n
CONFIG_NX_BLOCKING=y
CONFIG_LIB_BOARDCTL=y
```

## `nxterm` Display NuttShell (NSH) as NX Console

This directory contains yet another version of the NuttShell (NSH). This version
uses the NX console device defined in `include/nuttx/nx/nxterm.h` for output.
the result is that the NSH input still come from the standard console input
(probably a serial console). But the text output will go to an NX winbdow.
Prerequisite configuration settings for this test include:

- `CONFIG_NX=y`              – NX graphics must be enabled
- `CONFIG_NXTERM=y`          – The NX console driver must be built
- `CONFIG_DISABLE_MQUEUE=n`  – Message queue support must be available.
- `CONFIG_DISABLE_PTHREAD=n` – pthreads are needed
- `CONFIG_NX_BLOCKING=y`     – pthread APIs must be blocking
- `CONFIG_NSH_CONSOLE=y`     – NSH must be configured to use a console.

The following configuration options can be selected to customize the test:

- `CONFIG_EXAMPLES_NXTERM_BGCOLOR` – The color of the background. Default
  Default is a darker royal blue.
- `CONFIG_EXAMPLES_NXTERM_WCOLOR` – The color of the window. Default is a light
  slate blue.
- `CONFIG_EXAMPLES_NXTERM_FONTID` – Selects the font (see font ID numbers in
  `include/nuttx/nx/nxfonts.h`).
- `CONFIG_EXAMPLES_NXTERM_FONTCOLOR` – The color of the fonts. Default is black.
- `CONFIG_EXAMPLES_NXTERM_BPP` – Pixels per pixel to use. Valid options include
  `2`, `4`, `8`, `16`, `24` and `32`. Default is `32`.
- `CONFIG_EXAMPLES_NXTERM_TOOLBAR_HEIGHT` – The height of the toolbar. Default:
  `16`.
- `CONFIG_EXAMPLES_NXTERM_TBCOLOR` – The color of the toolbar. Default is a
  medium grey.
- `CONFIG_EXAMPLES_NXTERM_MINOR` – The NX console device minor number. Default
  is `0` corresponding to `/dev/nxterm0`.
- `CONFIG_EXAMPLES_NXTERM_DEVNAME` – The quoted, full path to the NX console
  device corresponding to `CONFIG_EXAMPLES_NXTERM_MINOR`. Default:
  `/dev/nxterm0`.
- `CONFIG_EXAMPLES_NXTERM_PRIO` – Priority of the NxTerm task. Default:
  `SCHED_PRIORITY_DEFAULT`.
- `CONFIG_EXAMPLES_NXTERM_STACKSIZE` – Stack size allocated for the NxTerm task.
  Default: `2048`.
- `CONFIG_EXAMPLES_NXTERM_STACKSIZE` – The stacksize to use when creating the NX
  server. Default: `2048`.
- `CONFIG_EXAMPLES_NXTERM_CLIENTPRIO` – The client priority. Default: `100`.
- `CONFIG_EXAMPLES_NXTERM_SERVERPRIO` – The server priority. Default: `120`.
- `CONFIG_EXAMPLES_NXTERM_LISTENERPRIO` – The priority of the event listener
  thread. Default: `80`.
- `CONFIG_EXAMPLES_NXTERM_NOTIFYSIGNO` – The signal number to use with
  `nx_eventnotify()`. Default: `4`.

## `nxflat` NXFLAT Binary

This example builds a small NXFLAT test case. This includes several test
programs under `examples/nxflat` tests. These tests are build using the NXFLAT
format and installed in a ROMFS file system. At run time, each program in the
ROMFS file system is executed. Requires `CONFIG_NXFLAT`.

## `nxhello`

A very simple graphics example that just says _Hello, World!_ in the center of
the display.

The following configuration options can be selected:

- `CONFIG_NSH_BUILTIN_APPS` – Build the `NXHELLO` example as a _built-in_ that
  can be executed from the NSH command line
- `CONFIG_EXAMPLES_NXHELLO_VPLANE` – The plane to select from the frame- buffer
  driver for use in the test. Default: `0`.
- `CONFIG_EXAMPLES_NXHELLO_DEVNO` – The LCD device to select from the LCD driver
  for use in the test. Default: `0`.
- `CONFIG_EXAMPLES_NXHELLO_BGCOLOR` – The color of the background. Default
  depends on `CONFIG_EXAMPLES_NXHELLO_BPP`.
- `CONFIG_EXAMPLES_NXHELLO_FONTID` – Selects the font (see font ID numbers in
  include/nuttx/nx/nxfonts.h).
- `CONFIG_EXAMPLES_NXHELLO_FONTCOLOR` – The color of the fonts used in the
  background window. Default depends on `CONFIG_EXAMPLES_NXHELLO_BPP`.
- `CONFIG_EXAMPLES_NXHELLO_BPP` – Pixels per pixel to use. Valid options include
  `2`, `4`, `8`, `16`, `24` and `32`. Default: `32`.

## `nximage` Display NuttX Logo

This is a simple example that just puts the NuttX logo image in the center of
the display. This only works for `RGB23` (`888`), `RGB16` (`656`), `RGB8`
(`332`), and 8-bit greyscale for now.

- `CONFIG_NSH_BUILTIN_APPS` – Build the `NXIMAGE` example as a _built-in_ that
  can be executed from the NSH command line.
- `CONFIG_EXAMPLES_NXIMAGE_VPLANE` – The plane to select from the frame- buffer
  driver for use in the test. Default: `0`.
- `CONFIG_EXAMPLES_NXIMAGE_DEVNO` – The LCD device to select from the LCD driver
  for use in the test: Default: `0`.
- `CONFIG_EXAMPLES_NXIMAGE_BPP` – Pixels per pixel to use. Valid options include
  `8`, `16` and `24`. Default is `16`.
- `CONFIG_EXAMPLES_NXIMAGE_XSCALEp5`, `CONFIG_EXAMPLES_NXIMAGE_XSCALE1p5` or
  `CONFIG_EXAMPLES_NXIMAGE_XSCALE2p0` – The logo image width is 160 columns. One
  of these may be defined to rescale the image horizontally by .5, 1.5 or 2.0.
- `CONFIG_EXAMPLES_NXIMAGE_YSCALEp5`, `CONFIG_EXAMPLES_NXIMAGE_YSCALE1p5` or
  `CONFIG_EXAMPLES_NXIMAGE_YSCALE2p0` – The logo image height is 160 rows. One
  of these may be defined to rescale the image vertically by .5, 1.5 or 2.0.
- `CONFIG_EXAMPLES_NXIMAGE_GREYSCALE` – Grey scale image. Default: `RGB`.

How was that run-length encoded image produced?

1. I used GIMP output the image as a `.c` file.  
2. I added some C logic to palette-ize the RGB image in the GIMP `.c` file.  
3. Then I add some simple run-length encoding to palette-ized image.  

But now there is a tool that can be found in the NxWidgets package at
`NxWidgets/tools/bitmap_converter.py` that can be used to convert any graphics
format to the NuttX RLE format.

**Note**: As of this writing, most of the pixel depth, scaling options, and
combinations thereof have not been tested.

## `nxlines` NX Line Drawing

A very simple graphics example that just exercised the NX line drawing logic.

The following configuration options can be selected:

- `CONFIG_EXAMPLES_NXLINES_VPLANE` – The plane to select from the frame- buffer
  driver for use in the test. Default: `0`.
- `CONFIG_EXAMPLES_NXLINES_DEVNO` – The LCD device to select from the LCD driver
  for use in the test: Default: `0`.
- `CONFIG_EXAMPLES_NXLINES_BGCOLOR` – The color of the background. Default
  depends on `CONFIG_EXAMPLES_NXLINES_BPP`.
- `CONFIG_EXAMPLES_NXLINES_LINEWIDTH` – Selects the width of the lines in pixels
  (default: `16`).
- `CONFIG_EXAMPLES_NXLINES_LINECOLOR` – The color of the central lines drawn in
  the background window. Default depends on `CONFIG_EXAMPLES_NXLINES_BPP` (there
  really is no meaningful default).
- `CONFIG_EXAMPLES_NXLINES_BORDERWIDTH` – The width of the circular border drawn
  in the background window. (default: `16`).
- `CONFIG_EXAMPLES_NXLINES_BORDERCOLOR` – The color of the circular border drawn
  in the background window. Default depends on `CONFIG_EXAMPLES_NXLINES_BPP`
  (there really is no meaningful default).
- `CONFIG_EXAMPLES_NXLINES_CIRCLECOLOR` – The color of the circular region
  filled in the background window. Default depends on
  `CONFIG_EXAMPLES_NXLINES_BPP` (there really is no meaningful default).
- `CONFIG_EXAMPLES_NXLINES_BORDERCOLOR` – The color of the lines drawn in the
  background window. Default depends on `CONFIG_EXAMPLES_NXLINES_BPP` (there
  really is no meaningful default).
- `CONFIG_EXAMPLES_NXLINES_BPP` – Pixels per pixel to use. Valid options include
  `2`, `4`, `8`, `16`, `24`, and `32`. Default is `16`.
- `CONFIG_NSH_BUILTIN_APPS` – Build the NX lines examples as an NSH built-in
  function.

## `nxtext` Display NX Text

This directory contains another simple test of a subset of the NX APIs defined
in `include/nuttx/nx/nx.h`. This text focuses on text displays on the display
background combined with pop-up displays over the text. The text display will
continue to update while the pop-up is visible.

**Note**: This example will **only** work with FB drivers and with LCD drivers
that support reading the contents of the internal LCD memory **unless** you
define `CONFIG_EXAMPLES_NXTEXT_NOGETRUN`. If you notice garbage on the display
or a failure at the point where the display should scroll, it is probably
because you have an LCD driver that is write-only.

The following configuration options can be selected:

- `CONFIG_NSH_BUILTIN_APPS` – Build the `NXTEXT` example as a _built-in_ that
  can be executed from the NSH command line.
- `CONFIG_EXAMPLES_NXTEXT_BGCOLOR` – The color of the background. Default
  depends on `CONFIG_EXAMPLES_NXTEXT_BPP`.
- `CONFIG_EXAMPLES_NXTEXT_BGFONTID` – Selects the font to use in the background
  text (see font ID numbers in `include/nuttx/nx/nxfonts.h`).
- `CONFIG_EXAMPLES_NXTEXT_BGFONTCOLOR` – The color of the fonts used in the
  background window. Default depends on `CONFIG_EXAMPLES_NXTEXT_BPP`.
- `CONFIG_EXAMPLES_NXTEXT_PUCOLOR` – The color of the pop-up window. Default
  depends on `CONFIG_EXAMPLES_NXTEXT_BPP`.
- `CONFIG_EXAMPLES_NXTEXT_PUFONTID` – Selects the font to use in the pop-up
  windows (see font ID numbers in `include/nuttx/nx/nxfonts.h`).
- `CONFIG_EXAMPLES_NXTEXT_PUFONTCOLOR` – The color of the fonts used in the
  background window. Default depends on `CONFIG_EXAMPLES_NXTEXT_BPP`.
- `CONFIG_EXAMPLES_NXTEXT_BPP` – Pixels per pixel to use. Valid options include
  `2`, `4`, `8`, `16`, `24` and `32`. Default is `32`.
- `CONFIG_EXAMPLES_NXTEXT_NOGETRUN` – If your display is read-only OR if reading
  is not reliable, then select this configuration to avoid reading from the
  display.
- `CONFIG_EXAMPLES_NXTEXT_BMCACHE` – The maximum number of characters that can
  be put in the background window. Default is `128`.
- `CONFIG_EXAMPLES_NXTEXT_GLCACHE` – The maximum number of pre-rendered fonts
  that can be retained for the background window.
- `CONFIG_EXAMPLES_NXTEXT_STACKSIZE` – The stacksize to use when creating the NX
  server. Default `2048`.
- `CONFIG_EXAMPLES_NXTEXT_CLIENTPRIO` – The client priority. Default: `100`.
- `CONFIG_EXAMPLES_NXTEXT_SERVERPRIO` – The server priority. Default: `120`.
- `CONFIG_EXAMPLES_NXTEXT_LISTENERPRIO` – The priority of the event listener
  thread. Default: `80`.
- `CONFIG_EXAMPLES_NXTEXT_NOTIFYSIGNO` – The signal number to use with
  `nx_eventnotify()`. Default: `4`.

The example also expects the following settings and will generate an error if
they are not as expected:

```conf
CONFIG_DISABLE_MQUEUE=n
CONFIG_DISABLE_PTHREAD=n
CONFIG_NX_BLOCKING=y
```

## `null`

This is the do nothing application. It is only used for bringing up new NuttX
architectures in the most minimal of environments.

## `obd2`

A simple test of `apps/canutils/libobd2`.

## `oneshot` Oneshot Timer

Simple test of a oneshot driver.

## `pca9635` `PCA9635PW` LED

A simple test of the `PCA9635PW` LED driver.

## `pdcurses`

This directory contains the `demo/test` programs that accompany the public
domain cursors package (`pdcurses`) that can be found at
`apps/graphics/pdcurs34`.

## `pipe`

A test of the `mkfifo()` and `pipe()` APIs. Requires `CONFIG_PIPES`

- `CONFIG_EXAMPLES_PIPE_STACKSIZE` – Sets the size of the stack to use when
  creating the child tasks. The default size is `1024`.

## `poll`

A test of the `poll()` and `select()` APIs using FIFOs and, if available,
`stdin`, and a TCP/IP socket. In order to use the TCP/IP select test, you must
have the following things selected in your NuttX configuration file:

- `CONFIG_NET`                        – Defined for general network support.
- `CONFIG_NET_TCP`                    – Defined for TCP/IP support.
- `CONFIG_NET_NTCP_READAHEAD_BUFFERS` – Defined to be greater than zero.
- `CONFIG_EXAMPLES_POLL_NOMAC`        – (May be defined to use software assigned
  MAC)
- `CONFIG_EXAMPLES_POLL_IPADDR`       – Target IP address.
- `CONFIG_EXAMPLES_POLL_DRIPADDR`     – Default router IP address.
- `CONFIG_EXAMPLES_POLL_NETMASK`      – Network mask.

In order to for select to work with incoming connections, you must also select:

- `CONFIG_NET_TCPBACKLOG` – Incoming connections pend in a backlog until
  `accept()` is called.

In additional to the target device-side example, there is also a host-side
application in this directory. It can be compiled under Linux or Cygwin as
follows:

```makefile
cd examples/usbserial
make -f Makefile.host TOPDIR=<nuttx-directory> TARGETIP=<target-ip>
```

Where `<target-ip>` is the IP address of your target board.

This will generate a small program called 'host'. Usage:

1. Build the `examples/poll` target program with TCP/IP poll support and start
   the target.

2. Then start the host application:

   ```bash
   ./host
   ```

The host and target will exchange are variety of small messages. Each message
sent from the host should cause the select to return in target. The target
example should read the small message and send it back to the host. The host
should then receive the echo'ed message.

If networking is enabled, applications using this example will need to provide
the following definition in the `defconfig` file to enable the networking
library:

- `CONFIG_NETUTILS_NETLIB=y`

## `posix_spawn`

This is a simple test of the `posix_spawn()` API. The example derives from
`examples/elf`. As a result, these tests are built using the relocatable ELF
format installed in a ROMFS file system. At run time, the test program in the
ROMFS file system is spawned using `posix_spawn()`.

Requires:

- `CONFIG_BINFMT_DISABLE=n` – Don't disable the binary loader.
- `CONFIG_ELF=y` – Enable ELF binary loader.
- `CONFIG_LIBC_EXECFUNCS=y` – Enable support for posix_spawn.
- `CONFIG_EXECFUNCS_SYMTAB_ARRAY="g_spawn_exports"` – The name of the symbol
  table created by the test.
- `CONFIG_EXECFUNCS_NSYMBOLS_VAR="g_spawn_nexports"` – Name of variable holding
  the number of symbols.
- `CONFIG_POSIX_SPAWN_STACKSIZE=768` – This default setting.

Test-specific configuration options:

- `CONFIG_EXAMPLES_POSIXSPAWN_DEVMINOR` – The minor device number of the ROMFS
  block. driver. For example, the `N` in `/dev/ramN`. Used for registering the
  RAM block driver that will hold the ROMFS file system containing the ELF
  executables to be tested. Default: `0`.

- `CONFIG_EXAMPLES_POSIXSPAWN_DEVPATH` – The path to the ROMFS block driver
  device. This must match `EXAMPLES_POSIXSPAWN_DEVMINOR`. Used for registering
  the RAM block driver that will hold the ROMFS file system containing the ELF
  executables to be tested. Default: `/dev/ram0`.

**Notes**:

1. `CFLAGS` should be provided in `CELFFLAGS`. RAM and FLASH memory regions may
    require long allcs. For ARM, this might be:

    ```makefile
    CELFFLAGS = $(CFLAGS) -mlong-calls
    ```

    Similarly for C++ flags which must be provided in `CXXELFFLAGS`.

2. Your top-level `nuttx/Make.defs` file must also include an appropriate
   definition, `LDELFFLAGS`, to generate a relocatable ELF object. With GNU LD,
   this should include `-r` and `-e main` (or `_main` on some platforms).

   ```makefile
   LDELFFLAGS = -r -e main
   ```

   If you use GCC to link, you make also need to include `-nostdlib` or
   `-nostartfiles` and `-nodefaultlibs`.

3. This example also requires `genromfs`. `genromfs` can be build as part of the
   nuttx toolchain. Or can built from the `genromfs` sources that can be found
   in the NuttX tools repository (`genromfs-0.5.2.tar.gz`). In any event, the
   `PATH` variable must include the path to the `genromfs` executable.

4. ELF size: The ELF files in this example are, be default, quite large because
   they include a lot of _build garbage_. You can greatly reduce the size of the
   ELF binaries are using the `objcopy --strip-unneeded` command to remove
   un-necessary information from the ELF files.

5. Simulator. You cannot use this example with the NuttX simulator on Cygwin.
   That is because the Cygwin GCC does not generate ELF file but rather some
   Windows-native binary format.

   If you really want to do this, you can create a NuttX x86 buildroot toolchain
   and use that be build the ELF executables for the ROMFS file system.

6. Linker scripts. You might also want to use a linker scripts to combine
   sections better. An example linker script is at
   `nuttx/binfmt/libelf/gnu-elf.ld`. That example might have to be tuned for
   your particular linker output to position additional sections correctly. The
   GNU LD `LDELFFLAGS` then might be:

   ```makefile
   LDELFFLAGS = -r -e main -T$(TOPDIR)/binfmt/libelf/gnu-elf.ld
   ```

## `powerled`

This is a powerled driver example application. This application support three
operation modes which can be selected from NSH command line:

1. Demo mode.
2. Continuous mode.
3. Flash mode.

## `pty_test` Pseudo-Terminals

A test of NuttX pseudo-terminals. Provided by Alan Carvalho de Assis.

## `pwfb`

A graphics example using pre-window frame buffers. The example shows three
windows containing text moving around, crossing each other from _above_ and from
_below_. The example application is NOT updating the windows any anyway! The
application is only changing the window position. The windows are being updated
from the per-winidow framebuffers automatically.

This example is reminiscent of Pong: Each window travels in straight line until
it hits an edge, then it bounces off. The window is also raised when it hits the
edge (gets _focus_). This tests all combinations of overap.

**Note**: A significant amount of RAM, usually external SDRAM, is required to
run this demo. At 16bpp and a 480x272 display, each window requires about 70Kb
of RAM for its framebuffer.

## `pwm` General PWM

A test of a PWM device driver. It simply enables a pulsed output for a specified
frequency and duty for a specified period of time. This example can ONLY be
built as an NSH built-in function.

This test depends on these specific PWM/NSH configurations settings (your
specific PWM settings might require additional settings).

- `CONFIG_PWM` – Enables PWM support.
- `CONFIG_PWM_PULSECOUNT` – Enables PWM pulse count support (if the hardware
  supports it).
- `CONFIG_NSH_BUILTIN_APPS` – Build the PWM test as an NSH built-in function.

Specific configuration options for this example include:

- `CONFIG_EXAMPLES_PWM_DEVPATH` – The path to the default PWM device. Default:
  `/dev/pwm0`.
- `CONFIG_EXAMPLES_PWM_FREQUENCY` – The initial PWM frequency. Default: `100` Hz
- `CONFIG_EXAMPLES_PWM_DUTYPCT` – The initial PWM duty as a percentage. Default:
  `50`%.
- `CONFIG_EXAMPLES_PWM_DURATION` – The initial PWM pulse train duration in
  seconds. Used only if the current pulse count is zero (pulse count is only
  supported if `CONFIG_PWM_PULSECOUNT` is defined). Default: `5` seconds.
- `CONFIG_EXAMPLES_PWM_PULSECOUNT` – The initial PWM pulse count. This option is
  only available if `CONFIG_PWM_PULSECOUNT` is non-zero. Default: `0` (i.e., use
  the duration, not the count).

## `qencoder` Quadrature Encoder

This example is a simple test of a Quadrature Encoder driver. It simply reads
positional data from the encoder and prints it.,

This test depends on these specific QE/NSH configurations settings (your
specific PWM settings might require additional settings).

- `CONFIG_SENSORS_QENCODER` – Enables quadrature encoder support (upper-half
  driver).
- `CONFIG_NSH_BUILTIN_APPS` – Build the QE test as an NSH built-in function.
  Default: Built as a standalone program.

Additional configuration options will mostly likely be required for the board-
specific lower-half driver. See the `README.txt` file in your board
configuration directory.

Specific configuration options for this example include:

- `CONFIG_EXAMPLES_QENCODER_DEVPATH` – The path to the QE device. Default:
  `/dev/qe0`.
- `CONFIG_EXAMPLES_QENCODER_NSAMPLES` – This number of samples is collected and
  the program terminates. Default: Samples are collected indefinitely.
- `CONFIG_EXAMPLES_QENCODER_DELAY` – This value provides the delay (in
  milliseconds) between each sample. Default: `100` milliseconds.

## `random` Random Numbers

This is a very simply test of `/dev/random`. It simple collects random numbers
and displays them on the console.

Prerequistes:

- `CONFIG_DEV_RANDOM` – Support for `/dev/random` must be enabled in order to
  select this example.

Configuration:

- `CONFIG_EXAMPLES_RANDOM` – Enables the `/dev/random` test.
- `CONFIG_EXAMPLES_MAXSAMPLES` – This is the size of the `/dev/random` I/O
  buffer in units of 32-bit samples. Careful! This buffer is allocated on the
  stack as needed! Default `64`.
- `CONFIG_EXAMPLES_NSAMPLES` – When you execute the `rand` command, a number of
  samples ranging from `1` to `EXAMPLES_MAXSAMPLES` may be specified. If no
  argument is specified, this is the default number of samples that will be
  collected and displayed. Default `8`.

## `relays` Relays

Requires `CONFIG_ARCH_RELAYS`. Contributed by Darcy Gong.

**Note**: This test exercises internal relay driver interfaces. As such, it
relies on internal OS interfaces that are not normally available to a user-space
program. As a result, this example cannot be used if a NuttX is built as a
protected, supervisor kernel (`CONFIG_BUILD_PROTECTED` or
`CONFIG_BUILD_KERNEL`).

## `rfid_readuid` RFID

RFID `READUID` example.

## `rgbled` RGB LED Using PWM

This example demonstrates the use of the RGB led driver to drive an RGB LED with
PWM outputs so that all color characteristcs of RGB LED can be controlled.

## `romfs` File System

This example exercises the romfs filesystem. Configuration options include:

- `CONFIG_EXAMPLES_ROMFS_RAMDEVNO` – The minor device number to use for the ROM
  disk. The default is `1` (meaning `/dev/ram1`).
- `CONFIG_EXAMPLES_ROMFS_SECTORSIZE` – The ROM disk sector size to use. Default
  is `64`.
- `CONFIG_EXAMPLES_ROMFS_MOUNTPOINT` – The location to mount the ROM disk.
  Default: `/usr/local/share`.

## `sendmail` SMTP Client

This examples exercises the uIP SMTP logic by sending a test message to a
selected recipient. This test can also be built to execute on the Cygwin/Linux
host environment:

```bash
cd examples/sendmail
make -f Makefile.host TOPDIR=<nuttx-directory>
```

Settings unique to this example include:

- `CONFIG_EXAMPLES_SENDMAIL_NOMAC`     – May be defined to use software assigned
  MAC (optional)
- `CONFIG_EXAMPLES_SENDMAIL_IPADDR`    – Target IP address (required)
- `CONFIG_EXAMPLES_SENDMAIL_DRIPADDR`  – Default router IP address (required)
- `CONFIG_EXAMPLES_SENDMAILT_NETMASK`  – Network mask (required)
- `CONFIG_EXAMPLES_SENDMAIL_RECIPIENT` – The recipient of the email (required)
- `CONFIG_EXAMPLES_SENDMAIL_SENDER`    – Optional. Default:
  `nuttx-testing@example.com`
- `CONFIG_EXAMPLES_SENDMAIL_SUBJECT`   – Optional. Default: `Testing SMTP from
  NuttX`
- `CONFIG_EXAMPLES_SENDMAIL_BODY`      – Optional. Default: `Test message sent
  by NuttX`

**Note 1**: This test has not been verified on the NuttX target environment. As
of this writing, unit-tested in the Cygwin/Linux host environment.

**Note 2**: This sendmail example only works for the simplest of environments.
Virus protection software on your host may have to be disabled to allow you to
send messages. Only very open, unprotected recipients can be used. Most will
protect themselves from this test email because it looks like SPAM.

Applications using this example will need to enable the following netutils
libraries in their defconfig file:

```conf
CONFIG_NETUTILS_NETLIB=y
CONFIG_NETUTILS_SMTP=y
```

## `serialblaster`

Sends a repeating pattern (the alphabet) out a serial port continuously. This
may be useful if you are trying run down other problems that you think might
only occur when the serial port usage is high.

## `serialrx`

Constant receives serial data. This is the complement to `serialblaster`. This
may be useful if you are trying run down other problems that you think might
only occur when the serial port usage is high.

## `serloop` Serial Loopback

This is a mindlessly simple loopback test on the console. Useful for testing new
serial drivers. Configuration options include:

- `CONFIG_EXAMPLES_SERLOOP_BUFIO` – Use C buffered I/O (`getchar`/`putchar`) vs.
   raw console I/O (read/read).

## `slcd` Alphanumeric Segment LCD

A simple test of alphanumeric, segment LCDs (SLCDs).

- `CONFIG_EXAMPLES_SLCD` – Enable the SLCD test

## `smps` Switched-Mode Power Supply

This is a SMPS (Switched-mode power supply) driver example application.

## `sotest` Shared Library Module Test

This example builds a small shared library module test case. The test shared
library is built using the relocatable ELF format and installed in a ROMFS file
system. At run time, the shared library is installed and exercised. Requires
`CONFIG_LIBC_DLFCN`. Other configuration options:

- `CONFIG_EXAMPLES_SOTEST_DEVMINOR` – The minor device number of the ROMFS block
  driver. For example, the `N` in `/dev/ramN`. Used for registering the RAM
  block driver that will hold the ROMFS file system containing the ELF
  executables to be tested. Default: `0`.

- `CONFIG_EXAMPLES_SOTEST_DEVPATH` – The path to the ROMFS block driver device.
  This must match `EXAMPLES_ELF_DEVMINOR`. Used for registering the RAM block
  driver that will hold the ROMFS file system containing the ELF executables to
  be tested. Default: `/dev/ram0`.

**Notes**:

1. `CFLAGS` should be provided in `CMODULEFLAGS`. RAM and FLASH memory regions
   may require long allcs. For ARM, this might be:

   ```makefile
   CMODULEFLAGS = $(CFLAGS) -mlong-calls
   ```

   Similarly for C++ flags which must be provided in `CXXMODULEFLAGS`.

2. Your top-level `nuttx/Make.defs` file must also include an appropriate
   definition, `LDMODULEFLAGS`, to generate a relocatable ELF object. With GNU
   LD, this should include `-r` and `-e <entry point>`.

   ```makefile
   LDMODULEFLAGS = -r -e module_initialize
   ```

   If you use GCC to link, you make also need to include `-nostdlib` or
   `-nostartfiles` and `-nodefaultlibs`.

3. This example also requires `genromfs`. `genromfs` can be build as part of the
   nuttx toolchain. Or can built from the `genromfs` sources that can be found
   in the NuttX tools repository (`genromfs-0.5.2.tar.gz`). In any event, the
   `PATH` variable must include the path to the `genromfs` executable.

4. ELF size: The ELF files in this example are, be default, quite large because
   they include a lot of _build garbage_. You can greatly reduce the size of the
   ELF binaries are using the `objcopy --strip-unneeded` command to remove
   un-necessary information from the ELF files.

5. Simulator. You cannot use this example with the NuttX simulator on Cygwin.
   That is because the Cygwin GCC does not generate ELF file but rather some
   Windows-native binary format.

   If you really want to do this, you can create a NuttX x86 buildroot toolchain
   and use that be build the ELF executables for the ROMFS file system.

6. Linker scripts. You might also want to use a linker scripts to combine
   sections better. An example linker script is at
   `nuttx/libc/modlib/gnu-elf.ld`. That example might have to be tuned for your
   particular linker output to position additional sections correctly. The GNU
   LD `LDMODULEFLAGS` then might be:

```makefile
LDMODULEFLAGS = -r -e module_initialize -T$(TOPDIR)/libc/modlib/gnu-elf.ld
```

## `stat`

A simple test of `stat()`, `fstat()`, and `statfs()`. This is useful primarily
for bringing up a new file system and verifying the correctness of these
operations.

## `sx127x_demo` `SX127X` Radio

This example demonstrates the use of the `SX127X` radio.

## `system`

This is a simple test of the `system()` command. The test simply executes this
`system` command:

```c
ret = system("ls -Rl /");
```

## `tcpblaster` TCP Performance Test

The `tcpblaster` example derives from the `nettest` example and basically
duplicates that example when the `nettest` PERFORMANCE option is selected.
`tcpblaster` has a little better reporting of performance stats, however.

## `tcpecho` TCP Echo Server

Simple single threaded, poll based TCP echo server. This example implements the
TCP Echo Server from W. Richard Stevens _UNIX Network Programming_ Book.
Contributed by Max Holtberg.

See also `examples/nettest`

- `CONFIG_EXAMPLES_TCPECHO=y` – Enables the TCP echo server.
- `CONFIG_XAMPLES_TCPECHO_PORT` – Server Port, default `80`.
- `CONFIG_EXAMPLES_TCPECHO_BACKLOG` – Listen Backlog, default `8`.
- `CONFIG_EXAMPLES_TCPECHO_NCONN` – Number of Connections, default `8`.
- `CONFIG_EXAMPLES_TCPECHO_DHCPC` – DHCP Client, default `n`.
- `CONFIG_EXAMPLES_TCPECHO_NOMAC` – Use Canned MAC Address, default `n`.
- `CONFIG_EXAMPLES_TCPECHO_IPADDR` – Target IP address, default `0x0a000002`.
- `CONFIG_EXAMPLES_TCPECHO_DRIPADDR` – Default Router IP address (Gateway),
  default `0x0a000001`.
- `CONFIG_EXAMPLES_TCPECHO_NETMASK` – Network Mask, default `0xffffff00`.

## `telnetd` Simple Telnet Shell

This directory contains a functional port of the tiny uIP shell. In the NuttX
environment, the NuttShell (at `apps/nshlib`) supersedes this tiny shell and
also supports `telnetd`.

- `CONFIG_EXAMPLES_TELNETD` – Enable the Telnetd example.
- `CONFIG_NETUTILS_NETLIB`, `CONFIG_NETUTILS_TELNED` – Enable netutils libraries
  needed by the Telnetd example.
- `CONFIG_EXAMPLES_TELNETD_DAEMONPRIO` – Priority of the Telnet daemon. Default:
  `SCHED_PRIORITY_DEFAULT`.
- `CONFIG_EXAMPLES_TELNETD_DAEMONSTACKSIZE` – Stack size allocated for the
  Telnet daemon. Default: `2048`.
- `CONFIG_EXAMPLES_TELNETD_CLIENTPRIO` – Priority of the Telnet client. Default:
  `SCHED_PRIORITY_DEFAULT`.
- `CONFIG_EXAMPLES_TELNETD_CLIENTSTACKSIZE` – Stack size allocated for the
  Telnet client. Default: `2048`.
- `CONFIG_EXAMPLES_TELNETD_NOMAC` – If the hardware has no MAC address of its
  own, define this `=y` to provide a bogus address for testing.
- `CONFIG_EXAMPLES_TELNETD_IPADDR` – The target IP address. Default `10.0.0.2`.
- `CONFIG_EXAMPLES_TELNETD_DRIPADDR` – The default router address. Default
  `10.0.0.1`.
- `CONFIG_EXAMPLES_TELNETD_NETMASK` – The network mask. Default:
  `255.255.255.0`.

Also, make sure that you have the following set in the NuttX configuration file
or else the performance will be very bad (because there will be only one
character per TCP transfer):

- `CONFIG_STDIO_BUFFER_SIZE` – Some value `>= 64`
- `CONFIG_STDIO_LINEBUFFER=y`

## `thttpd` THTTPD server

An example that builds `netutils/thttpd` with some simple NXFLAT CGI programs.
See `boards/README.txt` for most THTTPD settings. In addition to those, this
example accepts:

- `CONFIG_EXAMPLES_THTTPD_NOMAC`    – (May be defined to use software assigned
  MAC)
- `CONFIG_EXAMPLES_THTTPD_DRIPADDR` – Default router IP address.
- `CONFIG_EXAMPLES_THTTPD_NETMASK`  – Network mask.

Applications using this example will need to enable the following `netutils`
libraries in the `defconfig` file:

```conf
CONFIG_NETUTILS_NETLIB=y
CONFIG_NETUTILS_THTTPD=y
```

## `tiff`

This is a simple unit test for the TIFF creation library at `apps/graphic/tiff`.
It is configured to work in the Linux user-mode simulation and has not been
tested in any other environment.

At a minimum, to run in an embedded environment, you will probably have to
change the configured paths to the TIFF files defined in the example.

- `CONFIG_EXAMPLES_TIFF_OUTFILE` – Name of the resulting TIFF file. Default is
  `/tmp/result.tif`.
- `CONFIG_EXAMPLES_TIFF_TMPFILE1/2` – Names of two temporaries files that will
  be used in the file creation. Defaults are `/tmp/tmpfile1.dat` and
  `/tmp/tmpfile2.dat`.

The following must also be defined in your `apps/` configuration file:

```conf
CONFIG_EXAMPLES_TIFF=y
CONFIG_GRAPHICS_TIFF=y
```

## `timer`

This is a simple test of the timer driver (see `include/nuttx/timers/timer.h`).

Dependencies:

- `CONFIG_TIMER` – The timer driver must be selected

Example configuration:

- `CONFIG_EXAMPLES_TIMER_DEVNAME` – This is the name of the timer device that
  will be tested. Default: `/dev/timer0`.
- `CONFIG_EXAMPLES_TIMER_INTERVAL` – This is the timer interval in microseconds.
  Default: `1000000`.
- `CONFIG_EXAMPLES_TIMER_DELAY` – This is the delay between timer samples in
  microseconds. Default: `10000`.
- `CONFIG_EXAMPLES_TIMER_STACKSIZE` – This is the stack size allocated when the
  timer task runs. Default: `2048`.
- `CONFIG_EXAMPLES_TIMER_PRIORITY` – This is the priority of the timer task:
  Default: `100`.
- `CONFIG_EXAMPLES_TIMER_PROGNAME` – This is the name of the program that will
  be used when the NSH ELF program is installed. Default: `timer`.

## `touchscreen` Touchscreen Events

This configuration implements a simple touchscreen test at
`apps/examples/touchscreen`. This test will create an empty X11 window and will
print the touchscreen output as it is received from the simulated touchscreen
driver.

- `CONFIG_NSH_BUILTIN_APPS` – Build the touchscreen test as an NSH built-in
  function. Default: Built as a standalone program.
- `CONFIG_EXAMPLES_TOUCHSCREEN_MINOR` – The minor device number. Minor `N`
  corresponds to touchscreen device `/dev/inputN`. Note this value must with
  `CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH`. Default `0`.
- `CONFIG_EXAMPLES_TOUCHSCREEN_DEVPATH` – The path to the touchscreen device.
  This must be consistent with `CONFIG_EXAMPLES_TOUCHSCREEN_MINOR`. Default:
  `/dev/input0`.
- `CONFIG_EXAMPLES_TOUCHSCREEN_NSAMPLES` – This number of samples is collected
  and the program terminates. Default: Samples are collected indefinitely.
- `CONFIG_EXAMPLES_TOUCHSCREEN_MOUSE` – The touchscreen test can also be
  configured to work with a mouse driver by setting this option.

The following additional configurations must be set in the NuttX configuration
file:

- `CONFIG_INPUT=y` (plus any touchscreen-specific settings)

The following must also be defined in your apps configuration file:

- `CONFIG_EXAMPLES_TOUCHSREEN=y`

This example code will call `boardctl()` to setup the touchscreen driver for
texting. The implementation of `boardctl()` will require that board- specific
logic  provide the following interfaces that will be called by the `boardctl()`
in order to initialize the touchscreen hardware:

```c
int board_tsc_setup(int minor);
```

## `udp` Client/Server Over UDP

This is a simple network test for verifying client- and server- functionality
over UDP.

Applications using this example will need to enabled the following `netutils`
libraries in the `defconfig` file:

- `CONFIG_NETUTILS_NETLIB=y`

Possible configurations:

- Server on target hardware; client on host.
- Client on target hardware; Server on host.
- Server and Client on different targets.

## `udpblaster`

This is a simple network test for stressing UDP transfers. It simply sends UDP
packets from both the host and the target and the highest possible rate.

## `unionfs` Union File System

This is at trivial test of the Union File System. See
`nuttx/fs/unionfs/README.txt`. Dependencies:

- `CONFIG_DISABLE_MOUNTPOINT`          – Mountpoint support must not be
  disabled.
- `CONFIG_NFILE_DESCRIPTORS > 4`       – Some file descriptors must be
  allocated.
- `CONFIG_FS_ROMFS`                    – ROMFS support is required.
- `CONFIG_FS_UNIONFS`                  – Union File System support is required.

Configuration options. Use the defaults if you are unsure of what you are doing:

- `CONFIG_EXAMPLES_UNIONFS`            – Enables the example.
- `CONFIG_EXAMPLES_UNIONFS_MOUNTPT`    – Mountpoint path for the Union File
  System.
- `CONFIG_EXAMPLES_UNIONFS_TMPA`       – Temporary mount point for file system
  `1`.
- `CONFIG_EXAMPLES_UNIONFS_TMPB`       – Temporary mount point for file system
  `2`.
- `CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_A` – ROMFS file system `1` RAM disk device
  number.
- `CONFIG_EXAMPLES_UNIONFS_RAMDEVNO_B` – ROMFS file system `2` RAM disk device
  number.
- `CONFIG_EXAMPLES_UNIONFS_SECTORSIZE` – ROM disk sector size.

See the `README.txt` file at `nuttx/boards/sim/sim/sim/README.txt` for a
walk-through of the output of this text.

## `usbserial` USB Serial Hello World

### Target configuration

This is another implementation of _Hello, World_ but this one uses a USB serial
driver. Configuration options can be used to simply the test. These options
include:

-  `CONFIG_EXAMPLES_USBSERIAL_INONLY` – Only verify IN (device-to-host) data
   transfers. Default: both.
-  `CONFIG_EXAMPLES_USBSERIAL_OUTONLY` – Only verify OUT (host-to-device) data
   transfers. Default: both.
-  `CONFIG_EXAMPLES_USBSERIAL_ONLYSMALL` – Send only small, single packet
   messages. Default: Send large and small.
-  `CONFIG_EXAMPLES_USBSERIAL_ONLYBIG` – Send only large, multi-packet messages.
   Default: Send large and small.

If `CONFIG_USBDEV_TRACE` is enabled (or `CONFIG_DEBUG_FEATURES` and
`CONFIG_DEBUG_USB`), then the example code will also manage the USB trace
output. The amount of trace output can be controlled using:

- `CONFIG_EXAMPLES_USBSERIAL_TRACEINIT` – Show initialization events.
- `CONFIG_EXAMPLES_USBSERIAL_TRACECLASS` – Show class driver events.
- `CONFIG_EXAMPLES_USBSERIAL_TRACETRANSFERS` – Show data transfer events.
- `CONFIG_EXAMPLES_USBSERIAL_TRACECONTROLLER` – Show controller events.
- `CONFIG_EXAMPLES_USBSERIAL_TRACEINTERRUPTS` – Show interrupt-related events.

Error results are always shown in the trace output.

### Host-side test program

In additional to the target device-side example, there is also a host-side
application in this directory. This host side application must be executed on a
Linux host in order to perform the `USBSERIAL` test. The host application can be
compiled under Linux (or Cygwin?) as follows:

```bash
cd examples/usbserial
make -f Makefile.host TOPDIR=<nuttx-directory>
```

### Running the test

This will generate a small program called `host`. Usage:

1. Build the `examples/usbserial` target program and start the target.

2. Wait a bit, then do enter:

   ```shell
   dmesg
   ```

   At the end of the dmesg output, you should see the serial device was
   successfully idenfied and assigned to a tty device, probably `/dev/ttyUSB0`
   or `/dev/ttyACM0` (depending on the configured USB serial driver).

3. Then start the host application:

   ```bash
   ./host [<tty-dev>]
   ```

   Where:

   - `<tty-dev>` is the USB TTY device to use. The default is `/dev/ttyUSB0`
     (for the PL2303 emulation) or `/dev/ttyACM0` (for the CDC/ACM serial
     device).

The host and target will exchange are variety of very small and very large
serial messages.

## `userfs` UserFS File System

A simple test of the UserFS file system.

## `ustream` Unix Datagram Sockets

This is the same test as `examples/udp` and similar to `examples/ustream`, but
using Unix domain datagram sockets.

Dependencies:

- `CONFIG_NET_LOCAL` – Depends on support for Unix domain sockets.

Configuration:

- `CONFIG_EXAMPLES_UDGRAM` – Enables the Unix domain socket example.
- `CONFIG_EXAMPLES_UDGRAM_ADDR` – Specifics the Unix domain address. Default:
  `/dev/fifo`.

## `ustream` Unix Stream Sockets

This is the same test as `examples/udp` and similar to `examples/udgram`, but
using Unix domain stream sockets.

Dependencies:

- `CONFIG_NET_LOCAL` – Depends on support for Unix domain sockets.

Configuration:

- `CONFIG_EXAMPLES_USTREAM` – Enables the Unix domain socket example.
- `CONFIG_EXAMPLES_USTREAM_ADDR` – Specifics the Unix domain address. Default:
  `/dev/fifo`.

## `watchdog` Watchdog Timer

A simple test of a watchdog timer driver. Initializes starts the watchdog timer.
It pings the watchdog timer for a period of time then lets the watchdog timer
expire... resetting the CPU is successful. This example can ONLY be built as an
NSH built-in function.

This test depends on these specific Watchdog/NSH configurations settings (your
specific watchdog hardware settings might require additional settings).

- `CONFIG_WATCHDOG` – Enables watchdog timer support support.
- `CONFIG_NSH_BUILTIN_APPS` – Build the watchdog time test as an NSH built-in
  function.

Specific configuration options for this example include:

- `CONFIG_EXAMPLES_WATCHDOG_DEVPATH` – The path to the Watchdog device. Default:
  `/dev/watchdog0`.
- `CONFIG_EXAMPLES_WATCHDOG_PINGTIME` – Time in milliseconds that the example
  will ping the watchdog before letting the watchdog expire. Default: `5000`
  milliseconds.
- `CONFIG_EXAMPLES_WATCHDOG_PINGDELAY` – Time delay between pings in
  milliseconds. Default: `500` milliseconds.
- `CONFIG_EXAMPLES_WATCHDOG_TIMEOUT` – The watchdog timeout value in
  milliseconds before the watchdog timer expires. Default: `2000` milliseconds.

## `webserver` Simple Webserver

This is a port of uIP tiny webserver example application. Settings specific to
this example include:

- `CONFIG_EXAMPLES_WEBSERVER_NOMAC` (may be defined to use software assigned
  MAC)
- `CONFIG_EXAMPLES_WEBSERVER_IPADDR` – Target IP address.
- `CONFIG_EXAMPLES_WEBSERVER_DRIPADDR` – Default router IP address.
- `CONFIG_EXAMPLES_WEBSERVER_NETMASK` – Network mask.
- `CONFIG_EXAMPLES_WEBSERVER_DHCPC` – Select to get IP address via DHCP.

If you use DHCPC, then some special configuration network options are required.
These include:

- `CONFIG_NET=y` – of course.
- `CONFIG_NET_UDP=y` – UDP support is required for DHCP (as well as various
  other UDP-related configuration settings).
- `CONFIG_NET_BROADCAST=y` – UDP broadcast support is needed.
- `CONFIG_NET_ETH_PKTSIZE=650` or larger. Per RFC2131 (p. 9), the DHCP client
  must be prepared to receive DHCP messages of up to `576` bytes (excluding
  Ethernet, IP, or UDP headers and FCS). **Note** that the actual MTU setting
  will depend upon the specific link protocol. Here Ethernet is indicated.

Other configuration items apply also to the selected `webserver` net utility.
Additional relevant settings for the uIP `webserver` net utility are:

- `CONFIG_NETUTILS_HTTPDSTACKSIZE`
- `CONFIG_NETUTILS_HTTPDFILESTATS`
- `CONFIG_NETUTILS_HTTPDNETSTATS`

Applications using this example will need to enable the following `netutils`
libraries in their `defconfig` file:

```conf
CONFIG_NETUTILS_NETLIB=y
CONFIG_NETUTILS_DHCPC=y
CONFIG_NETDB_DNSCLIENT=y
CONFIG_NETUTILS_WEBSERVER=y
```

**Note**: This example does depend on the `perl` script at
`nuttx/tools/mkfsdata.pl`. You must have `perl` installed on your development
system at `/usr/bin/perl`.

## `wget` Web Client

A simple web client example. It will obtain a file from a server using the HTTP
protocol. Settings unique to this example include:

- `CONFIG_EXAMPLES_WGET_URL` – The URL of the file to get
- `CONFIG_EXAMPLES_WGET_NOMAC` – (May be defined to use software assigned MAC)
- `CONFIG_EXAMPLES_WGET_IPADDR` – Target IP address
- `CONFIG_EXAMPLES_WGET_DRIPADDR` – Default router IP address
- `CONFIG_EXAMPLES_WGET_NETMASK` – Network mask

This example uses `netutils/webclient`. Additional configuration settings apply
to that code as follows (but built-in defaults are probably OK):

- `CONFIG_WEBCLIENT_GETMIMETYPE`
- `CONFIG_WEBCLIENT_MAXHTTPLINE`
- `CONFIG_WEBCLIENT_MAXMIMESIZE`
- `CONFIG_WEBCLIENT_MAXHOSTNAME`
- `CONFIG_WEBCLIENT_MAXFILENAME`

Of course, the example also requires other settings including `CONFIG_NET` and
`CONFIG_NET_TCP`. The example also uses the uIP resolver which requires
`CONFIG_UDP`.

**Warning**: As of this writing, `wget` is untested on the target platform. At
present it has been tested only in the host-based configuration described in the
following note. The primary difference is that the target version will rely on
the also untested uIP name resolver.

**Note**: For test purposes, this example can be built as a host-based `wget`
function. This can be built as follows:

```bash
cd examples/wget
make -f Makefile.host
```

Applications using this example will need to enable the following `netutils`
libraries in the `defconfig` file:

```conf
CONFIG_NETUTILS_NETLIB=y
CONFIG_NETDB_DNSCLIENT=y
CONFIG_NETUTILS_WEBCLIENT=y
```

## `wgetjson` GET JSON Using `wget`

Uses `wget` to get a JSON encoded file, then decodes the file.

- `CONFIG_EXAMPLES_WDGETJSON_MAXSIZE` – Max. JSON Buffer Size.
- `CONFIG_EXAMPLES_EXAMPLES_WGETJSON_URL` – `wget` URL

## `xmlrpc` XML-RPC Server

This example exercises the _Embeddable Lightweight XML-RPC Server_ which is
discussed at:

http://www.drdobbs.com/web-development/an-embeddable-lightweight-xml-rpc-server/184405364

Configuration options:

- `CONFIG_EXAMPLES_XMLRPC_BUFFERSIZE` – HTTP buffer size. Default `1024`
- `CONFIG_EXAMPLES_XMLRPC_DHCPC` – Use DHCP Client. Default `n`. Ignored if
  `CONFIG_NSH_NETINIT` is selected.
- `CONFIG_EXAMPLES_XMLRPC_NOMAC` – Use Canned MAC Address. Default `n`. Ignored
  if `CONFIG_NSH_NETINIT` is selected.
- `CONFIG_EXAMPLES_XMLRPC_IPADDR` – Target IP address. Default `0x0a000002`.
  Ignored if `CONFIG_NSH_NETINIT` is selected.
- `CONFIG_EXAMPLES_XMLRPC_DRIPADDR` – Default Router IP address (Gateway).
  Default `0x0a000001`. Ignored if `CONFIG_NSH_NETINIT` is selected.
- `CONFIG_EXAMPLES_XMLRPC_NETMASK` – Network Mask. Default `0xffffff00`. Ignored
  if `CONFIG_NSH_NETINIT` is selected.

## `zerocross` Zero Crossing Device

A simple test of the Zero Crossing device driver.
