#!/bin/bash
############################################################################
# apps/tools/mksymtab.sh
#
#   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

usage="Usage: $0 <imagedirpath> <outfilepath>"

# Make sure we understand where we are

if [ ! -f tools/mksymtab.sh ]; then
  cd .. || { echo "ERROR: cd .. failed"; exit 1; }
  if [ ! -f tools/mksymtab.sh ]; then
    error "This script must be executed from the top-level apps/ directory"
    exit 1
  fi
fi

# Check for the required directory path

dir=$1
if [ -z "$dir" ]; then
  echo "ERROR: Missing <imagedirpath>"
  echo ""
  echo $usage
  exit 1
fi

if [ ! -d "$dir" ]; then
  echo "ERROR: Directory $dir does not exist"
  echo ""
  echo $usage
  exit 1
fi

# Get the output file name

outfile=$2
if [ -z "$outfile" ]; then
  echo "ERROR: Missing <outfilepath>"
  echo ""
  echo $usage
  exit 1
fi

rm -f $outfile

# Extract all of the undefined symbols from the ELF files and create a
# list of sorted, unique undefined variable names.

execlist=`find ${dir} -executable -type f`
for exec in ${execlist}; do
  nm $exec | fgrep ' U ' | sed -e "s/^[ ]*//g" | cut -d' ' -f2  >>_tmplist
done

varlist=`cat _tmplist | sort - | uniq -`
rm -f _tmplist

# Now output the symbol table as a structure in a C source file.  All
# undefined symbols are declared as void* types.  If the toolchain does
# any kind of checking for function vs. data objects, then this could
# faile

echo "#include <nuttx/compiler.h>" >$outfile
echo "#include <nuttx/binfmt/symtab.h>" >>$outfile
echo "" >>$outfile

for var in $varlist; do
  echo "extern void *${var};" >>$outfile
done

echo "" >>$outfile
echo "const struct symtab_s g_symtab[] = " >>$outfile
echo "{" >>$outfile

for var in $varlist; do
  echo "  {\"${var}\", &${var}}," >>$outfile
done

echo "};" >>$outfile
echo "" >>$outfile
echo "const int g_nsymbols = sizeof(g_symtab) / sizeof(struct symtab_s);" >>$outfile
