#!/usr/bin/env bash

usage="Usage: %0 <test-dir-path>"

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

varlist=`find $dir -name "*-thunk.S"| xargs grep -h asciz | cut -f3 | sort | uniq`

# Now output the symbol table as a structure in a C source file.  All
# undefined symbols are declared as void* types.  If the toolchain does
# any kind of checking for function vs. data objects, then this could
# failed

echo "#include <nuttx/compiler.h>"
echo "#include <nuttx/symtab.h>"
echo ""

for var in $varlist; do
	echo "extern void *${var};"
done

echo ""
echo "const struct symtab_s g_thttpd_exports[] = "
echo "{"

for string in $varlist; do
	var=`echo $string | sed -e "s/\"//g"`
	echo "  {$string, $var},"
done

echo "};"
echo "const int g_thttpd_nexports = sizeof(g_thttpd_exports) / sizeof(struct symtab_s);"
