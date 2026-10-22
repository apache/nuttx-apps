#!/usr/bin/env bash
############################################################################
# apps/testing/bluetooth/bsworld/prepare.sh
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
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
############################################################################

set -euo pipefail

usage()
{
  cat <<'EOF'
Usage: prepare.sh BSWORLD_DIR DEPENDENCY_DIR

Prepare BSWorld, its Python Controller and BabbleSim on Linux/x86_64.
BSWorld must be checked out at the revision pinned in bluetooth.yml.
BUILD_JOBS defaults to nproc. See README.md for build prerequisites.
EOF
}

if [ "${1:-}" = -h ] || [ "${1:-}" = --help ]; then
  usage
  exit 0
fi
if [ "$#" -ne 2 ]; then
  usage
  exit 2
fi

bsworld=$(realpath "$1")
mkdir -p "$2"
deps=$(realpath "$2")
revision=$(git -C "$bsworld" rev-parse HEAD)

if [ -f "$deps/bsworld-revision" ]; then
  if [ "$(cat "$deps/bsworld-revision")" != "$revision" ]; then
    echo "BSWorld revision changed; use a new dependency directory." >&2
    exit 1
  fi
fi

python3.12 -m venv "$deps/venv"
# shellcheck disable=SC1091
source "$deps/venv/bin/activate"
python -m pip install -e "$bsworld" -e "$bsworld/components/bscontroller" kconfiglib

export BSIM_OUT_PATH="$deps/babblesim/build"
export BSIM_COMPONENTS_PATH="$deps/babblesim/components"
export BSIM_BUILD_FAIL_ASAP=1
mkdir -p "$deps/babblesim"
(
  cd "$deps/babblesim"
  repo init --standalone-manifest --depth=1 \
    -u "file://$bsworld/components/babblesim.xml"
  repo sync -c --no-tags
  make -j"${BUILD_JOBS:-$(nproc)}" everything
)

test -x "$BSIM_OUT_PATH/bin/bs_2G4_phy_v1"
python -c 'from bsworld.devices import BSController'
printf '%s\n' "$revision" > "$deps/bsworld-revision"
python -m pip freeze > "$deps/python-packages.txt"
(
  cd "$deps/babblesim"
  repo manifest -r
) > "$deps/babblesim-manifest.xml"
{
  printf 'source %q\n' "$deps/venv/bin/activate"
  printf 'export BSWORLD_ROOT=%q\n' "$bsworld"
  printf 'export BSIM_OUT_PATH=%q\n' "$BSIM_OUT_PATH"
  printf 'export BSIM_COMPONENTS_PATH=%q\n' "$BSIM_COMPONENTS_PATH"
} > "$deps/env.sh"
