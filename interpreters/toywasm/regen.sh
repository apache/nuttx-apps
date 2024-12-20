#! /bin/sh
############################################################################
# apps/interpreters/toywasm/regen.sh
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

# A script to generate a few files for nuttx-apps build.

REPO=https://github.com/yamt/toywasm
REF=${REF:-master}

TMP=$(mktemp -d)
trap "rm -rf ${TMP}" 0
DIR="${TMP}/src"
BUILDDIR="${TMP}/build"

mkdir -p ${DIR}
git -C ${DIR} init
git -C ${DIR} fetch --tags ${REPO} ${REF}
git -C ${DIR} checkout FETCH_HEAD

# Note: for this build, TOYWASM_USE_SHORT_ENUMS is only used for
# the "toywasm --version" output.

cmake -B ${BUILDDIR} \
-DTOYWASM_USE_SHORT_ENUMS=OFF \
-DTOYWASM_ENABLE_WASM_EXCEPTION_HANDLING=ON \
-DTOYWASM_ENABLE_WASM_EXTENDED_CONST=ON \
-DTOYWASM_ENABLE_WASM_MULTI_MEMORY=ON \
-DTOYWASM_ENABLE_WASM_TAILCALL=ON \
-DTOYWASM_ENABLE_WASM_CUSTOM_PAGE_SIZES=ON \
-DTOYWASM_ENABLE_WASM_THREADS=ON \
-DTOYWASM_ENABLE_WASI_THREADS=ON \
-DTOYWASM_ENABLE_DYLD=ON \
-DTOYWASM_ENABLE_DYLD_DLFCN=ON \
${DIR}

for fn in \
    include/toywasm_config.h \
    include/toywasm_version.h \
    src/toywasm_config.c; do

    (m4 -DFILENAME=$fn tmpl/license.in
    case ${fn} in
    *.c)
        cat tmpl/c-sections.in
        ;;
    esac
    # remove comments to appease nxstyle
    sed -e '/^\/\*/d' ${BUILDDIR}/$(basename $fn)) > ${fn}
done
