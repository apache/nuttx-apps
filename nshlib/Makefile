############################################################################
# apps/nshlib/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

# NSH Library

CSRCS  = nsh_init.c nsh_parse.c nsh_console.c nsh_script.c nsh_system.c
CSRCS += nsh_command.c nsh_fscmds.c nsh_ddcmd.c nsh_proccmds.c nsh_mmcmds.c
CSRCS += nsh_timcmds.c nsh_envcmds.c nsh_syscmds.c nsh_dbgcmds.c

ifeq ($(CONFIG_FILE_STREAM),y)
CSRCS += nsh_session.c
ifeq ($(CONFIG_NSH_CONSOLE_LOGIN),y)
CSRCS += nsh_login.c
endif
else
CSRCS += nsh_stdsession.c
ifeq ($(CONFIG_NSH_CONSOLE_LOGIN),y)
CSRCS += nsh_stdlogin.c
endif
endif

CSRCS += nsh_fsutils.c

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
CSRCS += nsh_builtin.c
endif

ifeq ($(CONFIG_NSH_FILE_APPS),y)
CSRCS += nsh_fileapps.c
endif

ifeq ($(CONFIG_NSH_VARS),y)
CSRCS += nsh_vars.c
endif

ifeq ($(CONFIG_NSH_ROMFSETC),y)
CSRCS += nsh_romfsetc.c
endif

ifeq ($(CONFIG_NET),y)
CSRCS += nsh_netcmds.c

ifeq ($(CONFIG_NET_ROUTE),y)
CSRCS += nsh_routecmds.c
endif
endif

ifneq ($(CONFIG_DISABLE_MOUNTPOINT),y)
CSRCS += nsh_mntcmds.c
endif

ifeq ($(CONFIG_MODULE),y)
ifneq ($(CONFIG_NSH_DISABLE_MODCMDS),y)
CSRCS += nsh_modcmds.c
endif
endif

ifeq ($(CONFIG_NSH_CONSOLE),y)
CSRCS += nsh_consolemain.c
endif

ifneq ($(CONFIG_NSH_DISABLE_PRINTF),y)
CSRCS += nsh_printf.c
endif

ifeq ($(CONFIG_NSH_TELNET),y)
CSRCS += nsh_telnetd.c
ifeq ($(CONFIG_NSH_TELNET_LOGIN),y)
CSRCS += nsh_telnetlogin.c
endif
endif

ifneq ($(CONFIG_NSH_DISABLESCRIPT),y)
CSRCS += nsh_test.c
endif

ifeq ($(CONFIG_USBDEV),y)
CSRCS += nsh_usbconsole.c
endif

ifeq ($(CONFIG_NSH_ALTCONDEV),y)
CSRCS += nsh_altconsole.c
endif

ifeq ($(CONFIG_NSH_USBDEV_TRACE),y)
CSRCS += nsh_usbtrace.c
endif

ifeq ($(CONFIG_NETUTILS_CODECS),y)
CSRCS += nsh_codeccmd.c
endif

ifeq ($(CONFIG_NSH_LOGIN_PASSWD),y)
CSRCS += nsh_passwdcmds.c
endif

include $(APPDIR)/Application.mk
