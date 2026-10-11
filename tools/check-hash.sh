#!/usr/bin/env bash
############################################################################
# apps/tools/check-hash.sh
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

usage="Usage: $0 <hash-algo> <expected-hash> <file-to-check>"

if [ ${#} -ne 3 ]
then
    echo "ERROR: invalid number of arguments passed"
    echo ""
    echo ${usage}
    exit 1
fi

hash_algo=${1}
exp_hash=${2}
file_to_check=${3}

if [ ! -f "${file_to_check}" ]; then
    echo "ERROR: file '${file_to_check}' does not exist"
    echo ""
    echo ${usage}
    exit 1
fi

case "${hash_algo}" in
    sha1|sha224|sha256|sha384|sha512|md5)
        # valid hash passed, continue
        ;;

    *)
        echo "ERROR: invalid hash '${hash_algo}' for file '${file_to_check}'"
        echo "supported hashes are:"
        echo "    sha1, sha224, sha256, sha384, sha512, md5"
        echo ""
        echo ${usage}
        exit 1
esac

# If expected hash is explicitly set to skip or empty, allow bypass

if [ -z "${exp_hash}" ] || [ "${exp_hash}" = "none" ] || [ "${exp_hash}" = "skip" ]; then
    echo "WARNING: hash check skipped for '${file_to_check}'"
    exit 0
fi

# Calculate hash value of passed file

hash_algo_cmd=""
if [ "${hash_algo}" = "md5" ]; then
    if command -v md5sum > /dev/null 2>&1; then
        hash_algo_cmd="md5sum"
    elif command -v md5 > /dev/null 2>&1; then
        hash_algo_cmd="md5 -q"
    elif command -v openssl > /dev/null 2>&1; then
        hash_algo_cmd="openssl dgst -md5 -r"
    fi
else
    if command -v "${hash_algo}sum" > /dev/null 2>&1; then
        hash_algo_cmd="${hash_algo}sum"
    elif command -v shasum > /dev/null 2>&1; then
        hash_algo_len=$(echo "${hash_algo}" | cut -c 4-)
        hash_algo_cmd="shasum -a ${hash_algo_len}"
    elif command -v openssl > /dev/null 2>&1; then
        hash_algo_cmd="openssl dgst -${hash_algo} -r"
    fi
fi

if [ -z "${hash_algo_cmd}" ]; then
    echo "ERROR: no suitable hash tool found for '${hash_algo}'"
    exit 1
fi

calc_hash=$(${hash_algo_cmd} "${file_to_check}" | awk '{print $1}')

# Normalize hashes to lowercase for case-insensitive comparison

exp_hash=$(echo "${exp_hash}" | tr '[:upper:]' '[:lower:]')
calc_hash=$(echo "${calc_hash}" | tr '[:upper:]' '[:lower:]')

# Does it match expected hash?

if [ "${exp_hash}" = "${calc_hash}" ]; then
    # yes, they match, we're good
    exit 0
fi

# No, hashes don't match, print error message and remove corrupted file

echo "ERROR: file ${file_to_check} has invalid hash"
echo "got:      ${calc_hash}"
echo "expected: ${exp_hash}"
rm -f "${file_to_check}"
exit 1
