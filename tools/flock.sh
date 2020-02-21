#! /bin/sh

# apps/tools/flock.sh
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

# This script tries to mimic flock using shlock
#
# NOTE: It just aims to be enough for our usage in libapps build.
# Not intended to be a full replacement of flock.

LOCKFILE=$1
shift

while ! shlock -f ${LOCKFILE} -p $$; do
	sleep 1
done

set -e
$@
STATUS=$?
rm -f ${LOCKFILE} || :
exit ${STATUS}
