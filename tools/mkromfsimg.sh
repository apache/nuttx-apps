#!/usr/bin/env bash
############################################################################
# apps/tools/mkromfsimg.sh
#
#   Copyright (C) 2014 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name NuttX nor the names of its contributors may be
#    used to endorse or promote products derived from this software
#    without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
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
