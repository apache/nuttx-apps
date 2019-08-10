#!/usr/bin/env bash
# apps/graphics/nxglyphs/src/mksrle.sh
#
#   Copyright (C) 2019 Gregory Nutt. All rights reserved.
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

# Get the input parameter list
#
# <input-file>   - A 4-color image exported as a GIMP C-source file.
# <output-file>  - A 2-bpp cursor image in NuttX cursor format
# <back-ground>  - Treated as the transparent background color (default, 0x000000)
# <color1>       - Usually the primary color of the image (default, 0xff0000)
# <color2>       - Usually the outline color of the image (default, 0x00007f)
# <color3>       - Usually blend of color1 and color2 for low-cost anti-aliasing (default, 0x7f003f)
# Output is to stdout, but may be re-directed to a file.

PROGNAME=$0
USAGE="USAGE: $PROGNAME [-d] -f <input-file> -o <output-file>"

CFILE=mksrle.c
TMPFILE1=_tmpfile1.c
TMPFILE2=_mksrle.c
TMPPROG=_mksrle.exe

unset INFILE
unset OUTFILE

while [ ! -z "$1" ]; do
  case $1 in
    -d )
      set -x
      ;;
    -f )
      shift
      INFILE=$1
      ;;
    -o )
      shift
      OUTFILE=$1
      ;;
    * )
      echo "Unrecognized argument: $1"
      echo $USAGE
      exit 1
      ;;
    esac
  shift
done

# Check arguments

if [ -z "${INFILE}" ]; then
  echo "MK: Missing input Gimp C-source file name"
  echo $USAGE
  exit 1
fi

if [ ! -r ${INFILE} ]; then
  echo "MK: No readable file at ${INFILE}"
  exit 1
fi

if [ ! -r ${CFILE} ]; then
  echo "MK: Can't find ${CFILE}"
  exit 1
fi

if [ -z ${OUTFILE} ]; then
  echo "MK: Missing output file name"
  exit 1
fi

echo "#include <stdint.h>" > ${TMPFILE1}
echo "typedef uint8_t guint8;" >> ${TMPFILE1}
echo "typedef uint16_t guint;" >> ${TMPFILE1}

cat ${TMPFILE1} ${INFILE} ${CFILE} > ${TMPFILE2}

gcc -g -o ${TMPPROG} ${TMPFILE2}
./${TMPPROG} > ${OUTFILE}

rm ${TMPFILE1} ${TMPFILE2}
