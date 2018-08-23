symtab
======

Symbol Tables and Build Modes
-----------------------------
This directory provide support for a symbol table which provides all/most of
system and C library services/functions to the application and NSH.

Symbol tables have differing usefulness in different NuttX build modes:

  1. In the FLAT build (CONFIG_BUILD_FLAT), symbol tables are used to bind
     addresses in loaded ELF or NxFLAT modules to base code that usually
     resides in FLASH memory.  Both OS interfaces and user/application
     libraries are made available to the loaded module via symbol tables.

  2. Symbol tables may be of value in a protected build
     (CONFIG_BUILD_PROTECTED) where the newly started user task must
     share resources with other user code (but should use system calls to
     interact with the OS).

  3. But in the kernel build mode (CONFIG_BUILD_LOADABLE), only fully linked
     executables loadable via execl(), execv(), or posix_spawan() can used.
     There is no use for a symbol table with the kernel build since all
     memory resources are separate; nothing is share-able with the newly
     started process.

Creating the Canned Symbol Table
--------------------------------
The support is selected by CONFIG_SYSTEM_SYMTAB option and table has to be
prepared in advance manually.  It can be prepared from NuttX top level
directory by using the following commands:

  cd <nuttx-path>
  cat syscall/syscall.csv libc/libc.csv | sort > <apps-path>/symtab/symtab.csv
  tools/mksymtab <apps-path>/symtab/symtab.csv <apps-path>/symtab/symtab.inc

where:
  <nuttx-path> is the path to the NuttX top level build directory
  <apps-path> is the path to the top level application directory

You may want omit syscall/syscall.csv in the above command in the protected
mode.  It is optional since the system calls are provided through system
call traps.

Your board-level start up code code then needs to select the symbol table
by calling the function symtab_initialize():

  #include "system/symtab.h"
  ...
    symtab_initialize();

Code/Text Size Implications
---------------------------
The option can have substantial effect on system image size, mainly
code/text.  That is because the instructions to generate symtab.inc
above will cause EVERY interface in the NuttX RTOS and the C library to be
included into build.  Add to that the size of a huge symbol table.

In order to reduce the code/text size, you may want to manually prune the
auto-generated symtab.inc file to remove all interfaces that you do
not wish to include into the base FLASH image.
