############################################################################
# apps/examples/mcuboot/swap_test/Makefile
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

MAINSRC   += confirm_main.c set_img_main.c version_main.c

PROGNAME  += mcuboot_confirm mcuboot_set_img mcuboot_version
PRIORITY  += SCHED_PRIORITY_DEFAULT
STACKSIZE += $(CONFIG_DEFAULT_TASK_STACKSIZE)

include $(APPDIR)/Application.mk
