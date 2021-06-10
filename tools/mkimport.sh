#!/usr/bin/env bash
# apps/tools/mkimport.sh
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

# Get the input parameter list

USAGE="USAGE: $0 [-d] [-z] [-l <ext>] -x <export-path>"
unset EXPORT
unset TGZ
LIBEXT=.a

while [ ! -z "$1" ]; do
	case $1 in
		-d )
			set -x
			;;
		-l )
			shift
			LIBEXT=$1
			;;
		-x )
			shift
			EXPORT=$1
			;;
		-z )
			TGZ=y
			;;
		-h )
			echo $USAGE
			exit 0
			;;
		* )
			echo "ERROR: Unrecognized argument: $1"
			echo $USAGE
			exit 1
			;;
		esac
	shift
done

# Check arguments

if [ -z "${EXPORT}" ]; then
	echo "ERROR: Missing required argument"
	echo $USAGE
	exit 1
fi

if [ ! -f "${EXPORT}" ]; then
	echo "ERROR: Export file does not exist at ${EXPORT}"
	exit 1
fi

# Orient ourself

if [ ! -d import ]; then
	cd .. || { echo "ERROR: cd .. failed"; exit 1; }
	if [ ! -d import ]; then
		echo "ERROR: This script must be executed from the top-level apps/ directory"
		exit 1
	fi
fi

# Set up paths

WD=${PWD}
IMPORTDIR=${WD}/import
BUILTINDIR=${WD}/builtin
DARCHDIR=${IMPORTDIR}/arch
DINCDIR=${IMPORTDIR}/include
DLIBDIR=${IMPORTDIR}/libs
DSCRIPTSDIR=${IMPORTDIR}/scripts
DSTARTDIR=${IMPORTDIR}/startup
DTOOLSDIR=${IMPORTDIR}/tools
TMPDIR=${IMPORTDIR}/tmp
DALLDIRS="${DARCHDIR} ${DINCDIR} ${DLIBDIR} ${DSCRIPTSDIR} ${DSTARTDIR} ${DTOOLSDIR} ${TMPDIR}"

# Remove any old import directories

rm -rf ${DALLDIRS}

# Create the temporary directory

mkdir ${TMPDIR} || \
	{ echo "ERROR: Failed to create ${TMPDIR}"; exit 1; }

if [ "X${TGZ}" == "Xy" ]; then
	tar zxf ${EXPORT} -C ${TMPDIR} || \
		{ echo "ERROR: tar zxf ${EXPORT} failed"; exit 1; }
else
	unzip ${EXPORT} -d ${TMPDIR} || \
		{ echo "ERROR: unzip ${EXPORT} failed"; exit 1; }
fi

# Unpack the export package into the temporary directory

cd ${TMPDIR} || \
	{ echo "ERROR: Failed to cd to ${TMPDIR}"; exit 1; }

EXPORTDIR=`ls`

if [ -z "${EXPORTDIR}" ]; then
	echo "ERROR: Nothing was unpacked"
	exit 1
fi

if [ ! -d "${EXPORTDIR}" ]; then
	echo "ERROR: No directory found after unpacking"
	exit 1
fi

SARCHDIR=${EXPORTDIR}/arch
SINCDIR=${EXPORTDIR}/include
SLIBDIR=${EXPORTDIR}/libs
SSCRIPTSDIR=${EXPORTDIR}/scripts
SSTARTDIR=${EXPORTDIR}/startup
STOOLSDIR=${EXPORTDIR}/tools
REGISTERSDIR=${EXPORTDIR}/registry

unset SALLDIRS
if [ -d ${SARCHDIR} ]; then
	SALLDIRS="${SALLDIRS} ${SARCHDIR}"
fi
if [ -d ${SINCDIR} ]; then
	SALLDIRS="${SALLDIRS} ${SINCDIR}"
fi
if [ -d ${SLIBDIR} ]; then
	SALLDIRS="${SALLDIRS} ${SLIBDIR}"
fi
if [ -d ${SSCRIPTSDIR} ]; then
	SALLDIRS="${SALLDIRS} ${SSCRIPTSDIR}"
fi
if [ -d ${SSTARTDIR} ]; then
	SALLDIRS="${SALLDIRS} ${SSTARTDIR}"
fi
if [ -d ${STOOLSDIR} ]; then
	SALLDIRS="${SALLDIRS} ${STOOLSDIR}"
fi

# Move all of the exported directories in place in the import directory

mv ${SALLDIRS} ${IMPORTDIR}/. || \
	{ echo "ERROR: Failed to move ${SALLDIRS} to ${IMPORTDIR}"; exit 1; }

cp -rf ${REGISTERSDIR} ${BUILTINDIR}/. || \
	{ echo "ERROR: Failed to move ${REGISTERSDIR} to ${BUILTINDIR}"; exit 1; }

# Move the .config file in place in the import directory

SFILES=".config"
for file in ${SFILES}; do
	if [ -f "${EXPORTDIR}/${file}" ]; then
		cp -a ${EXPORTDIR}/${file} ${IMPORTDIR}/${file} || \
			{ echo "ERROR: Failed to copy ${EXPORTDIR}/${file} to ${IMPORTDIR}/${file}"; exit 1; }
	fi
done

# Finally, remove the temporary directory

cd ${WD} || { echo "ERROR: Failed to cd to ${WD}"; exit 1; }

rm -rf ${TMPDIR}
