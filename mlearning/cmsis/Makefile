############################################################################
# apps/mlearning/cmsis/Makefile
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

cmsis.zip:
	$(Q) curl -L https://github.com/ARM-software/CMSIS_5/archive/refs/tags/$(CONFIG_CMSIS_VER).zip -o cmsis.zip
	$(Q) unzip -o cmsis.zip
	$(Q) mv CMSIS_5-$(CONFIG_CMSIS_VER) CMSIS_5
	$(Q) patch -p0 < cmsis-nn-support_nnabla.patch

context:: cmsis.zip

distclean::
	$(call DELDIR, CMSIS_5)
	$(call DELFILE, cmsis.zip)

include $(APPDIR)/Application.mk
