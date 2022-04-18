# `nshlib` NuttShell (NSH)

This directory contains the NuttShell (NSH) library. This library can be linked
with other logic to provide a simple shell application for NuttX.

- Console/NSH Front End
- Command Overview
- Conditional Command Execution
- Looping
- Built-In Variables
- Current Working Directory
  - Environment Variables
- NSH Start-Up Script
- Simple Commands
- Built-In Applications
- NSH Configuration Settings
  - Command Dependencies on Configuration Settings
  - Built-in Application Configuration Settings
  - NSH-Specific Configuration Settings
- Common Problems

## Console / NSH Front End

Using settings in the configuration file, NSH may be configured to use either
the serial `stdin`/`out` or a telnet connection as the console or BOTH. When NSH
is started, you will see the following welcome on either console:

```
NuttShell (NSH)
nsh>
```

`nsh>` is the NSH prompt and indicates that you may enter a command from the
console.

## Command Overview

This directory contains the NuttShell (NSH). This is a simple shell-like
application. At present, NSH supports the following commands forms:

- Simple command:

  ```
  <cmd>
  ```

- Command with re-directed output:

  ```
  <cmd> > <file>
  <cmd> >> <file>
  ```

- Background command:

  ```
  <cmd> &
  ```

- Re-directed background command:

  ```
  <cmd> > <file> &
  <cmd> >> <file> &
  ```

Where:

- `<cmd>` - is any one of the simple commands listed later.
- `<file>` -  is the full or relative path to any writeable object in the file
  system name space (file or character driver). Such objects will be referred to
  simply as files throughout this README.

NSH executes at the mid-priority (`128`). Backgrounded commands can be made to
execute at higher or lower priorities using nice:

```
[nice [-d <niceness>>]] <cmd> [> <file>|>> <file>] [&]
```

Where `<niceness>` is any value between `-20` and `19` where lower (more
negative values) correspond to higher priorities. The default niceness is `10`.

Multiple commands per line. NSH will accept multiple commands per command line
with each command separated with the semi-colon character (`;`).

If `CONFIG_NSH_CMDPARMS` is selected, then the output from commands, from file
applications, and from NSH built-in commands can be used as arguments to other
commands. The entity to be executed is identified by enclosing the command line
in back quotes. For example,

```shell
set FOO `myprogram $BAR`
```

Will execute the program named myprogram passing it the value of the environment
variable `BAR`. The value of the environment variable `FOO` is then set output
of myprogram on stdout. Because this feature commits significant resources, it
is disabled by default.

If `CONFIG_NSH_ARGCAT` is selected, the support concatenation of strings with
environment variables or command output. For example:

```shell
set FOO XYZ
set BAR 123
set FOOBAR ABC_${FOO}_${BAR}
```

would set the environment variable `FOO` to `XYZ`, `BAR` to `123` and `FOOBAR`
to `ABC_XYZ_123`. If `NSH_ARGCAT` is not selected, then a slightly small FLASH
footprint results but then also only simple environment variables like `$FOO`
can be used on the command line.

`CONFIG_NSH_QUOTE` enables back-slash quoting of certain characters within the
command. This option is useful for the case where an NSH script is used to
dynamically generate a new NSH script. In that case, commands must be treated as
simple text strings without interpretation of any special characters. Special
characters such as `$`, `` ` ``, `"`, and others must be retained intact as part of
the test string. This option is currently only available is `CONFIG_NSH_ARGCAT`
is also selected.

## Conditional Command Execution

An `if-then[-else]-fi` construct is also supported in order to support
conditional execution of commands. This works from the command line but is
primarily intended for use within NSH scripts (see the `sh` command). The syntax
is as follows:

```
if [!] <cmd>
then
  [sequence of <cmd>]
else
  [sequence of <cmd>]
fi
```

## Looping

`while-do-done` and `until-do-done` looping constructs are also supported. These
works from the command line but are primarily intended for use within NSH
scripts (see the `sh` command). The syntax is as follows:

```
while <test-cmd>; do <cmd-sequence>; done
```

(Execute `<cmd-sequence>` as long as `<test-cmd>` has an exit status of zero.)

```
until <test-cmd>; do <cmd-sequence>; done
```

(Execute `<cmd-sequence>` as long as `<test-cmd>` has a non-zero exit status.)

A break command is also supported. The break command is only meaningful within
the body of the a `while` or `until` loop, between the do and done tokens. If
the break command is executed within the body of a loop, the loop will
immediately terminate and execution will continue with the next command
immediately following the done token.

## Built-In Variables

- `$?` - The result of the last simple command execution.

## Current Working Directory

All path arguments to commands may be either an absolute path or a path relative
to the current working directory. The current working directory is set using the
`cd` command and can be queried either by using the `pwd` command or by using
the `echo $PWD` command.

### Environment Variables:

- `PWD`    - The current working directory
- `OLDPWD` - The previous working directory

## NSH System-init And Start-Up Script

NSH supports options to provide a system init script and start up script for NSH.
In general this capability is enabled with `CONFIG_NSH_ROMFSETC`, but has
several other related configuration options as described in the final section
of this README. This capability also depends on:

- `CONFIG_DISABLE_MOUNTPOINT` not set
- `CONFIG_FS_ROMFS`

### Default Script Behavior

The implementation that is provided is intended to provide great flexibility for
the use of script files, include system init file and start-up file. This
paragraph will discuss the general behavior when all of the configuration
options are set to the default values.

System-init script is executed before Start-up script. The system-init script
is mainly used for file system mounting and core system service startup, and the
start-up script is used for application and other system service startup. So,
Between them, some initialize can use filesystem and core system service,
Examples: Peripheral driver initialize at `boardctl(BOARDIOC_FINALINIT, 0)`.

In this default case, enabling `CONFIG_NSH_ROMFSETC` will cause NSH to behave as
follows at NSH startup time:

- NSH will create a read-only RAM disk (a ROM disk), containing a tiny ROMFS
  file system containing the following:

  ```
   |   `--init.d/
           `-- rcS
           `-- rc.sysinit
  ````

  Where `rcS` is the NSH start-up script
  Where `rc.sysinit` is the NSH system-init script

- NSH will then mount the ROMFS file system at `/etc`, resulting in:

  ```
   |--dev/
   |   `-- ram0
   `--etc/
       `--init.d/
           `-- rcS
           `-- rc.sysinit
  ```

- By default, the contents of `rc.sysinit` script are:

  ```shell
  # Create a RAMDISK and mount it at XXXRDMOUNTPOINTXXX

  mkrd -m 1 -s 512 1024
  mkfatfs /dev/ram1
  mount -t vfat /dev/ram1 /tmp
  ```

- NSH will execute the script at `/etc/init.d/rc.sysinit` at system init
  before the first NSH prompt. After execution of the script, the root
  FS will look like:

  ```
   |--dev/
   |   |-- ram0
   |   `-- ram1
   |--etc/
   |   `--init.d/
   |       `-- rcS
   |       `-- rc.sysinit
   `--tmp/
  ```

### Modifying the ROMFS Image

The contents of the `/etc` directory are retained in the file
`apps/nshlib/nsh_romfsimg.h` (OR, if `CONFIG_NSH_ARCHROMFS` is defined,
`include/arch/board/rcS.template`). In order to modify the start-up behavior,
there are three things to study:

1. Configuration Options.
   The additional `CONFIG_NSH_ROMFSETC` configuration options discussed in the
   final section of this README.

