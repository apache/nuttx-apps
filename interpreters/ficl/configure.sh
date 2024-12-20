#!/usr/bin/env bash
############################################################################
# apps/interpreters/ficl/configure.sh
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


USAGE="$0 <Ficl-dir>"

FICLDIR=$1
if [ -z "${FICLDIR}" ]; then
    echo "Missing command line argument"
    echo $USAGE
    exit 1
fi

if [ ! -d "${FICLDIR}" ]; then
    echo "Sub-directory ${FICLDIR} does not exist"
    echo $USAGE
    exit 1
fi

if [ ! -r "${FICLDIR}/Makefile" ]; then
    echo "Readable ${FICLDIR}/Makefile does not exist"
    echo $USAGE
    exit 1
fi

OBJECTS=`grep "^OBJECTS" ${FICLDIR}/Makefile`
if [ -z "${OBJECTS}" ]; then
    echo "No OBJECTS found in ${FICLDIR}/Makefile"
    echo $USAGE
    exit 1
fi

OBJLIST=`echo ${OBJECTS} | cut -d'=' -f2 | sed -e "s/unix\.o//g"`

rm -f Make.srcs
echo "# apps/interpreters/ficl/Make.obs" >> Make.srcs
echo "# Auto-generated file.. Do not edit" >> Make.srcs
echo "" >> Make.srcs
echo "FICL_SUBDIR = ${1}" >> Make.srcs
echo "FICL_DEPPATH = --dep-path ${1}" >> Make.srcs

unset CSRCS
for OBJ in ${OBJLIST}; do
    SRC=`echo ${OBJ} | sed -e "s/\.o/\.c/g"`
    CSRCS=${CSRCS}" ${SRC}"
done
echo "FICL_ASRCS = " >> Make.srcs
echo "FICL_CXXSRCS = " >> Make.srcs
echo "FICL_CSRCS = ${CSRCS}" >> Make.srcs
