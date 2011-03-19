Application Folder
==================

This folder provides various applications found in sub-directories.

Application entry points with their requirements are gathered together in 
in two files:

	- exec_nuttapp_proto.h	Entry points, prototype function
	- exec_nuttapp_list.h	Application specific information and requirements

Information is collected during the make .depend process.

To execute an application function:

	exec_nuttapp() is defined in the nuttx/include/apps/apps.h 
	
NuttShell provides transparent method of invoking the command, when the
following option is enabled:

	CONFIG_NSH_BUILTIN_APPS=y

in the NuttX configuration.

A special configuration file is used to configure which applications
are to be included in the build.  This file is configs/<board>/<configuration>/appconfig.
The existence of the appconfig file in the board configuration directory
is sufficient to enable building of applications.

The appconfig file is copied into the apps/ directory as .config when
NuttX is configured.  .config is included in the toplevel apps/Makefile.
As a minimum, this configuration file must define files to add to the
CONFIGURED_APPS list like:

  CONFIGURED_APPS  += hello/.built_always poweroff/.built_always jvm/.built_always

The form of each entry is <dir>/<dependency> when:

  <dir> is the name of a subdirectory in the apps directory, and

  <dependency> is a make dependency.  This will be "touch"-ed each time
  that the sub-directory is rebuilt.

When the user defines an option:
	CONFIG_BUILTIN_APP_START=<application name>
	
that application shall be invoked immediately after system starts.
Note that application name must be provided in ".." as: "hello", 
will call:
	int hello_main(int argc, char *argv[])

Application skeleton can be found under the hello sub-directory,
which shows how an application can be added to the project. One must
define:

 1. create sub-directory as: appname
 2. provide entry point: appname_main()
 3. set the requirements in the file: Makefile, specially the lines:
	APPNAME		= appname
	PRIORITY	= SCHED_PRIORITY_DEFAULT
	STACKSIZE	= 768
	ASRCS		= asm source file list as a.asm b.asm ...
	CSRCS		= C source file list as foo1.c foo2.c ..

 4. add application in the apps/Makefile