2. `tools/mkromfsimg.sh` Script.
   The script `tools/mkromfsimg.sh` creates `nsh_romfsimg.h`. It is not
   automatically executed. If you want to change the configuration settings
   associated with creating and mounting the `/tmp` directory, then it will be
   necessary to re-generate this header file using the `mkromfsimg.sh` script.

   The behavior of this script depends upon three things:

   - The configuration settings of the installed NuttX configuration.
   - The genromfs tool (available from http://romfs.sourceforge.net).
   - The file `apps/nshlib/rcS.template` (OR, if `CONFIG_NSH_ARCHROMFS` is
     defined, `include/arch/board/rcs.template`)

3. `rc.sysinit.template`. The file `apps/nshlib/rc.sysinit.template` contains
   the general form of the `rc.sysinit.template` file; configured values
   are plugged into this template file to produce the final `rc.sysinit` file.

   `rcS.template`. The file `apps/nshlib/rcS.template` contains the general form
   of the `rcS` file; configured values are plugged into this template file to
   produce the final `rcS` file.

**Note**: `apps/nshlib/rc.sysinit.template` and ` apps/nshlib/rcS.template`
generates the standard, default `nsh_romfsimg.h` file. If `CONFIG_NSH_ARCHROMFS`
is defined in the NuttX configuration file, then a custom, board-specific
`nsh_romfsimg.h` file residing in `boards/<arch>/<chip>/<board>/include` will be
used. **Note** when the OS is configured, `include/arch/board` will be linked to
`boards/<arch>/<chip>/<board>/include`.

All of the startup-behavior is contained in `rc.sysinit.template` and
`rcS.template`. The role of `mkromfsimg.sh` is to (1) apply the specific
configuration settings to `rc.sysinit.template` to create the final
`rc.sysinit.template`, and `rcS.template` to create the final `rcS` and
(2) to generate the header file `nsh_romfsimg.h` containing the ROMFS file
system image.

## Simple Commands

- `[ <expression> ]`
  `test <expression>`

   These are two alternative forms of the same command. They support evaluation
   of a boolean expression which sets `$?`. This command is used most frequently
   as the conditional command following the `if` in the `if-then[-else]-fi`
   construct.

   **Expression Syntax**:

   ```
   expression = simple-expression | !expression |
                expression -o expression | expression -a expression

   simple-expression = unary-expression | binary-expression

   unary-expression = string-unary | file-unary

   string-unary = -n string | -z string

   file-unary = -b file | -c file | -d file | -e file | -f file |
                -r file | -s file | -w file

   binary-expression = string-binary | numeric-binary

   string-binary = string = string | string == string | string != string

   numeric-binary = integer -eq integer | integer -ge integer |
                    integer -gt integer | integer -le integer |
                    integer -lt integer | integer -ne integer
   ```

- `addroute <target> [<netmask>] <router>`
  `addroute default <ipaddr> <interface>`

  This command adds an entry in the routing table. The new entry
  will map the IP address of a router on a local network(`<router>`)
  to an external network characterized by the `<target>` IP address and
  a network mask `<netmask>`

  The netmask may also be expressed using IPv4 CIDR or IPv6 slash
  notation. In that case, the netmask need not be provided.

  **Example**:

  ```
  nsh> addroute 11.0.0.0 255.255.255.0 10.0.0.2
  ```

  which is equivalent to

  ```
  nsh> addroute 11.0.0.0/24 10.0.0.2
  ```

  The second form of the `addroute` command can be used to set the default
  gateway.

- `arp [-t|-a <ipaddr>|-d <ipaddr>|-s <ipaddr> <hwaddr>]`

  Access the OS ARP table.

  - `-a <ipaddr>`
     Will show the hardware address that the IP address `<ipaddr>` is mapped to.

  - `-d <ipaddr>`
     Will delete the mapping for the IP address `<ipaddr>` from the ARP table.

  - `-s <ipaddr> <hwaddr>`
     Will set (or replace) the mapping of the IP address `<ipaddr>` to the
     hardware address `<hwaddr>`.

  - `-t`
     Will dump the entire content of the ARP table. This option is only
     available if `CONFIG_NETLINK_ROUTE` is enabled.

  **Example**:

  ```
  nsh> arp -a 10.0.0.1
  nsh: arp: no such ARP entry: 10.0.0.1

  nsh> arp -s 10.0.0.1 00:13:3b:12:73:e6
  nsh> arp -a 10.0.0.1
  HWAddr: 00:13:3b:12:73:e6

  nsh> arp -d 10.0.0.1
  nsh> arp -a 10.0.0.1
  nsh: arp: no such ARP entry: 10.0.0.1
  ```

- `base64dec [-w] [-f] <string or filepath>`

- `base64dec [-w] [-f] <string or filepath>`

- `basename <path> [<suffix>]`

  Extract the final string from a `<path>` by removing the preceding path
  segments and (optionally) removing any trailing `<suffix>`.

- `break`

  The `break` command is only meaningful within the body of the a `while` or
  `until` loop, between the do and done tokens. Outside of a loop, `break`
  command does nothing. If the break command is executed within the body of a
  loop, the loop will immediately terminate and execution will continue with the
  next command immediately following the done token.

- `cat <path> [<path> [<path> ...]]`

  This command copies and concatenates all of the files at `<path>` to the
  console (or to another file if the output is redirected).

- `cd [<dir-path>|-|~|..]`

  Changes the current working directory (`PWD`). Also sets the previous working
  directory environment variable (`OLDPWD`).

  **Forms**:

  - `cd <dir-path>` sets the current working directory to `<dir-path>`.
  - `cd -` sets the current working directory to the previous working directory
       (`$OLDPWD`). Equivalent to `cd $OLDPWD`.
  - `cd` or `cd ~` set the current working directory to the _home_ directory.
       The _home_ directory can be configured by setting `CONFIG_LIBC_HOMEDIR` in
       the configuration file. The default _home_ directory is `/`.
  - `cd ..` sets the current working directory to the parent directory.

- `cmp <path1> <path2>`

  Compare of the contents of the file at `<file1>` with the contents of the file
  at `<path2>`. Returns an indication only if the files differ.

- `cp <source-path> <dest-path>`

  Copy of the contents of the file at `<source-path>` to the location in the
  file system indicated by `<path-path>`

- `date [-s "MMM DD HH:MM:SS YYYY"] [-u]`

  Show or set the current date and time.

  Only one format is used both on display and when setting the date/time:
  `MMM DD HH:MM:SS YYYY`. For example,

  ```shell
  data -s "Sep 1 11:30:00 2011"
  ```

  24-hour time format is assumed.

- `dd if=<infile> of=<outfile> [bs=<sectsize>] [count=<sectors>] [skip=<sectors>]`

  Copy blocks from `<infile>` to `<outfile>`. `<nfile>` or `<outfile>` may be
  the path to a standard file, a character device, or a block device.

  **Examples**:

  1. Read from character device, write to regular file. This will create a new
     file of the specified size filled with zero.

     ```shell
     nsh> dd if=/dev/zero of=/tmp/zeros bs=64 count=16
     nsh> ls -l /tmp
     /tmp:
     -rw-rw-rw-    1024 ZEROS
     ```

  2. Read from character device, write to block device. This will fill the
     entire block device with zeros.

     ```shell
     nsh> ls -l /dev
     /dev:
      brw-rw-rw-       0 ram0
      crw-rw-rw-       0 zero
     nsh> dd if=/dev/zero of=/dev/ram0
     ```

  3. Read from a block device, write to a character device. This will read the
     entire block device and dump the contents in the bit bucket.

     ```shell
     nsh> ls -l /dev
     /dev:
      crw-rw-rw-       0 null
      brw-rw-rw-       0 ram0
     nsh> dd if=/dev/ram0 of=/dev/null
     ```

- `delroute <target> [<netmask>]`

  This command removes an entry from the routing table. The entry removed will
  be the first entry in the routing table that matches the external network
  characterized by the `<target>` IP address and the network mask `<netmask>`

  The netmask may also be expressed using IPv4 CIDR or IPv6 slash notation. In
  that case, the netmask need not be provided.

  **Example**:

  ```
  nsh> delroute 11.0.0.0 255.255.255.0
  ```

  which is equivalent to

  ```
  nsh> delroute 11.0.0.0/24
  ```

- `df`

  Show the state of each mounted volume.

  **Example**:

  ```
  nsh> mount
  /etc type romfs
  /tmp type vfat
  nsh> df
    Block  Number
    Size   Blocks     Used Available Mounted on
      64        6        6         0 /etc
     512      985        2       983 /tmp
  nsh>
  ```

- `dirname <path>`

  Extract the path string leading up to the full `<path>` by removing the final
  directory or file name.

- `dmesg`

  This command can be used to dump (and clear) the content of any buffered
  syslog output messages. This command is only available if
  `CONFIG_RAMLOG_SYSLOG` is enabled. In that case, syslog output will be
  collected in an in-memory, circular buffer. Entering the `dmesg` command will
  dump the content of that in-memory, circular buffer to the NSH console output.
  `dmesg` has the side effect of clearing the buffered data so that entering
  `dmesg` again will show only newly buffered data.

- `echo [-n] [<string|$name> [<string|$name>...]]`

  Copy the sequence of strings and expanded environment variables to console out
  (or to a file if the output is re-directed).

  The `-n` option will suppress the trailing newline character.

- `env`

  Show the current name-value pairs in the environment. **Example**:

  ```
  nsh> env
  PATH=/bin

  nsh> set foo bar
  nsh> env
  PATH=/bin
  foo=bar

  nsh> unset PATH
  nsh> env
  foo=bar

  nsh>
  ```

  **Note**: NSH variables are **not** shown by the `env` command.

- `exec <hex-address>`

  Execute the user logic at address `<hex-address>`. NSH will pause until the
  execution unless the user logic is executed in background via
  `exec <hex-address> &`

- `exit`

  Exit NSH. Only useful if you have started some other tasks (perhaps using the
  `exec` command) and you would like to have NSH out of the way.

- `export <name> [<value>]`

  The `export` command sets an environment variable, or promotes an NSH variable
  to an environment variable. As examples:

  1. Using `export` to promote an NSH variable to an environment variable.

     ```
     nsh> env
     PATH=/bin

     nsh> set foo bar
     nsh> env
     PATH=/bin

     nsh> export foo
     nsh> env
     PATH=/bin
     foo=bar
     ```

     A group-wide environment variable is created with the same value as the
     local NSH variable; the local NSH variable is removed.

     **Note**: This behavior differs from the Bash shell. Bash will retain the
     local Bash variable which will shadow the environment variable of the same
     name and same value.

  2. Using `export` to set an environment variable

     ```
     nsh> export dog poop
     nsh> env
     PATH=/bin
     foo=bar
     dog=poop
     ```

  The export command is not supported by NSH unless both `CONFIG_NSH_VARS=y` and
  `CONFIG_DISABLE_ENVIRON` is not set.

- `free`

  Show the current state of the memory allocator. For example,

  ```
  nsh> free
  free
               total       used       free    largest
  Mem:       4194288    1591552    2602736    2601584
  ```

  Where:

  - total - This is the total size of memory allocated for use
      by `malloc` in bytes.
  - used - This is the total size of memory occupied by
      chunks handed out by `malloc`.
  - free - This is the total size of memory occupied by
      free (not in use) chunks.
  - largest - Size of the largest free (not in use) chunk.

- `get [-b|-n] [-f <local-path>] -h <ip-address> <remote-path>`

  Use TFTP to copy the file at `<remote-address>` from the host whose IP
  address is identified by `<ip-address>`. Other options:

  - `-f <local-path>`
    The file will be saved relative to the current working directory unless
    `<local-path>` is provided.
  - `-b|-n`
    Selects either binary (_octet_) or text (_netascii_) transfer mode. Default:
    text.

- `help [-v] [<cmd>]`

  Presents summary information about NSH commands to console. Options:

  - `-v`
    Show verbose output will full command usage.

  - `<cmd>`
    Show full command usage only for this command.

- `hexdump <file or device>`

  Dump data in hexadecimal format from a file or character device.

- `ifconfig [nic_name [<ip-address>|dhcp]] [dr|gw|gateway <dr-address>] [netmask <net-mask>] [dns <dns-address>] [hw <hw-mac>]`

  Show the current configuration of the network, for example:

  ```
  nsh> ifconfig
  eth0    HWaddr 00:18:11:80:10:06
          IPaddr:10.0.0.2 DRaddr:10.0.0.1 Mask:255.255.255.0
  ```

  if networking statistics are enabled (`CONFIG_NET_STATISTICS`), then
  this command will also show the detailed state of transfers by protocol.

  **Note**: This commands depends upon having the `procfs` file system configured
  into the system. The `procfs` file system must also have been mounted
  with a command like:

  ```
  nsh> mount -t procfs /proc
  ```

- `ifdown <interface>`

  Take down the interface identified by the name `<interface>`.

  **Example**:

  ```
  ifdown eth0
  ```

- `ifup <interface>`

  Bring up down the interface identified by the name `<interface>`.

  **Example**:

  ```
  ifup eth0
  ```

- `insmod <file-path> <module-name>`

  Install the loadable OS module at `<file-path>` as module `<module-name>`

  **Example**:

  ```
  nsh> ls -l /mnt/romfs
  /mnt/romfs:
    dr-xr-xr-x       0 .
    -r-xr-xr-x    9153 chardev
  nsh> ls -l /dev
  /dev:
    crw-rw-rw-       0 console
    crw-rw-rw-       0 null
    brw-rw-rw-       0 ram0
    crw-rw-rw-       0 ttyS0
  nsh> insmod /mnt/romfs/chardev mydriver
  nsh> ls -l /dev
  /dev:
    crw-rw-rw-       0 chardev
    crw-rw-rw-       0 console
    crw-rw-rw-       0 null
    brw-rw-rw-       0 ram0
    crw-rw-rw-       0 ttyS0
  nsh> lsmod
  NAME                 INIT   UNINIT      ARG     TEXT     SIZE     DATA     SIZE
  mydriver         20404659 20404625        0 20404580      552 204047a8        0
  ```

- `irqinfo`

  Show the current count of interrupts taken on all attached interrupts.

  **Example**:

  ```
  nsh> irqinfo
   IRQ HANDLER  ARGUMENT      COUNT     RATE
     3 00001b3d 00000000        156   19.122
    15 0000800d 00000000        817  100.000
    30 00000fd5 20000018         20    2.490
  ```

- `kill -<signal> <pid>`

  Send the `<signal>` to the task identified by `<pid>`.

- `losetup [-d <dev-path>] | [[-o <offset>] [-r] <ldev-path> <file-path>]`

  Setup or teardown the loop device:

  1. Teardown the setup for the loop device at `<dev-path>`:

     ```shell
     losetup d <dev-path>
     ```

  2. Setup the loop device at `<dev-path>` to access the file at `<file-path>`
     as a block device:

     ```shell
     losetup [-o <offset>] [-r] <dev-path> <file-path>
     ```

  **Example**:

  ```
  nsh> dd if=/dev/zero of=/tmp/image bs=512 count=512
  nsh> ls -l /tmp
  /tmp:
    -rw-rw-rw-   262144 IMAGE
  nsh> losetup /dev/loop0 /tmp/image
  nsh> ls -l /dev
  /dev:
    brw-rw-rw-       0 loop0
  nsh> mkfatfs /dev/loop0
  nsh> mount -t vfat /dev/loop0 /mnt/example
  nsh> ls -l /mnt
  ls -l /mnt
  /mnt:
    drw-rw-rw-       0 example/
  nsh> echo "This is a test" >/mnt/example/atest.txt
  nsh> ls -l /mnt/example
  /mnt/example:
    -rw-rw-rw-      16 ATEST.TXT
  nsh> cat /mnt/example/atest.txt
  This is a test
  nsh>
  ```

- `ln [-s] <target> <link>`

  The link command will create a new symbolic link at `<link>` for the
  existing file or directory, `<target>`. This implementation is simplied
  for use with NuttX in these ways:

    - Links may be created only within the NuttX top-level, pseudo file
      system. No file system currently supported by NuttX provides
      symbolic links.
    - For the same reason, only soft links are implemented.
    - File privileges are ignored.
    - `c_time` is not updated.

- `ls [-lRs] <dir-path>`

  Show the contents of the directory at `<dir-path>`. **Note**:
  `<dir-path>` must refer to a directory and no other file system
  object.

  Options:

  - `-R` Show the constents of specified directory and all of its
    sub-directories.
  - `-s` Show the size of the files along with the filenames in the listing.
  - `-l` Show size and mode information along with the filenames in the listing.

- `lsmod`

  Show information about the currently installed OS modules. This information
  includes:

  - The module name assigned to the module when it was installed (`NAME`,
    string).
  - The address of the module initialization function (`INIT`, hexadecimal).
  - The address of the module un-initialization function (`UNINIT`,
    hexadecimal).
  - An argument that will be passed to the module un-initialization function
    (`ARG`, hexadecimal).
  - The start of the `.text` memory region (`TEXT`, hexadecimal).
  - The size of the `.text` memory region size (`SIZE`, decimal).
  - The start of the `.bss`/`.data` memory region (`DATA`, hexadecimal).
  - The size of the `.bss`/`.data` memory region size (`SIZE`, decimal).

  **Example**:

  ```
  nsh> lsmod
  NAME                 INIT   UNINIT      ARG     TEXT     SIZE     DATA     SIZE
  mydriver         20404659 20404625        0 20404580      552 204047a8        0
  ```

- `md5 [-f] <string or filepath>`

- `mb <hex-address>[=<hex-value>][ <hex-byte-count>]`
  `mh <hex-address>[=<hex-value>][ <hex-byte-count>]`
  `mw <hex-address>[=<hex-value>][ <hex-byte-count>]`

  Access memory using byte size access (`mb`), 16-bit accesses (`mh`),
  or 32-bit access (`mw`). In each case,

  - `<hex-address>`. Specifies the address to be accessed. The current value at
      that address will always be read and displayed.
  - `<hex-address>=<hex-value>`. Read the value, then write `<hex-value>` to the
      location.
  - `<hex-byte-count>`. Perform the `mb`, `mh`, or `mw` operation on a total of
      `<hex-byte-count>` bytes, increment the `<hex-address>` appropriately
      after each access

  **Example**:

  ```
  nsh> mh 0 16
    0 = 0x0c1e
    2 = 0x0100
    4 = 0x0c1e
    6 = 0x0110
    8 = 0x0c1e
    a = 0x0120
    c = 0x0c1e
    e = 0x0130
    10 = 0x0c1e
    12 = 0x0140
    14 = 0x0c1e
  nsh>
  ```

- `mkdir <path>`

  Create the directory at `<path>`. All components of of `<path>` except the
  final directory name must exist on a mounted file system; the final directory
  must not.

  Recall that NuttX uses a pseudo file system for its root file system. The
  `mkdir` command can only be used to create directories in volumes set up with
  the `mount` command; it cannot be used to create directories in the pseudo file
  system.

  **Example**:

  ```
  nsh> mkdir /mnt/fs/tmp
  nsh> ls -l /mnt/fs
  /mnt/fs:
    drw-rw-rw-       0 TESTDIR/
    drw-rw-rw-       0 TMP/
  nsh>
  ```

- `mkfatfs [-F <fatsize>] [-r <rootdirentries>] <block-driver>`

  Format a `fat` file system on the block device specified by `<block-driver>`
  path. The FAT size may be provided as an option. Without the `<fatsize>`
  option, mkfatfs will select either the FAT12 or FAT16 format. For historical
  reasons, if you want the FAT32 format, it must be explicitly specified on the
  command line.

  The `-r` option may be specified to select the the number of entries in the
  root directory. Typical values for small volumes would be `112` or `224`;
  `512` should be used for large volumes, such as hard disks or very large SD
  cards. The default is `512` entries in all cases.

  The reported number of root directory entries used with FAT32 is zero because
  the FAT32 root directory is a cluster chain.

  NSH provides this command to access the `mkfatfs()` NuttX API. This block
  device must reside in the NuttX pseudo file system and must have been created
  by some call to `register_blockdriver()` (see `include/nuttx/fs/fs.h`).

- `mkfifo <path>`

  Creates a FIFO character device anywhere in the pseudo file system, creating
  whatever pseudo directories that may be needed to complete the full path. By
  convention, however, device drivers are place in the standard `/dev`
  directory. After it is created, the FIFO device may be used as any other
  device driver. NSH provides this command to access the `mkfifo()` NuttX API.

  **Example**:

  ```
  nsh> ls -l /dev
  /dev:
    crw-rw-rw-       0 console
    crw-rw-rw-       0 null
    brw-rw-rw-       0 ram0
  nsh> mkfifo /dev/fifo
  nsh> ls -l /dev
  ls -l /dev
  /dev:
    crw-rw-rw-       0 console
    crw-rw-rw-       0 fifo
    crw-rw-rw-       0 null
    brw-rw-rw-       0 ram0
  nsh>
  ```

- `mkrd [-m <minor>] [-s <sector-size>] <nsectors>`

  Create a ramdisk consisting of `<nsectors>`, each of size `<sector-size>` (or
  `512` bytes if `<sector-size>` is not specified). The ramdisk will be
  registered as `/dev/ram<minor>`. If `<minor>` is not specified, mkrd will
  attempt to register the ramdisk as `/dev/ram0`.

  **Example**:

  ```
  nsh> ls /dev
  /dev:
    console
    null
    ttyS0
    ttyS1
  nsh> mkrd 1024
  nsh> ls /dev
  /dev:
    console
    null
    ram0
    ttyS0
    ttyS1
  nsh>
  ```

  Once the ramdisk has been created, it may be formatted using the `mkfatfs`
  command and mounted using the `mount` command.

  **Example**:

  ```
  nsh> mkrd 1024
  nsh> mkfatfs /dev/ram0
  nsh> mount -t vfat /dev/ram0 /tmp
  nsh> ls /tmp
  /tmp:
  nsh>
  ```

- `mount [-t <fstype> [-o <options>] <block-device> <dir-path>]`

  The `mount` command performs one of two different operations. If no parameters
  are provided on the command line after the `mount` command, then the `mount`
  command will enumerate all of the current mountpoints on the console.

  If the mount parameters are provided on the command after the `mount` command,
  then the `mount` command will mount a file system in the NuttX pseudo-file
  system. `mount` performs a three way association, binding:

  - File system. The `-t <fstype>` option identifies the type of file system
    that has been formatted on the `<block-device>`. As of this writing, `vfat`
    is the only supported value for `<fstype>`

  - Block Device. The `<block-device>` argument is the full or relative path to
    a block driver inode in the pseudo file system. By convention, this is a
    name under the `/dev` sub-directory. This `<block-device>` must have been
    previously formatted with the same file system type as specified by
    `<fstype>`

  - Mount Point. The mount point is the location in the pseudo file system where
    the mounted volume will appear. This mount point can only reside in the
    NuttX pseudo file system. By convention, this mount point is a subdirectory
    under `/mnt`. The `mount` command will create whatever pseudo directories
    that may be needed to complete the full path but the full path must not
    already exist.

  After the volume has been mounted in the NuttX pseudo file system, it may be
  access in the same way as other objects in the file system.

  **Examples**:

  ```
  nsh> ls -l /dev
  /dev:
    crw-rw-rw-       0 console
    crw-rw-rw-       0 null
    brw-rw-rw-       0 ram0
  nsh> ls /mnt
  nsh: ls: no such directory: /mnt
  nsh> mount -t vfat /dev/ram0 /mnt/fs
  nsh> ls -l /mnt/fs/testdir
  /mnt/fs/testdir:
    -rw-rw-rw-      15 TESTFILE.TXT
  nsh> echo "This is a test" >/mnt/fs/testdir/example.txt
  nsh> ls -l /mnt/fs/testdir
  /mnt/fs/testdir:
  -rw-rw-rw-      15 TESTFILE.TXT
    -rw-rw-rw-      16 EXAMPLE.TXT
  nsh> cat /mnt/fs/testdir/example.txt
  This is a test
  nsh>

  nsh> mount
    /etc type romfs
    /tmp type vfat
    /mnt/fs type vfat
  ```

- `mv <old-path> <new-path>`

  Rename the file object at `<old-path>` to `<new-path>`. Both paths must
  reside in the same mounted file system.

- `nfsmount <server-address> <mount-point> <remote-path>`

  Mount the remote NFS server directory `<remote-path>` at `<mount-point>` on
  the target machine. `<server-address>` is the IP address of the remote server.

- `nslookup <host-name>`

  Lookup and print the IP address associated with `<host-name>`

- `passwd <username> <password>`

  Set the password for the existing user `<username>` to `<password>`

- `pmconfig [stay|relax] [normal|idle|standby|sleep]`

  Control power management subsystem.

- `poweroff [<n>]`

  Shutdown and power off the system. This command depends on board- specific
  hardware support to power down the system. The optional, decimal numeric
  argument `<n>` may be included to provide power off mode to board-specific
  power off logic.

  **Note**: Supporting both the `poweroff` and `shutdown` commands is redundant.

- `printf [\xNN] [\n\r\t] [<string|$name> [<string|$name>...]]`

  Copy the sequence of strings, characters and expanded environment variables to
  console out (or to a file if the output is re-directed).

  No trailing newline character is added. The escape sequences `\n`, `\r` or
  `\t` can be used to add line feed, carriage return or tab character to output,
  respectively.

  The hexadecimal escape sequence `\xNN` takes up to two hexadesimal digits to
  specify the printed character.

- `ps`

  Show the currently active threads and tasks. For example:

  ```
  nsh> ps
  PID PRI POLICY   TYPE    NPX STATE    EVENT     SIGMASK  COMMAND
    0   0 FIFO     Kthread --- Ready              00000000 Idle Task
    1 128 RR       Task    --- Running            00000000 init
    2 128 FIFO     Task    --- Waiting  Semaphore 00000000 nsh_telnetmain()
    3 100 RR       pthread --- Waiting  Semaphore 00000000 <pthread>(21)
  nsh>
  ```

  **Note**: This commands depends upon having the `procfs` file system
  configured into the system. The procfs file system must also have been mounted
  with a command like:

  ```shell
  nsh> mount -t procfs /proc
  ```

- `put [-b|-n] [-f <remote-path>] -h <ip-address> <local-path>`

  Copy the file at <local-address> to the host whose IP address is
  identified by <ip-address>. Other options:

  - `-f <remote-path>`
    The file will be saved with the same name on the host unless
    unless `<local-path>` is provided.

  - `-b|-n`
     Selects either binary (_octet_) or test (_netascii_) transfer
     mode. Default: text.

- `pwd`

  Show the current working directory.

  ```
  nsh> cd /dev
  nsh> pwd
  /dev
  nsh>
  ```

  Same as `echo $PWD`

  ```
  nsh> echo $PWD
  /dev
  nsh>
  ```

- `readlink <link>`

  Show target of a soft link.

- `reboot [<n>]`

  Reset and reboot the system immediately. This command depends on hardware
  support to reset the system. The optional, decimal numeric argument `<n>` may
  be included to provide reboot mode to board-specific reboot logic.

  **Note**: Supporting both the `reboot` and `shutdown` commands is redundant.

- `rm <file-path>`

  Remove the specified `<file-path>` name from the mounted file system. Recall
  that NuttX uses a pseudo file system for its root file system. The `rm`
  command can only be used to remove (`unlink`) files in volumes set up with the
  `mount` command; it cannot be used to remove names from the pseudo file
  system.

  **Example**:

  ```
  nsh> ls /mnt/fs/testdir
  /mnt/fs/testdir:
    TESTFILE.TXT
    EXAMPLE.TXT
  nsh> rm /mnt/fs/testdir/example.txt
  nsh> ls /mnt/fs/testdir
  /mnt/fs/testdir:
    TESTFILE.TXT
  nsh>
  ```

- `rmdir <dir-path>`

  Remove the specified `<dir-path>` directory from the mounted file system.
  Recall that NuttX uses a pseudo file system for its root file system. The
  `rmdir` command can only be used to remove directories from volumes set up
  with the `mount` command; it cannot be used to remove directories from the
  pseudo file system.

  **Example**:

  ```
  nsh> mkdir /mnt/fs/tmp
  nsh> ls -l /mnt/fs
  /mnt/fs:
    drw-rw-rw-       0 TESTDIR/
    drw-rw-rw-       0 TMP/
  nsh> rmdir /mnt/fs/tmp
  nsh> ls -l /mnt/fs
  ls -l /mnt/fs
  /mnt/fs:
    drw-rw-rw-       0 TESTDIR/
  nsh>
  ```

- `rmmod <module-name>`

  Remove the loadable OS module with the `<module-name>`. **Note**: An OS module
  can only be removed if it is not busy.

  **Example**:

  ```
  nsh> lsmod
  NAME                 INIT   UNINIT      ARG     TEXT     SIZE     DATA     SIZE
  mydriver         20404659 20404625        0 20404580      552 204047a8        0
  nsh> rmmod mydriver
  nsh> lsmod
  NAME                 INIT   UNINIT      ARG     TEXT     SIZE     DATA     SIZE
  nsh>
  ```

- `route ipv4|ipv6`

  Show the contents of routing table for IPv4 or IPv6.

  If only IPv4 or IPv6 is enabled, then the argument is optional but, if
  provided, must match the enabled internet protocol version.

- `rptun start|stop <dev-path>`

  Start or stop the OpenAMP RPC tunnel device at `<dev-path>`.

- `set [{+|-}{e|x|xe|ex}] [<name> <value>]`

  Set the variable `<name>` to the string `<value>` and or set NSH parser
  control options.

  For example, a variable may be set like this:

  ```
  nsh> echo $foobar

  nsh> set foobar foovalue
  nsh> echo $foobar
  foovalue
  nsh>
  ```

  If `CONFIG_NSH_VARS` is selected, the effect of this `set` command is to set
  the local NSH variable. Otherwise, the group-wide environment variable will be
  set.

  If the local NSH variable has already been 'promoted' to an environment
  variable, then the `set` command will set the value of the environment
  variable rather than the local NSH variable.

  **Note**: The Bash shell does not work this way. Bash would set the value of
  both the local Bash variable and of the environment variable of the same name
  to the same value.

  If `CONFIG_NSH_VARS` is selected and no arguments are provided, then the `set`
  command will list all list all NSH variables.

  ```
  nsh> set
  foolbar=foovalue
  ```

  Set the _exit on error control_ and/or _print a trace_ of commands when
  parsing scripts in NSH. The settinngs are in effect from the point of
  execution, until they are changed again, or in the case of the init script,
  the settings are returned to the default settings when it exits. Included
  child scripts will run with the parents settings and changes made in the child
  script will effect the parent on return.

  Use `set -e` to enable and `set +e` to disable (ignore) the exit condition
  on commands. The default is `-e`. Errors cause script to exit.

  Use `set -x` to enable and `set +x` to disable (silence) printing a trace of
  the script commands as they are ececuted. The default is `+x`. No printing of
  a trace of script commands as they are executed.

  - Example 1 - no exit on command not found

    ```
    set +e
    notacommand
    ```

  - Example 2 - will exit on command not found

    ```
    set -e
    notacommand
    ```

  - Example 3 - will exit on command not found, and print a trace of the script commands

    ```
    set -ex
    ```

  - Example 4 - will exit on command not found, and print a trace of the script commands
              and set foobar to foovalue.

    ```
    set -ex foobar foovalue
    nsh> echo $foobar
    foovalue
    ```

- `shutdown [--reboot]`

  Shutdown and power off the system or, optionally, reset and reboot the system
  immediately. This command depends on hardware support to power down or reset
  the system; one, both, or neither behavior may be supported.

  **Note**: The `shutdown` command duplicates the behavior of the `poweroff` and
  `reboot` commands.

- `sleep <sec>`

  Pause execution (sleep) of `<sec>` seconds.

- `source <script-path>`

  Execute the sequence of NSH commands in the file referred to by
  `<script-path>`.

- `telnetd`

  The Telnet daemon may be started either programmatically by calling
  `nsh_telnetstart()` or it may be started from the NSH command line using this
  `telnetd` command.

  Normally this command would be suppressed with `CONFIG_NSH_DISABLE_TELNETD`
  because the Telnet daemon is automatically started in `nsh_main.c`. The
  exception is when `CONFIG_NETINIT_NETLOCAL` is selected. IN that case, the
  network is not enabled at initialization but rather must be enabled from the
  NSH command line or via other applications.

  In that case, calling `nsh_telnetstart()` before the the network is
  initialized will fail.

- `time "<command>"`

  Perform command timing. This command will execute the following `<command>`
  string and then show how much time was required to execute the command. Time
  is shown with a resolution of 100 microseconds which may be beyond the
  resolution of many configurations. Note that the `<command>` must be enclosed
  in quotation marks if it contains spaces or other delimiters.

  **Example**:

  ```
  nsh> time "sleep 2"

  2.0100 sec
  nsh>
  ```

  The additional 10 milliseconds in this example is due to the way that the
  `sleep` command works: It always waits one system clock tick longer than
  requested and this test setup used a 10 millisecond periodic system timer.
  Sources of error could include various quantization errors, competing CPU
  usage,  and the additional overhead of the `time` command execution itself
  which is included in the total.

  The reported time is the elapsed time from starting of the command to
  completion of the command. This elapsed time may not necessarily be just the
  processing time for the command. It may included interrupt level processing,
  for example. In a busy system, command processing could be delayed if
  pre-empted by other, higher priority threads competing for CPU time. So the
  reported time includes all CPU processing from the start of the command to its
  finish possibly including unrelated processing time during that interval.

  Notice that:

  ```
  nsh> time "sleep 2 &"
  sleep [3:100]

  0.0000 sec
  nsh>
  ```

  Since the `sleep` command is executed in background, the `sleep` command
  completes almost immediately. As opposed to the following where the `time`
  command is run in background with the `sleep` command:

  ```
  nsh> time "sleep 2" &
  time [3:100]
  nsh>
  2.0100 sec
  ```

- `truncate -s <length> <file-path>`

  Shrink or extend the size of the regular file at `<file-path>` to the
  specified `<length>`.

  A `<file-path>` argument that does not exist is created. The `<length>` option
  is NOT optional.

  If a `<file-path>` is larger than the specified size, the extra data is lost.
  If a `<file-path>` is shorter, it is extended and the extended part reads as
  zero bytes.

- `umount <dir-path>`

  Un-mount the file system at mount point `<dir-path>`. The `umount` command can
  only be used to un-mount volumes previously mounted using `mount` command.

  **Example**:

  ```
  nsh> ls /mnt/fs
  /mnt/fs:
    TESTDIR/
  nsh> umount /mnt/fs
  nsh> ls /mnt/fs
  /mnt/fs:
  nsh: ls: no such directory: /mnt/fs
  nsh>
  ```

- `unset <name>`

  Remove the value associated with the variable `<name>`. This will remove the
  name-value pair from both the NSH local variables and the group-wide
  environment variables. For example:

  ```
  nsh> echo $foobar
  foovalue
  nsh> unset foobar
  nsh> echo $foobar

  nsh>
  ```

- `urldecode [-f] <string or filepath>`

- `urlencode [-f] <string or filepath>`

- `uname [-a | -imnoprsv]`

  Print certain system information. With no options, the output is the same as
  `-s`.

  - `-a` Print all information, in the following order, except omit `-p` and
    `-i` if unknown:
    - `-s`, `-o`, Print the operating system name (NuttX)
    - `-n` Print the network node hostname (only available if `CONFIG_NET=y`)
    - `-r` Print the kernel release
    - `-v` Print the kernel version
    - `-m` Print the machine hardware name
    - `-i` Print the machine platform name
    - `-p` Print "unknown"

- `useradd <username> <password>`

  Add a new user with `<username>` and `<password>`

- `userdel <username>`

  Delete the user with the name `<username>`

- `usleep <usec>`

  Pause execution (sleep) of `<usec>` microseconds.

- `wget [-o <local-path>] <url>`

  Use HTTP to copy the file at `<url>` to the current directory. Options:

  - `-o <local-path>` The file will be saved relative to the current working
    directory and with the same name as on the HTTP server unless `<local-path>`
    is provided.

- `xd <hex-address> <byte-count>`

  Dump <byte-count> bytes of data from address `<hex-address>`

  **Example**:

  ```
  nsh> xd 410e0 512
  Hex dump:
  0000: 00 00 00 00 9c 9d 03 00 00 00 00 01 11 01 10 06 ................
  0010: 12 01 11 01 25 08 13 0b 03 08 1b 08 00 00 02 24 ....%..........$
  ...
  01f0: 08 3a 0b 3b 0b 49 13 00 00 04 13 01 01 13 03 08 .:.;.I..........
  nsh>
  ```

## Built-In Commands

In addition to the commands that are part of NSH listed above, there can be
additional, external _built-in_ applications that can be added to NSH. These are
separately excecuble programs but will appear much like the commands that are a
part of NSH. The primary difference from the user's perspective is that help
information about the built-in applications is not directly available from NSH.
Rather, you will need to execute the application with the -h option to get help
about using the built-in applications.

There are several built-in appliations in the `apps/` repository. No attempt is
made here to enumerate all of them. But a few of the more common built- in
applications are listed below.

- `ping [-c <count>] [-i <interval>] <ip-address>`
  `ping6 [-c <count>] [-i <interval>] <ip-address>`

  Test the network communication with a remote peer. Example:

  ```
  nsh> 10.0.0.1
  PING 10.0.0.1 56 bytes of data
  56 bytes from 10.0.0.1: icmp_seq=1 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=2 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=3 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=4 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=5 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=6 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=7 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=8 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=9 time=0 ms
  56 bytes from 10.0.0.1: icmp_seq=10 time=0 ms
  10 packets transmitted, 10 received, 0% packet loss, time 10190 ms
  nsh>
  ```

  `ping6` differs from `ping` in that it uses IPv6 addressing.

## NSH Configuration Settings

The availability of the above commands depends upon features that may or may not
be enabled in the NuttX configuration file. The following table indicates the
dependency of each command on NuttX configuration settings. General
configuration settings are discussed in the NuttX Porting Guide. Configuration
settings specific to NSH as discussed at the bottom of this README file.

## Command Dependencies on Configuration Settings

 Command  | Depends on Configuration
----------|--------------------------
[         | !`CONFIG_NSH_DISABLESCRIPT`
addroute  | `CONFIG_NET` && `CONFIG_NET_ROUTE`
arp       | `CONFIG_NET` && `CONFIG_NET_ARP`
base64dec | `CONFIG_NETUTILS_CODECS` && `CONFIG_CODECS_BASE64`
base64enc | `CONFIG_NETUTILS_CODECS` && `CONFIG_CODECS_BASE64`
basename  | -
break     | !`CONFIG_NSH_DISABLESCRIPT` && !`CONFIG_NSH_DISABLE_LOOPS`
cat       | -
cd        | !`CONFIG_DISABLE_ENVIRON`
cp        | -
dd        | -
delroute  | `CONFIG_NET` && `CONFIG_NET_ROUTE`
df        | !`CONFIG_DISABLE_MOUNTPOINT`
dirname   | -
dmesg     | `CONFIG_RAMLOG_SYSLOG`
echo      | -
env       | `CONFIG_FS_PROCFS` && !`CONFIG_DISABLE_ENVIRON` && !`CONFIG_PROCFS_EXCLUDE_ENVIRON`
exec      | -
exit      | -
export    | `CONFIG_NSH_VARS` && !`CONFIG_DISABLE_ENVIRON`
free      | -
get       | `CONFIG_NET` && `CONFIG_NET_UDP` && MTU >= 558  (see note 1)
help      | -
hexdump   | -
ifconfig  | `CONFIG_NET` && `CONFIG_FS_PROCFS` && !`CONFIG_FS_PROCFS_EXCLUDE_NET`
ifdown    | `CONFIG_NET` && `CONFIG_FS_PROCFS` && !`CONFIG_FS_PROCFS_EXCLUDE_NET`
ifup      | `CONFIG_NET` && `CONFIG_FS_PROCFS` && !`CONFIG_FS_PROCFS_EXCLUDE_NET`
insmod    | `CONFIG_MODULE`
irqinfo   | `CONFIG_FS_PROCFS` && `CONFIG_SCHED_IRQMONITOR`
kill      | -
losetup   | !`CONFIG_DISABLE_MOUNTPOINT` && `CONFIG_DEV_LOOP`
ln        | `CONFIG_PSEUDOFS_SOFTLINK`
ls        | -
lsmod     | `CONFIG_MODULE` && `CONFIG_FS_PROCFS` && !`CONFIG_FS_PROCFS_EXCLUDE_MODULE`
md5       | `CONFIG_NETUTILS_CODECS` && `CONFIG_CODECS_HASH_MD5`
mb,mh,mw  | -
mkdir     | !`CONFIG_DISABLE_MOUNTPOINT` || !`CONFIG_DISABLE_PSEUDOFS_OPERATIONS`
mkfatfs   | !`CONFIG_DISABLE_MOUNTPOINT` && `CONFIG_FSUTILS_MKFATFS`
mkfifo    | `CONFIG_PIPES` && `CONFIG_DEV_FIFO_SIZE` > 0
mkrd      | !`CONFIG_DISABLE_MOUNTPOINT`
mount     | !`CONFIG_DISABLE_MOUNTPOINT`
mv        | !`CONFIG_DISABLE_MOUNTPOINT` || !`CONFIG_DISABLE_PSEUDOFS_OPERATIONS`
nfsmount  | !`CONFIG_DISABLE_MOUNTPOINT` && `CONFIG_NET` && `CONFIG_NFS`
nslookup  | `CONFIG_LIBC_NETDB` && `CONFIG_NETDB_DNSCLIENT`
password  | !`CONFIG_DISABLE_MOUNTPOINT` && `CONFIG_NSH_LOGIN_PASSWD`
pmconfig  | `CONFIG_PM` && !`CONFIG_NSH_DISABLE_PMCONFIG`
poweroff  | `CONFIG_BOARDCTL_POWEROFF`
printf    | -
ps        | `CONFIG_FS_PROCFS` && !`CONFIG_FS_PROCFS_EXCLUDE_PROC`
put       | `CONFIG_NET` && `CONFIG_NET_UDP` && MTU >= 558 (see note 1,2)
pwd       | !`CONFIG_DISABLE_ENVIRON`
readlink  | `CONFIG_PSEUDOFS_SOFTLINK`
reboot    | `CONFIG_BOARDCTL_RESET`
rm        | !`CONFIG_DISABLE_MOUNTPOINT` || !`CONFIG_DISABLE_PSEUDOFS_OPERATIONS`
rmdir     | !`CONFIG_DISABLE_MOUNTPOINT` || !`CONFIG_DISABLE_PSEUDOFS_OPERATIONS`
rmmod     | `CONFIG_MODULE`
route     | `CONFIG_FS_PROCFS` && `CONFIG_FS_PROCFS_EXCLUDE_NET` && <br> !`CONFIG_FS_PROCFS_EXCLUDE_ROUTE` && `CONFIG_NET_ROUTE` && <br> !`CONFIG_NSH_DISABLE_ROUTE` && (`CONFIG_NET_IPv4` || `CONFIG_NET_IPv6`)
rptun     | `CONFIG_RPTUN`
set       | `CONFIG_NSH_VARS` || !`CONFIG_DISABLE_ENVIRON`
shutdown  | `CONFIG_BOARDCTL_POWEROFF` || `CONFIG_BOARDCTL_RESET`
sleep     | -
source    | `CONFIG_FILE_STREAM` && !`CONFIG_NSH_DISABLESCRIPT`
test      | !`CONFIG_NSH_DISABLESCRIPT`
telnetd   | `CONFIG_NSH_TELNET` && !`CONFIG_NSH_DISABLE_TELNETD`
time      | -
truncate  | !`CONFIG_DISABLE_MOUNTPOINT`
umount    | !`CONFIG_DISABLE_MOUNTPOINT`
uname     | !`CONFIG_NSH_DISABLE_UNAME`
unset     | `CONFIG_NSH_VARS` || !`CONFIG_DISABLE_ENVIRON`
urldecode | `CONFIG_NETUTILS_CODECS` && `CONFIG_CODECS_URLCODE`
urlencode | `CONFIG_NETUTILS_CODECS` && `CONFIG_CODECS_URLCODE`
useradd   | !`CONFIG_DISABLE_MOUNTPOINT` && `CONFIG_NSH_LOGIN_PASSWD`
userdel   | !`CONFIG_DISABLE_MOUNTPOINT` && `CONFIG_NSH_LOGIN_PASSWD`
usleep    | -
get       | `CONFIG_NET` && `CONFIG_NET_TCP`
xd        | -

**Notes**:

1. Because of hardware padding, the actual MTU required for `put` and `get`
   operations size may be larger.
2. Special TFTP server start-up options will probably be required to permit
   creation of file for the correct operation of the `put` command.

In addition, each NSH command can be individually disabled via one of the
following settings. All of these settings make the configuration of NSH
potentially complex but also allow it to squeeze into very small memory
footprints.

```
CONFIG_NSH_DISABLE_ADDROUTE,  CONFIG_NSH_DISABLE_BASE64DEC, CONFIG_NSH_DISABLE_BASE64ENC,
CONFIG_NSH_DISABLE_BASENAME,  CONFIG_NSH_DISABLE_CAT,       CONFIG_NSH_DISABLE_CD,
CONFIG_NSH_DISABLE_CP,        CONFIG_NSH_DISABLE_DD,        CONFIG_NSH_DISABLE_DELROUTE,
CONFIG_NSH_DISABLE_DF,        CONFIG_NSH_DISABLE_DIRNAME,   CONFIG_NSH_DISABLE_DMESG,
CONFIG_NSH_DISABLE_ECHO,      CONFIG_NSH_DISABLE_ENV,       CONFIG_NSH_DISABLE_EXEC,
CONFIG_NSH_DISABLE_EXIT,      CONFIG_NSH_DISABLE_EXPORT,    CONFIG_NSH_DISABLE_FREE,
CONFIG_NSH_DISABLE_GET,       CONFIG_NSH_DISABLE_HELP,      CONFIG_NSH_DISABLE_HEXDUMP,
CONFIG_NSH_DISABLE_IFCONFIG,  CONFIG_NSH_DISABLE_IFUPDOWN,  CONFIG_NSH_DISABLE_KILL,
CONFIG_NSH_DISABLE_LOSETUP,   CONFIG_NSH_DISABLE_LN,        CONFIG_NSH_DISABLE_LS,
CONFIG_NSH_DISABLE_MD5,       CONFIG_NSH_DISABLE_MB,        CONFIG_NSH_DISABLE_MKDIR,
CONFIG_NSH_DISABLE_MKFATFS,   CONFIG_NSH_DISABLE_MKFIFO,    CONFIG_NSH_DISABLE_MKRD,
CONFIG_NSH_DISABLE_MH,        CONFIG_NSH_DISABLE_MODCMDS,   CONFIG_NSH_DISABLE_MOUNT,
CONFIG_NSH_DISABLE_MW,        CONFIG_NSH_DISABLE_MV,        CONFIG_NSH_DISABLE_NFSMOUNT,
CONFIG_NSH_DISABLE_NSLOOKUP,  CONFIG_NSH_DISABLE_PASSWD,    CONFIG_NSH_DISABLE_PING6,
CONFIG_NSH_DISABLE_POWEROFF,  CONFIG_NSH_DISABLE_PRINTF,    CONFIG_NSH_DISABLE_PS,
CONFIG_NSH_DISABLE_PUT,       CONFIG_NSH_DISABLE_PWD,       CONFIG_NSH_DISABLE_READLINK,
CONFIG_NSH_DISABLE_REBOOT,    CONFIG_NSH_DISABLE_RM,        CONFIG_NSH_DISABLE_RPTUN,
CONFIG_NSH_DISABLE_RMDIR,     CONFIG_NSH_DISABLE_ROUTE,     CONFIG_NSH_DISABLE_SET,
CONFIG_NSH_DISABLE_SHUTDOWN,  CONFIG_NSH_DISABLE_SLEEP,     CONFIG_NSH_DISABLE_SOURCE,
CONFIG_NSH_DISABLE_TEST,      CONFIG_NSH_DISABLE_TIME,      CONFIG_NSH_DISABLE_TRUNCATE,
CONFIG_NSH_DISABLE_UMOUNT,    CONFIG_NSH_DISABLE_UNSET,     CONFIG_NSH_DISABLE_URLDECODE,
CONFIG_NSH_DISABLE_URLENCODE, CONFIG_NSH_DISABLE_USERADD,   CONFIG_NSH_DISABLE_USERDEL,
CONFIG_NSH_DISABLE_USLEEP,    CONFIG_NSH_DISABLE_WGET,      CONFIG_NSH_DISABLE_XD
```

Verbose help output can be suppressed by defining `CONFIG_NSH_HELP_TERSE`. In
that case, the `help` command is still available but will be slightly smaller.

## Built-in Application Configuration Settings

All built-in applications require that support for NSH built-in applications has
been enabled. This support is enabled with `CONFIG_BUILTIN=y` and
`CONFIG_NSH_BUILTIN_APPS=y`.

Application | Depends on Configuration
------------|--------------------------
ping        | `CONFIG_NET` && `CONFIG_NET_ICMP` && `CONFIG_NET_ICMP_SOCKET` && <br> `CONFIG_SYSTEM_PING`
ping6       | `CONFIG_NET` && `CONFIG_NET_ICMPv6` && `CONFIG_NET_ICMPv6_SOCKET` && <br> `CONFIG_SYSTEM_PING6`

## NSH-Specific Configuration Settings

The behavior of NSH can be modified with the following settings in the
`boards/<arch>/<chip>/<board>/configs/<config>/defconfig` file:

- `CONFIG_NSH_READLINE` – Selects the minimal implementation of `readline()`.
  This minimal implementation provides on backspace for command line editing.

- `CONFIG_NSH_CLE`

  Selects the more extensive, EMACS-like command line editor. Select this option
  only if (1) you don't mind a modest increase in the FLASH footprint, and (2)
  you work with a terminal that support VT100 editing commands.

  Selecting this option will add probably 1.5-2KB to the FLASH footprint.

- `CONFIG_NSH_BUILTIN_APPS` – Support external registered, _builtin_
  applications that can be executed from the NSH command line (see
  `apps/README.md` for more information).

- `CONFIG_NSH_FILEIOSIZE` – Size of a static I/O buffer used for file access
  (ignored if there is no file system). Default is `1024`.

- `CONFIG_NSH_STRERROR` – `strerror(errno)` makes more readable output but
  `strerror()` is very large and will not be used unless this setting is `y`.
  This setting depends upon the `strerror()` having been enabled with
  `CONFIG_LIBC_STRERROR`.

- `CONFIG_NSH_LINELEN` – The maximum length of one command line and of one
  output line. Default: `80`

- `CONFIG_NSH_DISABLE_SEMICOLON` – By default, you can enter multiple NSH
  commands on a line with each command separated by a semicolon. You can disable
  this feature to save a little memory on FLASH challenged platforms. Default:
  `n`

- `CONFIG_NSH_CMDPARMS`

  If selected, then the output from commands, from file applications, and from
  NSH built-in commands can be used as arguments to other commands. The entity
  to be executed is identified by enclosing the command line in back quotes. For
  example,

  ```shell
  set FOO `myprogram $BAR`
  ```

  Will execute the program named myprogram passing it the value of the
  environment variable `BAR`. The value of the environment variable FOO is then
  set output of myprogram on stdout. Because this feature commits significant
  resources, it is disabled by default.

  The `CONFIG_NSH_CMDPARMS` interim output will be retained in a temporary file.
  Full path to a directory where temporary files can be created is taken from
  `CONFIG_LIBC_TMPDIR` and it defaults to `/tmp` if `CONFIG_LIBC_TMPDIR` is not
  set.

- `CONFIG_NSH_MAXARGUMENTS` – The maximum number of NSH command arguments.
  Default: `6`

- `CONFIG_NSH_ARGCAT`

  Support concatenation of strings with environment variables or command
  output. For example:

  ```shell
  set FOO XYZ
  set BAR 123
  set FOOBAR ABC_${FOO}_${BAR}
  ```

  would set the environment variable `FOO` to `XYZ`, `BAR` to `123` and `FOOBAR`
  to `ABC_XYZ_123`. If `NSH_ARGCAT` is not selected, then a slightly small FLASH
  footprint results but then also only simple environment variables like `$FOO`
  can be used on the command line.

- `CONFIG_NSH_VARS`

  By default, there are no internal NSH variables. NSH will use OS environment
  variables for all variable storage. If this option, NSH will also support
  local NSH variables. These variables are, for the most part, transparent and
  work just like the OS environment variables. The difference is that when you
  create new tasks, all of environment variables are inherited by the created
  tasks. NSH local variables are not.

  If this option is enabled (and `CONFIG_DISABLE_ENVIRON` is not), then a new
  command called 'export' is enabled. The export command works very must like
  the set command except that is operates on environment variables. When
  `CONFIG_NSH_VARS` is enabled, there are changes in the behavior of certain
  commands

   Command         | w/o `CONFIG_NSH_VARS`            | w/`CONFIG_NSH_VARS`
  -----------------|----------------------------------|---------------------
  `set <a> <b>`    | Set environment var `a` to `b`.  | Set NSH var `a` to `b`.
  `set`            | Causes an error.                 | Lists all NSH variables.
  `unset <a>`      | Unsets environment var `a`.      | Unsets both environment var and NSH var `a`.
  `export <a> <b>` | Causes an error.                 | Unsets NSH var `a`. Sets environment var `a` to `b`.
  `export <a>`     | Causes an error.                 | Sets environment var `a` to NSH var `b` (or `""`). <br> Unsets local var `a`.
  `env`            | Lists all environment variables. | Lists all environment variables (only).

- `CONFIG_NSH_QUOTE` – Enables back-slash quoting of certain characters within
  the command. This option is useful for the case where an NSH script is used to
  dynamically generate a new NSH script. In that case, commands must be treated
  as simple text strings without interpretation of any special characters.
  Special characters such as `$`, `` ` ``, `"`, and others must be retained
  intact as part of the test string. This option is currently only available is
  `CONFIG_NSH_ARGCAT` is also selected.

- `CONFIG_NSH_NESTDEPTH` – The maximum number of nested `if-then[-else]-fi`
  sequences that are permissible. Default: `3`

- `CONFIG_NSH_DISABLESCRIPT` – This can be set to `y` to suppress support for
  scripting. This setting disables the `sh`, `test`, and `[` commands and the
  `if-then[-else]-fi` construct. This would only be set on systems where a
  minimal footprint is a necessity and scripting is not.

- `CONFIG_NSH_DISABLE_ITEF` – If scripting is enabled, then then this option can
  be selected to suppress support for `if-then-else-fi` sequences in scripts.
  This would only be set on systems where some minimal scripting is required but
  `if-then-else-fi` is not.

- `CONFIG_NSH_DISABLE_LOOPS` – If scripting is enabled, then then this option
  can be selected suppress support for `while-do-done` and `until-do-done`
  sequences in scripts. This would only be set on systems where some minimal
  scripting is required but looping is not.

- `CONFIG_NSH_DISABLEBG` – This can be set to `y` to suppress support for
  background commands. This setting disables the `nice` command prefix and the
  `&` command suffix. This would only be set on systems where a minimal
  footprint is a necessity and background command execution is not.

- `CONFIG_NSH_MMCSDMINOR` – If the architecture supports an MMC/SD slot and if
  the NSH architecture specific logic is present, this option will provide the
  MMC/SD minor number, i.e., the MMC/SD block driver will be registered as
  `/dev/mmcsdN` where `N` is the minor number. Default is zero.

- `CONFIG_NSH_ROMFSETC` – Mount a ROMFS file system at `/etc` and provide a
  system init script at `/etc/init.d/rc.sysinit` and a startup script at
  `/etc/init.d/rcS`. The default system init script will mount a FAT FS RAMDISK
  at `/tmp` but the logic is easily extensible.

- `CONFIG_NSH_CONSOLE`

  If `CONFIG_NSH_CONSOLE` is set to `y`, then a serial console front-end is
  selected.

  Normally, the serial console device is a UART and RS-232 interface. However,
  if `CONFIG_USBDEV` is defined, then a USB serial device may, instead, be used
  if the one of the following are defined:

  - `CONFIG_PL2303` and `CONFIG_PL2303_CONSOLE`
    Sets up the Prolifics PL2303 emulation as a console device at
    `/dev/console`.

  - `CONFIG_CDCACM` and `CONFIG_CDCACM_CONSOLE`
    Sets up the CDC/ACM serial device as a console device at `/dev/console`.

  - `CONFIG_NSH_USBCONSOLE` – If defined, then the an arbitrary USB device may
    be used to as the NSH console. In this case, `CONFIG_NSH_USBCONDEV` must be
    defined to indicate which USB device to use as the console.

  - `CONFIG_NSH_USBCONDEV` – If `CONFIG_NSH_USBCONSOLE` is set to `y`, then
    `CONFIG_NSH_USBCONDEV` must also be set to select the USB device used to
    support the NSH console. This should be set to the quoted name of a
    read-/write-able USB driver. Default: `/dev/ttyACM0`.

  If there are more than one USB devices, then a USB device minor number may
  also need to be provided:

  - `CONFIG_NSH_USBDEV_MINOR` – The minor device number of the USB device.
    Default: `0`

  - `CONFIG_NSH_USBKBD` – Normally NSH uses the same device for `stdin`,
    `stdout`, and `stderr`. By default, that device is `/dev/console`. If this
    option is selected, then NSH will use a USB HID keyboard for stdin. In this
    case, the keyboard is connected directly to the target (via a USB host
    interface) and the data from the keyboard will drive NSH. NSH output
    (`stdout` and `stderr`) will still go to `/dev/console`.

  - `CONFIG_NSH_USBKBD_DEVNAME` – If `NSH_USBKBD` is set to `y`, then
    `NSH_USBKBD_DEVNAME` must also be set to select the USB keyboard device used
    to support the NSH console input. This should be set to the quoted name of a
    read- able keyboard driver. Default: `/dev/kbda`.

  - `CONFIG_NSH_USBDEV_TRACE` – If USB tracing is enabled
    (`CONFIG_USBDEV_TRACE`), then NSH can be configured to show the buffered USB
    trace data after each NSH command:

    If `CONFIG_NSH_USBDEV_TRACE` is selected, then USB trace data can be
    filtered as follows. Default: Only USB errors are traced.

    - `CONFIG_NSH_USBDEV_TRACEINIT` - Show initialization events
    - `CONFIG_NSH_USBDEV_TRACECLASS` - Show class driver events
    - `CONFIG_NSH_USBDEV_TRACETRANSFERS` - Show data transfer events
    - `CONFIG_NSH_USBDEV_TRACECONTROLLER` - Show controller events
    - `CONFIG_NSH_USBDEV_TRACEINTERRUPTS` - Show interrupt-related events.

- `CONFIG_NSH_ALTCONDEV` and `CONFIG_NSH_CONDEV`

  If `CONFIG_NSH_CONSOLE` is set to `y`, then `CONFIG_NSH_ALTCONDEV` may also be
  selected to enable use of an alternate character device to support the NSH
  console. If `CONFIG_NSH_ALTCONDEV` is selected, then `CONFIG_NSH_CONDEV` holds
  the quoted name of a readable/write-able character driver such as:
  `CONFIG_NSH_CONDEV="/dev/ttyS1"`. This is useful, for example, to separate the
  NSH command line from the system console when the system console is used to
  provide debug output. Default:  stdin and stdout (probably `/dev/console`)

  **Note 1**: When any other device other than `/dev/console` is used for a
  user interface, (1) linefeeds (`\n`) will not be expanded to carriage return
  / linefeeds (`\r\n`). You will need to configure your terminal program to
  account for this. And (2) input is not automatically echoed so you will have
  to turn local echo on.

  **Note 2**: This option forces the console of all sessions to use
  `NSH_CONDEV`. Hence, this option only makes sense for a system that supports
  only a single session. This option is, in particular, incompatible with
  Telnet sessions because each Telnet session must use a different console
  device.

- `CONFIG_NSH_TELNET` – If `CONFIG_NSH_TELNET` is set to `y`, then a TELENET
  server front-end is selected. When this option is provided, you may log into
  NuttX remotely using telnet in order to access NSH.

- `CONFIG_NSH_ARCHINIT` – Set if your board provides architecture specific
  initialization via the board-interface function `boardctl()`. This function
  will be called early in NSH initialization to allow board logic to do such
  things as configure MMC/SD slots.

If Telnet is selected for the NSH console, then we must configure the resources
used by the Telnet daemon and by the Telnet clients.

- `CONFIG_NSH_TELNETD_PORT` – The telnet daemon will listen on this TCP port
  number for connections. Default: `23`
- `CONFIG_NSH_TELNETD_DAEMONPRIO` – Priority of the Telnet daemon. Default:
  `SCHED_PRIORITY_DEFAULT`
- `CONFIG_NSH_TELNETD_DAEMONSTACKSIZE` – Stack size allocated for the Telnet
  daemon. Default: `2048`
- `CONFIG_NSH_TELNETD_CLIENTPRIO` – Priority of the Telnet client. Default:
  `SCHED_PRIORITY_DEFAULT`
- `CONFIG_NSH_TELNETD_CLIENTSTACKSIZE` – Stack size allocated for the Telnet
  client. Default: `2048`

One or both of CONFIG_NSH_CONSOLE and `CONFIG_NSH_TELNET` must be defined. If
`CONFIG_NSH_TELNET` is selected, then there some other configuration settings
that apply:

- `CONFIG_NET=y` – Of course, networking must be enabled
- `CONFIG_NET_TCP=y` – TCP/IP support is required for telnet (as well as various
  other TCP-related configuration settings).
- `CONFIG_NSH_IOBUFFER_SIZE` – Determines the size of the I/O buffer to use for
  sending/ receiving TELNET commands/responses.
- `CONFIG_NETINIT_DHCPC` – Obtain the IP address via DHCP.
- `CONFIG_NETINIT_IPADDR` – If `CONFIG_NETINIT_DHCPC` is NOT set, then the
  static IP address must be provided.
- `CONFIG_NETINIT_DRIPADDR` – Default router IP address.
- `CONFIG_NETINIT_NETMASK` – Network mask.
- `CONFIG_NETINIT_NOMAC` – Set if your ethernet hardware has no built-in MAC
  address. If set, a bogus MAC will be assigned.

If you use DHCPC, then some special configuration network options are required.
These include:

- `CONFIG_NET=y` – Of course, networking must be enabled.
- `CONFIG_NET_UDP=y` – UDP support is required for DHCP (as well as various other
  UDP-related configuration settings).
- `CONFIG_NET_BROADCAST=y` – UDP broadcast support is needed.
- `CONFIG_NET_ETH_PKTSIZE=650` (or larger). Per RFC2131 (p. 9), the DHCP client
  must be prepared to receive DHCP messages of up to `576` bytes (excluding
  Ethernet, IP, or UDP headers and FCS). **Note**: Note that the actual MTU
  setting will depend upon the specific link protocol. Here Ethernet is
  indicated.

If `CONFIG_NSH_ROMFSETC` is selected, then the following additional
configuration setting apply:

- `CONFIG_NSH_ROMFSMOUNTPT` – The default mountpoint for the ROMFS volume is
  `/etc`, but that can be changed with this setting. This must be a absolute
  path beginning with `/`.

- `CONFIG_NSH_SYSINITSCRIPT` – This is the relative path to the system init
  script within the mountpoint. The default is `init.d/rc.sysinit`. This
  is a relative path and must not start with `/`.

- `CONFIG_NSH_INITSCRIPT` – This is the relative path to the startup script
  within the mountpoint. The default is `init.d/rcS`. This is a relative path
  and must not start with `/`.

- `CONFIG_NSH_ROMFSDEVNO` – This is the minor number of the ROMFS block device.
  The default is `0` corresponding to `/dev/ram0`.

- `CONFIG_NSH_ROMFSSECTSIZE` – This is the sector size to use with the ROMFS
  volume. Since the default volume is very small, this defaults to `64` but
  should be increased if the ROMFS volume were to be become large. Any value
  selected must be a power of `2`.

When the default rcS file used when `CONFIG_NSH_ROMFSETC` is selected, it will
mount a FAT FS under `/tmp`. The following selections describe that FAT FS.

- `CONFIG_NSH_FATDEVNO` – This is the minor number of the FAT FS block device.
  The default is `1` corresponding to `/dev/ram1`.

- `CONFIG_NSH_FATSECTSIZE` – This is the sector size use with the FAT FS.
  Default is `512`.

- `CONFIG_NSH_FATNSECTORS` – This is the number of sectors to use with the FAT
  FS. Default is `1024`. The amount of memory used by the FAT FS will be
  `CONFIG_NSH_FATSECTSIZE` * `CONFIG_NSH_FATNSECTORS` bytes.

- `CONFIG_NSH_FATMOUNTPT` – This is the location where the FAT FS will be
  mounted. Default is `/tmp`.

## Common Problems

Problem:

```
The function 'readline' is undefined.
```

Usual Cause:

The following is missing from your `defconfig` file:

```conf
CONFIG_SYSTEM_READLINE=y
```
