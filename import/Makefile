############################################################################
# apps/import/Makefile
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

# Sub-directories from the NuttX export package

SUBDIRS = arch include libs scripts startup tmp tools
FILES   = .config System.map User.map

all:
.PHONY: context register depend clean distclean

install:

context:

register:

depend:

clean:

distclean:
	$(foreach SDIR, $(SUBDIRS), $(call DELDIR, $(SDIR)))
	$(foreach FILE, $(FILES), $(call DELFILE, $(FILE)))
