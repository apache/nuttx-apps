#!/usr/bin/env bash
#################################################################################
# apps/graphics/nxwidgets/Doxygen/gendoc.sh
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
#################################################################################
#
# set -x
# Functions

function ShowUsage()
{
    echo ""
    echo "USAGE: $0 <doxygen-output-directory-path>"
    echo ""
    echo "Where:"
    echo " <doxygen-output-directory-path> is the full, absolute path to place the doxygen output"
    echo ""
}

# Input parameters

DOXYGENOUTPUT_DIR=$1
if [ -z "${DOXYGENOUTPUT_DIR}" ]; then
    echo "Missing required arguments"
    ShowUsage
    exit 1
fi

# Check that the directory exist

if [ ! -d "${DOXYGENOUTPUT_DIR}" ]; then
    echo "Directory ${DOXYGENOUTPUT_DIR} does not exist"
    exit 1
fi

# Find the doxygen configuration file

DOXYFILE="Doxyfile"
if [ ! -e "${DOXYFILE}" ]; then
    echo "This script must be executed in the documentation/ directory"
    exit 1
fi

doxygen "${DOXYFILE}" || \
    {
        echo "Failed to run doxygen"; \
        exit 1;

    }

cp -rf html "${DOXYGENOUTPUT_DIR}" || \
    {
        echo "Failed to move html output"; \
        exit 1;
    }

rm -rf html || \
    {
        echo "Failed to remove the html/ directory"; \
        exit 1;
    }

echo "open ${DOXYGENOUTPUT_DIR}/html/index.html to start browsing"
