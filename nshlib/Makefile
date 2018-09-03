############################################################################
# apps/nshlib/Makefile
#
#   Copyright (C) 2011-2013, 2016 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
############################################################################

-include $(TOPDIR)/Make.defs

# NSH Library

CSRCS  = nsh_init.c nsh_parse.c nsh_console.c nsh_script.c nsh_system.c
CSRCS += nsh_command.c nsh_fscmds.c nsh_ddcmd.c nsh_proccmds.c nsh_mmcmds.c
CSRCS += nsh_timcmds.c nsh_envcmds.c nsh_syscmds.c nsh_dbgcmds.c

ifeq ($(CONFIG_NFILE_STREAMS),0)
CSRCS += nsh_stdsession.c
ifeq ($(CONFIG_NSH_CONSOLE_LOGIN),y)
CSRCS += nsh_stdlogin.c
endif
else
CSRCS += nsh_session.c
ifeq ($(CONFIG_NSH_CONSOLE_LOGIN),y)
CSRCS += nsh_login.c
endif
endif

ifneq ($(CONFIG_NFILE_DESCRIPTORS),0)
CSRCS += nsh_fsutils.c
endif

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
CSRCS += nsh_builtin.c
endif

ifeq ($(CONFIG_NSH_FILE_APPS),y)
CSRCS += nsh_fileapps.c
endif

ifeq ($(CONFIG_NSH_ROMFSETC),y)
CSRCS += nsh_romfsetc.c
endif

ifeq ($(CONFIG_NET),y)
CSRCS += nsh_netinit.c nsh_netcmds.c

ifeq ($(CONFIG_WIRELESS_WAPI),y)
ifeq ($(CONFIG_NSH_NETINIT),y)
ifneq ($(CONFIG_NSH_NETLOCAL),y)
CSRCS += nsh_associate.c
endif
endif
endif

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

ifeq ($(CONFIG_USBHOST),y)
CSRCS += nsh_usbkeyboard.c
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
