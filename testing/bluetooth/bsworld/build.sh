#!/usr/bin/env bash
############################################################################
# apps/testing/bluetooth/bsworld/build.sh
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
Usage: build.sh NUTTX_DIR APPS_DIR DEPENDENCY_DIR WORK_DIR RESULT_DIR

Build BTSak and NimBLE firmware in separate source copies, preserving the
original configurations and build outputs. Run prepare.sh first.
BUILD_JOBS defaults to nproc. Run pytest separately as described in README.md.
EOF
}

if [ "${1:-}" = -h ] || [ "${1:-}" = --help ]; then
  usage
  exit 0
fi
if [ "$#" -ne 5 ]; then
  usage
  exit 2
fi

copy_source()
{
  local source_dir=$1 target_dir=$2
  mkdir -p "$target_dir"
  # Do not copy old configurations, objects or downloaded package trees.
  git -C "$source_dir" ls-files --cached --others --exclude-standard -z |
    rsync -a --from0 --files-from=- "$source_dir/" "$target_dir/"
}

nuttx=$(realpath "$1")
apps=$(realpath "$2")
deps=$(realpath "$3")
mkdir -p "$4" "$5"
work=$(realpath "$4")
results=$(realpath "$5")
# shellcheck disable=SC1091
source "$deps/env.sh"
export PYTHONDONTWRITEBYTECODE=1
test -x "$BSIM_OUT_PATH/bin/bs_2G4_phy_v1"
mkdir -p "$results/firmware"

{
  printf 'NuttX: '; git -C "$nuttx" rev-parse HEAD
  printf 'apps: '; git -C "$apps" rev-parse HEAD
  printf 'BSWorld: '; cat "$deps/bsworld-revision"
  gcc --version
  python --version
  git -C "$nuttx" diff HEAD
  git -C "$apps" diff HEAD
} > "$results/environment.txt"
cp "$deps/python-packages.txt" "$deps/babblesim-manifest.xml" "$results/"

for config in bthcisock nimble; do
  build=$(mktemp -d "$work/$config.XXXXXX")
  copy_source "$nuttx" "$build/nuttx"
  copy_source "$apps" "$build/apps"
  (
    cd "$build/nuttx"
    ./tools/configure.sh -l -a ../apps "sim:$config"
    # Kconfig applies the last assignment when loading a fragment.
    cat "$build/apps/testing/bluetooth/bsworld/configs/$config.conf" >> .config
    make olddefconfig
    make -j"${BUILD_JOBS:-$(nproc)}"
  ) 2>&1 | tee "$results/build-$config.log"
  binary_name=$config
  if [ "$config" = bthcisock ]; then binary_name=btsak; fi
  cp "$build/nuttx/nuttx" "$results/firmware/$binary_name"
  cp "$build/nuttx/.config" "$results/firmware/$binary_name.config"
done
