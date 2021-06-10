#!/usr/bin/env bash
############################################################################
# apps/tools/mksymtab.sh
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

export LC_ALL=C

usage="Usage: $0 <imagedirpath> [symtabprefix]"

# Check for the required directory path

dir=$1
if [ -z "$dir" ]; then
  echo "ERROR: Missing <imagedirpath>"
  echo ""
  echo $usage
  exit 1
fi

# Get the symbol table prefix

prefix=$2

# Extract all of the undefined symbols from the ELF files and create a
# list of sorted, unique undefined variable names.

varlist=`find $dir -name *-thunk.S 2>/dev/null | xargs grep -h asciz | cut -f3 | sort | uniq`
if [ -z "$varlist" ]; then
  execlist=`find $dir -type f -perm -a=x 2>/dev/null`
  if [ ! -z "$execlist" ]; then
    varlist=`nm $execlist | fgrep ' U ' | sed -e "s/^[ ]*//g" | cut -d' ' -f2 | sort | uniq`
  fi
fi

# Now output the symbol table as a structure in a C source file.  All
# undefined symbols are declared as void* types.  If the toolchain does
# any kind of checking for function vs. data objects, then this could
# failed

echo "#include <nuttx/compiler.h>"
echo "#include <nuttx/symtab.h>"
echo ""

for string in $varlist; do
  var=`echo $string | sed -e "s/\"//g"`
  echo "extern void *${var};"
done

echo ""
if [ -z "$prefix" ]; then
  echo "#if defined(CONFIG_EXECFUNCS_HAVE_SYMTAB)"
  echo "const struct symtab_s CONFIG_EXECFUNCS_SYMTAB_ARRAY[] = "
  echo "#elif defined(CONFIG_SYSTEM_NSH_SYMTAB)"
  echo "const struct symtab_s CONFIG_SYSTEM_NSH_SYMTAB_ARRAYNAME[] = "
  echo "#else"
  echo "const struct symtab_s dummy_symtab[] = "
  echo "#endif"
else
  echo "const struct symtab_s ${prefix}_exports[] = "
fi
echo "{"

for string in $varlist; do
  var=`echo $string | sed -e "s/\"//g"`
  echo "  {\"${var}\", &${var}},"
done

echo "};"
echo ""
if [ -z "$prefix" ]; then
  echo "#if defined(CONFIG_EXECFUNCS_HAVE_SYMTAB)"
  echo "const int CONFIG_EXECFUNCS_NSYMBOLS_VAR = sizeof(CONFIG_EXECFUNCS_SYMTAB_ARRAY) / sizeof(struct symtab_s);"
  echo "#elif defined(CONFIG_SYSTEM_NSH_SYMTAB)"
  echo "const int CONFIG_SYSTEM_NSH_SYMTAB_COUNTNAME = sizeof(CONFIG_SYSTEM_NSH_SYMTAB_ARRAYNAME) / sizeof(struct symtab_s);"
  echo "#else"
  echo "const int dummy_nsymtabs = sizeof(dummy_symtab) / sizeof(struct symtab_s);"
  echo "#endif"
else
  echo "const int ${prefix}_nexports = sizeof(${prefix}_exports) / sizeof(struct symtab_s);"
fi
