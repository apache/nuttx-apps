############################################################################
# apps/system/libuv/tests/Makefile
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

# libuv unit-tests command

PROGNAME := $(CONFIG_LIBUV_UNIT_TESTS_PROGNAME)
PRIORITY := $(CONFIG_LIBUV_UNIT_TESTS_PRIORITY)
STACKSIZE := $(CONFIG_LIBUV_UNIT_TESTS_STACKSIZE)
MODULE := $(CONFIG_LIBUV_UNIT_TESTS)

# Files

MAINSRC := test_main.c
CSRCS += runner-nuttx.c
CSRCS += $(LIBUV_UNPACKDIR)/test/runner.c

# Tests

CSRCS += $(LIBUV_UNPACKDIR)/test/test-loop-time.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-async.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-ip4-addr.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-active.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-idle.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-timer.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-timer-from-check.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-timer-again.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-threadpool.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-walk-handles.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-poll-close.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-loop-close.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-loop-stop.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-tcp-read-stop.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-ping-pong.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-tcp-write-after-connect.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-fs-copyfile.c
CSRCS += $(LIBUV_UNPACKDIR)/test/test-fs-poll.c
ifeq ($(CONFIG_DEV_URANDOM),y)
CSRCS += $(LIBUV_UNPACKDIR)/test/test-random.c
endif
# Test helpers

CSRCS += $(LIBUV_UNPACKDIR)/test/echo-server.c

# Compilation flags

CFLAGS += -I$(LIBUV_UNPACKDIR)/test
CFLAGS += -I.
CFLAGS += -D__NUTTX__

clean::
	$(call DELFILE, $(OBJS))

include $(APPDIR)/Application.mk
