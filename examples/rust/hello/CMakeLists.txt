# ##############################################################################
# apps/examples/rust/hello/CMakeLists.txt
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

if(CONFIG_EXAMPLES_HELLO_RUST_CARGO)

  # Build the Rust crate using nuttx_add_rust
  nuttx_add_rust(CRATE_NAME hello CRATE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

  nuttx_add_application(
    NAME ${CONFIG_EXAMPLES_HELLO_RUST_CARGO_PROGNAME} STACKSIZE
    ${CONFIG_EXAMPLES_HELLO_STACKSIZE} PRIORITY
    ${CONFIG_EXAMPLES_HELLO_PRIORITY})

endif() # CONFIG_EXAMPLES_HELLO_RUST_CARGO
