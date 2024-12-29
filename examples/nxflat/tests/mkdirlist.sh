#!/usr/bin/env bash
############################################################################
# apps/examples/nxflat/tests/mkdirlist.sh
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
