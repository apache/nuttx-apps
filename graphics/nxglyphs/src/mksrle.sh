#!/usr/bin/env bash
# apps/graphics/nxglyphs/src/mksrle.sh
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
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
