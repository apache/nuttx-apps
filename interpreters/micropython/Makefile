############################################################################
# interpreters/micropython
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

#DUKTAPE_VERSION  = 2.5.0
#DUKTAPE_UNPACK   = duktape
#DUKTAPE_TARBALL  = duktape-$(DUKTAPE_VERSION).tar.xz
#DUKTAPE_URL_BASE = https://github.com/svaarala/duktape/releases/download/
#DUKTAPE_URL      = $(DUKTAPE_URL_BASE)v$(DUKTAPE_VERSION)/$(DUKTAPE_TARBALL)

MPY_DIR  = ../../../micropython
MPY_PORT_DIR = $(MPY_DIR)/ports/unix
MPY_PORT_VARIANT_DIR = $(MPY_PORT_DIR)/variants/nuttx
#TOP = $(MICROPYTHON_DIR)
BUILD=$(MPY_PORT_DIR)/build-nuttx

#include $(MICROPYTHON_DIR)/py/mkenv.mk

#include $(APP_MICROPYTHON_DIR)/Makefile
#$(MAKE) -C $(APP_MICROPYTHON_DIR)
#include $(MICROPYTHON_UNIX_DIR)/variants/minimal/mpconfigvariant.mk

UNAME_S := NuttX
PROGNAME  = micropython
PRIORITY  = $(CONFIG_INTERPRETERS_MICROPYTHON_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_MICROPYTHON_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_MICROPYTHON)


#######################################
# BEGIN MICROPYTHON MAKE 
#######################################


VPATH +=  \
	 $(MPY_PORT_DIR)

INC += -I$(MPY_PORT_DIR) \
       -I$(MPY_PORT_VARIANT_DIR) \
       -I$(TOP) \
       -I$(BUILD) \

CFLAGS += $(INC)

OBJS += $(wildcard *.o)

#MAINSRC = main.c

CFLAGS += -Wno-undef

export CFLAGS

#ifneq ($(CONFIG_NET),y) # DOESN'T override so does nothing
#export MICROPY_PY_SOCKET = 0
#endif

.PHONY: micropython
micropython:
	@echo "Calling MPY Make"
	$(MAKE) -C $(MPY_PORT_DIR) lib VARIANT=nuttx CROSS_COMPILE=$(CROSSDEV) PROG="" \
		MICROPY_PY_SOCKET=0 MICROPY_SSL_AXTLS=0 MICROPY_PY_USSL=0
	$(CROSSDEV)ar -x $(MPY_PORT_DIR)/libmicropython.a

context:: micropython

#$(MPY_PORT_DIR)/libmicropython.a: micropython

#####################################
# END MICROPYTHON MAKE
#####################################

include $(APPDIR)/Application.mk
