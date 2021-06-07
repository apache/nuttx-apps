############################################################################
# examples/udgram/Makefile
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

# Basic Unix domain networking test

MAINSRC = udgram_client.c udgram_server.c

PROGNAME = $(CONFIG_EXAMPLES_UDGRAM_CLIENT_PROGNAME)
PROGNAME += $(CONFIG_EXAMPLES_UDGRAM_SERVER_PROGNAME)
PRIORITY = $(CONFIG_EXAMPLES_UDGRAM_CLIENT_PRIORITY)
PRIORITY += $(CONFIG_EXAMPLES_UDGRAM_SERVER_PRIORITY)
STACKSIZE = $(CONFIG_EXAMPLES_UDGRAM_CLIENT_STACKSIZE)
STACKSIZE += $(CONFIG_EXAMPLES_UDGRAM_SERVER_STACKSIZE)

MODULE = $(CONFIG_EXAMPLES_UDGRAM)

include $(APPDIR)/Application.mk
