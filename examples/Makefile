############################################################################
# apps/examples/Makefile
#
#   Copyright (C) 2011-2012 Gregory Nutt. All rights reserved.
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

-include $(TOPDIR)/.config # Current configuration

# Sub-directories

SUBDIRS  = adc buttons can cdcacm composite cxxtest dhcpd discover elf
SUBDIRS += flash_test ftpc ftpd hello helloxx hidkbd igmp json keypadtest
SUBDIRS += lcdrw mm modbus mount mtdpart nettest nrf24l01_term nsh null
SUBDIRS += nx nxconsole nxffs nxflat nxhello nximage nxlines nxtext ostest 
SUBDIRS += pashello pipe poll posix_spawn pwm qencoder relays rgmp romfs
SUBDIRS += sendmail serloop slcd smart smart_test tcpecho telnetd thttpd tiff
SUBDIRS += touchscreen udp uip usbserial usbstorage usbterm watchdog
SUBDIRS += wget wgetjson xmlrpc

# Sub-directories that might need context setup.  Directories may need
# context setup for a variety of reasons, but the most common is because
# the example may be built as an NSH built-in function.

CNTXTDIRS = pwm

ifeq ($(CONFIG_NSH_BUILTIN_APPS),y)
CNTXTDIRS += adc can cdcacm composite cxxtest dhcpd discover flash_test ftpd
CNTXTDIRS += hello helloxx json keypadtestmodbus lcdrw mtdpart nettest nx
CNTXTDIRS += nxhello nximage nxlines nxtext nrf24l01_term ostest relays
CNTXTDIRS += qencoder slcd smart_test tcpecho telnetd tiff touchscreen
CNTXTDIRS += usbstorage usbterm watchdog wgetjson
endif

all: nothing

.PHONY: nothing context depend clean distclean

define SDIR_template
$(1)_$(2):
	$(Q) $(MAKE) -C $(1) $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"
endef

$(foreach SDIR, $(CNTXTDIRS), $(eval $(call SDIR_template,$(SDIR),context)))
$(foreach SDIR, $(SUBDIRS), $(eval $(call SDIR_template,$(SDIR),depend)))
$(foreach SDIR, $(SUBDIRS), $(eval $(call SDIR_template,$(SDIR),clean)))
$(foreach SDIR, $(SUBDIRS), $(eval $(call SDIR_template,$(SDIR),distclean)))

nothing:

context: $(foreach SDIR, $(CNTXTDIRS), $(SDIR)_context)

depend: $(foreach SDIR, $(SUBDIRS), $(SDIR)_depend)

clean: $(foreach SDIR, $(SUBDIRS), $(SDIR)_clean)

distclean: $(foreach SDIR, $(SUBDIRS), $(SDIR)_distclean)

-include Make.dep

