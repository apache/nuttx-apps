############################################################################
# apps/uploadfiles/Makefile
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

# Upload File volume mounter

PROGNAME = upload
PRIORITY = SCHED_PRIORITY_DEFAULT
STACKSIZE = $(CONFIG_DEFAULT_TASK_STACKSIZE)
MODULE = $(CONFIG_UPLOADFILES)

# Upload File Code

MAINSRC = upload_main.c

UPLOADFILES_DIR = $(APPDIR)$(DELIM)uploadfiles
DATA_DIR        = $(UPLOADFILES_DIR)$(DELIM)data
ROMFS_IMG       = romfs.img
ROMFS_HDR       = romfs.h

# Common build

# Create the romfs.h header file from the tests/ directory

$(ROMFS_IMG) :
	$(Q) genromfs -f $@ -d $(DATA_DIR) -V "UPLOAD_FILES"

$(ROMFS_HDR) : $(ROMFS_IMG)
	$(Q) (xxd -i $(ROMFS_IMG) | sed -e "s/^unsigned/static const unsigned/g" >$@)

# Add the UPLOADFILES object to the archive

context:: $(ROMFS_HDR)

distclean::
	$(call DELFILE, $(ROMFS_HDR))
	$(call DELFILE, $(ROMFS_IMG))

include $(APPDIR)/Application.mk
