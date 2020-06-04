#!/usr/bin/env bash

usage="Usage: %0 <romfs-dir-path>"

dir=$1
if [ -z "$dir" ]; then
	echo "ERROR: Missing <romfs-dir-path>"
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

echo "#include <stddef.h>"
echo ""
echo "const char *dirlist[] ="
echo "{"

for file in `ls $dir`; do
	echo "  \"$file\","
done

echo "  NULL"
echo "};"
