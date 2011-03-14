
Application Folder
==================

This folder provides various applications found in sub-directories.

Application entry points with their requirements are gathered together in 
in two files:
	- exec_nuttapp_proto.h	Entry points, prototype function
	- exec_nuttapp_list.h	Application specific information and requirements

Information is collected during the make .depend process.

To execute an application function:
	exec_nuttapp() is defined in the include/nuttx/nuttapp.h 
	
NuttShell provides transparent method of invoking the command, when the
following option is enabled:
	CONFIG_EXAMPLES_NSH_BUILTIN_APPS=y

To select which application to be included in the build process set your
preferences in the nuttx/.config file as:

To include applications under the nuttx apps directory:
	CONFIG_BUILTIN_APPS_NUTTX=y/n
	
where each application can be controlled as:
	CONFIG_BUILTIN_APPS_<NAME>=y/n
	
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
