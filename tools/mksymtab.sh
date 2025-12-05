#!/usr/bin/env bash
############################################################################
# apps/tools/mksymtab.sh
#
# SPDX-License-Identifier: Apache-2.0
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

usage() {
  if [ $# -ne 0 ]; then
    echo "ERROR: $@"
  fi
  echo -e "\nUsage: $0 <imagedirpath1> [imagedirpath2 ... ] [symtabprefix] [-a additionalsymbolspath]"
  exit 1
}

# Collect all image directory/file paths until we hit a non-path argument
dirs=()
prefix=""
arg_count=0

while [ $# -gt 0 ]; do
  # Check if this is an option flag
  if [ "x${1:0:1}" = "x-" ]; then
    break
  fi

  arg_count=$((arg_count + 1))

  # Always collect the argument (whether path exists or not)
  # Only treat as prefix if it doesn't look like a path
  if [[ "$1" =~ ^/ ]] || [ -e "$1" ]; then
    # Looks like a path (starts with /) or exists
    dirs+=("$1")
    shift
  else
    # Doesn't look like a path, treat as prefix
    prefix=$1
    shift
    break
  fi
done

# Check we have at least one argument
if [ $arg_count -eq 0 ]; then
  usage "Missing <imagedirpath>"
fi

# Parse remaining arguments for options
while getopts a: opt; do
  case $opt in
    a)
      addlist="${addlist[@]} $OPTARG"
      ;;
    \?)
      usage
  esac
done

if [ $OPTIND != $(($# + 1)) ]; then
  usage "Arguments remaining: \"${@:$OPTIND}\""
fi

# Function to get exec list from a path (file or directory)
get_exec_list() {
  local path=$1
  if [ -f "$path" ]; then
    echo "$path"
  elif [ -d "$path" ]; then
    find "$path" -type f 2>/dev/null
  fi
}

# Extract all of the undefined symbols from the ELF files and create a
# list of sorted, unique undefined variable names.

# First try to find thunk files from all directories
varlist=""
for dir in "${dirs[@]}"; do
  if [ -d "$dir" ]; then
    thunklist=`find $dir -name *-thunk.S 2>/dev/null | xargs grep -h asciz 2>/dev/null | cut -f3`
    if [ !  -z "$thunklist" ]; then
      varlist="${varlist} ${thunklist}"
    fi
  fi
done

if [ -z "$varlist" ]; then
  # Collect all executable files from all paths
  execlist=""
  for dir in "${dirs[@]}"; do
    # Only process if path exists
    if [ -e "$dir" ]; then
      pathlist=`get_exec_list "$dir"`
      if [ ! -z "$pathlist" ]; then
        execlist="${execlist} ${pathlist}"
      fi
    fi
  done

  if [ ! -z "$execlist" ]; then
    # Get all undefined symbol names
    varlist=$(nm $execlist 2>/dev/null | grep -F ' U ' | sed -e "s/^[ ]*//g" | cut -d' ' -f2)

    # Get all defined symbol names
    deflist=$(nm $execlist 2>/dev/null | grep -F -v -e ' U ' -e ':' | sed -e "s/^[0-9a-z]* //g" | cut -d' ' -f2)

    # Remove the intersection between them, and the remaining symbols are found in the main image
    if [ ! -z "$varlist" ] && [ !  -z "$deflist" ]; then
      varlist=$(echo "$varlist" | tr ' ' '\n' | sort -u | grep -vFxf <(echo "$deflist" | tr ' ' '\n' | sort -u) | tr '\n' ' ')
    fi
  fi
fi

# Sort and unique the varlist
varlist=`echo "$varlist" | tr ' ' '\n' | sort | uniq | tr '\n' ' '`

for addsym in ${addlist[@]}; do
  if [ -f $addsym ]; then
    varlist="${varlist}\n$(cat $addsym | grep -v "^,.*")"
  elif [ -d $addsym ]; then
    varlist="${varlist}\n$(find $addsym -type f | xargs cat | grep -v "^,.*")"
  else
    usage
  fi
  varlist=$(echo -e "${varlist}" | sort -u)
done

# Now output the symbol table as a structure in a C source file.  All
# undefined symbols are declared as void* types.  If the toolchain does
# any kind of checking for function vs. data objects, then this could
# failed

echo "#include <nuttx/compiler.h>"
echo "#include <nuttx/symtab.h>"
echo ""

for string in $varlist; do
  var=`echo $string | sed -e "s/\"//g"`
  echo "extern void *${var/,*/};"
done

echo ""
if [ -z "$prefix" ]; then
  echo "#if defined(CONFIG_EXECFUNCS_HAVE_SYMTAB)"
  echo "const struct symtab_s CONFIG_EXECFUNCS_SYMTAB_ARRAY[] = "
  echo "#elif defined(CONFIG_NSH_SYMTAB)"
  echo "const struct symtab_s CONFIG_NSH_SYMTAB_ARRAYNAME[] = "
  echo "#elif defined(CONFIG_LIBC_ELF_HAVE_SYMTAB)"
  echo "const struct symtab_s CONFIG_LIBC_ELF_SYMTAB_ARRAY[] = "
  echo "#else"
  echo "const struct symtab_s dummy_symtab[] = "
  echo "#endif"
else
  echo "const struct symtab_s ${prefix}_exports[] = "
fi
echo "{"

for string in $varlist; do
  var=`echo $string | sed -e "s/\"//g"`
  echo "  {\"${var/*,/}\", &${var/,*/}},"
done

echo "};"
echo ""
if [ -z "$prefix" ]; then
  echo "#if defined(CONFIG_EXECFUNCS_HAVE_SYMTAB)"
  echo "const int CONFIG_EXECFUNCS_NSYMBOLS_VAR = sizeof(CONFIG_EXECFUNCS_SYMTAB_ARRAY) / sizeof(struct symtab_s);"
  echo "#elif defined(CONFIG_NSH_SYMTAB)"
  echo "const int CONFIG_NSH_SYMTAB_COUNTNAME = sizeof(CONFIG_NSH_SYMTAB_ARRAYNAME) / sizeof(struct symtab_s);"
  echo "#elif defined(CONFIG_LIBC_ELF_HAVE_SYMTAB)"
  echo "const int CONFIG_LIBC_ELF_NSYMBOLS_VAR = sizeof(CONFIG_LIBC_ELF_SYMTAB_ARRAY) / sizeof(struct symtab_s);"
  echo "#else"
  echo "const int dummy_nsymtabs = sizeof(dummy_symtab) / sizeof(struct symtab_s);"
  echo "#endif"
else
  echo "const int ${prefix}_nexports = sizeof(${prefix}_exports) / sizeof(struct symtab_s);"
fi
