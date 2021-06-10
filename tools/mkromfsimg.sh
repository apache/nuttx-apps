#!/usr/bin/env bash
############################################################################
# apps/tools/mkromfsimg.sh
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

# Make sure we understand where we are

if [ ! -f tools/mkromfsimg.sh ]; then
  cd .. || { echo "ERROR: cd .. failed"; exit 1; }
  if [ ! -f tools/mkromfsimg.sh ]; then
    error "This script must be executed from the top-level apps/ directory"
    exit 1
  fi
fi

# Environmental stuff

topdir=$PWD
fsdir=${topdir}/bin
romfsimg=romfs.img
headerfile=boot_romfsimg.h

# Sanity checks

if [ ! -d "${fsdir}" ]; then
  echo "ERROR: Directory ${fsdir} does not exist"
  exit 1
fi

genromfs -h 1>/dev/null 2>&1 || { \
  echo "Host executable genromfs not available in PATH"; \
  echo "You may need to download in from http://romfs.sourceforge.net/"; \
  exit 1; \
}

# Now we are ready to make the ROMFS image

genromfs -f ${romfsimg} -d ${fsdir} -V "NuttXBootVol" || { echo "genromfs failed" ; exit 1 ; }

# And, finally, create the header file

xxd -i ${romfsimg} | sed 's/unsigned/const unsigned/' >${headerfile} || \
  { echo "ERROR: xxd of $< failed" ; rm -f ${romfsimg}; exit 1 ; }
rm -f ${romfsimg}
