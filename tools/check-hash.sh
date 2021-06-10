#!/usr/bin/env bash
############################################################################
# apps/tools/check-hash.sh
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
    sha1|sha224|sha256|sha384|sha512)
        # valid hash passed, continue
        ;;

    *)
        echo "ERROR: invalid hash '${hash_algo}' for file '${file_to_check}'"
        echo "supported hashes are:"
        echo "    sha1, sha224, sha256, sha384, sha512"
        echo ""
        echo ${usage}
        exit 1
esac

# Calculate hash value of passed file

if [ `which ${hash_algo}sum 2> /dev/null` ]; then
    hash_algo_cmd="${hash_algo}sum"
elif [ `which shasum 2> /dev/null` ]; then
    hash_algo_len=$( echo ${hash_algo} | cut -c 4- )
    hash_algo_cmd="shasum -a ${hash_algo_len}"
fi

calc_hash=$( ${hash_algo_cmd} "${file_to_check}" | cut -d' ' -f1 )

# Does it match expected hash?

if [ "${exp_hash}" == "${calc_hash}" ]; then
    # yes, they match, we're good
    exit 0
fi

# No, hashes don't match, print error message and remove corrupted file

echo "ERROR: file ${file_to_check} has invalid hash"
echo "got:      ${calc_hash}"
echo "expected: ${exp_hash}"
rm "${file_to_check}"
exit 1
