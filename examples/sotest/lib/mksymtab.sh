#!/bin/bash
# set -x

usage="Usage: $0 <test-dir-path>"

# Check for the required ROMFS directory path

dir=$1
if [ -z "$dir" ]; then
	echo "ERROR: Missing <test-dir-path>"
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

# Extract all of the undefined symbols from the SOTEST files and create a
# list of sorted, unique undefined variable names.

tmplist=`find ${dir} -executable -type f | xargs nm | fgrep ' U ' | sed -e "s/^[ ]*//g" | cut -d' ' -f2 | sort | uniq`

# Remove the special symbol 'modprint'.  It it is not exported by the
# base firmware, but rather in this test from one shared library to another.

varlist=`echo $tmplist | sed -e "s/modprint//g"`

# Now output the symbol table as a structure in a C source file.  All
# undefined symbols are declared as void* types.  If the toolchain does
# any kind of checking for function vs. data objects, then this could
# fail

echo "#include <nuttx/compiler.h>"
echo "#include <nuttx/binfmt/symtab.h>"
echo ""

for var in $varlist; do
	echo "extern void *${var};"
done

echo ""
echo "const struct symtab_s g_sot_exports[] = "
echo "{"

for var in $varlist; do
	echo "  {\"${var}\", &${var}},"
done

echo "};"
echo ""
echo "const int g_sot_nexports = sizeof(g_sot_exports) / sizeof(struct symtab_s);"

