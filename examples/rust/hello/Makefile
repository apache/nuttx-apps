############################################################################
# apps/examples/rust/hello/Makefile
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

# Hello, Rust! built-in application info

PROGNAME  = $(CONFIG_EXAMPLES_HELLO_RUST_CARGO_PROGNAME)
PRIORITY  = $(CONFIG_EXAMPLES_HELLO_RUST_CARGO_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_HELLO_RUST_CARGO_STACKSIZE)
MODULE    = $(CONFIG_EXAMPLES_HELLO_RUST_CARGO)

RUST_LIB  := $(call RUST_GET_BINDIR,hello,$(APPDIR)/examples/rust)
RUST_SRCS := $(call RUST_CARGO_SRCS,hello,$(APPDIR)/examples/rust)

ifeq ($(CONFIG_ARCH_SIM),y)
HELLO_RUST_FEATURES := sim
endif

$(RUST_LIB): $(RUST_SRCS)
	$(call RUST_CARGO_BUILD,hello,$(APPDIR)/examples/rust,$(HELLO_RUST_FEATURES))

context:: $(RUST_LIB)

all:: $(RUST_LIB)

clean::
	$(call RUST_CARGO_CLEAN,hello,$(APPDIR)/examples/rust)

include $(APPDIR)/Application.mk
