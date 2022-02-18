############################################################################
# interpreters/micropython/Makefile
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

MPY_DIR  = micropython
MPY_PORT_DIR = $(MPY_DIR)/ports/unix
MPY_PORT_VARIANT_DIR = $(MPY_PORT_DIR)/variants/nuttx

UNAME_S := NuttX
PROGNAME  = micropython
PRIORITY  = $(CONFIG_INTERPRETERS_MICROPYTHON_PRIORITY)
STACKSIZE = $(CONFIG_INTERPRETERS_MICROPYTHON_STACKSIZE)
MODULE    = $(CONFIG_INTERPRETERS_MICROPYTHON)

OBJS += $(wildcard *.o)

CFLAGS += -Wno-undef
CFLAGS += -DMICROPY_UNIX_STACKLIMIT=$(CONFIG_INTERPRETERS_MICROPYTHON_STACKSIZE)
CFLAGS += -DMICROPY_UNIX_DEFAULT_HEAPSIZE=$(CONFIG_INTERPRETERS_MICROPYTHON_HEAPSIZE)

# Micropython will use the CFLAGS that we export
export CFLAGS

# Tell micropython to use the correct compiler
MPY_OVERRIDES = "CROSS_COMPILE=$(CROSSDEV)"

ifneq ($(CONFIG_INTERPRETERS_MICROPYTHON_SOCKET),y)
MPY_OVERRIDES += "MICROPY_PY_SOCKET=0"
endif

ifeq ($(CONFIG_INTERPRETERS_MICROPYTHON_USSL),y)
ifeq ($(CONFIG_INTERPRETERS_MICROPYTHON_MBEDTLS),y)
MPY_OVERRIDES += "MICROPY_PY_MBEDTLS=1"
endif
else
MPY_OVERRIDES += "MICROPY_PY_USSL=0"
endif

micropython:
	$(Q) git clone https://github.com/mransom-campbell/micropython.git -b nuttx
	$(MAKE) -C $(MPY_PORT_DIR) submodules VARIANT=nuttx
	# build mpy-cross
	$(MAKE) -C $(MPY_DIR)/mpy-cross


.PHONY: micropython_make
micropython_make: micropython
	@echo "Calling MPY Make"
	$(MAKE) -C $(MPY_PORT_DIR) lib VARIANT=nuttx $(MPY_OVERRIDES)
	$(CROSSDEV)ar -x $(MPY_PORT_DIR)/libmicropython.a

context:: micropython_make

clean::
	$(Q) test ! -d $(MPY_PORT_DIR) || make -C $(MPY_PORT_DIR) clean VARIANT=nuttx

distclean::
	$(call DELDIR, micropython)

# doesn't work for some reason
#$(MPY_PORT_DIR)/libmicropython.a: micropython

include $(APPDIR)/Application.mk
