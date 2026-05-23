############################################################################
# apps/system/microros/Makefile
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

include $(APPDIR)/Make.defs

MICROROS_DIR     := $(APPDIR)/system/microros
MICROROS_LIB_DIR := $(MICROROS_DIR)/micro_ros_lib
MICROROS_DISTRO  := $(patsubst "%",%,$(CONFIG_MICROROS_DISTRO))
MICROROS_AR      := $(CROSSDEV)ar

MAX_NODES   := $(CONFIG_MICROROS_MAX_NODES)
MAX_PUB     := $(CONFIG_MICROROS_MAX_PUBLISHERS)
MAX_SUB     := $(CONFIG_MICROROS_MAX_SUBSCRIPTIONS)
MAX_SVC     := $(CONFIG_MICROROS_MAX_SERVICES)
MAX_CLIENTS := $(CONFIG_MICROROS_MAX_CLIENTS)

# Escape for sed: forward slashes and strip quotes
ESCAPED_CC     := $(subst /,\/,$(CC))
ESCAPED_CXX    := $(subst /,\/,$(CXX))
ESCAPED_TOPDIR := $(subst /,\/,$(TOPDIR))
ESCAPED_APPDIR := $(subst /,\/,$(APPDIR))
ESCAPED_LIBDIR := $(subst /,\/,$(MICROROS_LIB_DIR))
ESCAPED_CFLAGS   := $(subst /,\/,$(subst ",,$(CFLAGS)))
ESCAPED_CXXFLAGS := $(subst /,\/,$(subst ",,$(CXXFLAGS)))

MENUDESC = "micro-ROS Library"

# Architecture for CMake toolchain
ifeq ($(CONFIG_ARCH_ARM),y)
MICROROS_ARCH := arm
else ifeq ($(CONFIG_ARCH_ARM64),y)
MICROROS_ARCH := arm64
else ifeq ($(CONFIG_ARCH_RISCV),y)
MICROROS_ARCH := riscv
else ifeq ($(CONFIG_ARCH_XTENSA),y)
MICROROS_ARCH := xtensa
else
MICROROS_ARCH := generic
endif

# Wire libmicroros.a into the build when the context target fires
context:: $(MICROROS_DIR)/libmicroros.a

# ---------------------------------------------------------------------------
# Step 1 – Build the ament/colcon dev tools (host-side)
# ---------------------------------------------------------------------------

$(MICROROS_LIB_DIR)/micro_ros_dev/install:
	rm -rf $(MICROROS_LIB_DIR)/micro_ros_dev
	mkdir -p $(MICROROS_LIB_DIR)/micro_ros_dev/src
	cd $(MICROROS_LIB_DIR)/micro_ros_dev && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ament/ament_cmake src/ament_cmake && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ament/ament_lint src/ament_lint && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ament/ament_package src/ament_package && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ament/googletest src/googletest && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/ament_cmake_ros src/ament_cmake_ros && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ament/ament_index src/ament_index && \
	  touch src/ament_cmake_ros/rmw_test_fixture_implementation/COLCON_IGNORE 2>/dev/null || true && \
	  touch src/ament_cmake_ros/rmw_test_fixture/COLCON_IGNORE 2>/dev/null || true && \
	  colcon build --cmake-args -DBUILD_TESTING=OFF

# ---------------------------------------------------------------------------
# Step 2 – Clone all micro-ROS source packages
# ---------------------------------------------------------------------------

$(MICROROS_LIB_DIR)/micro_ros_src/src: $(MICROROS_LIB_DIR)/micro_ros_dev/install
	rm -rf $(MICROROS_LIB_DIR)/micro_ros_src
	mkdir -p $(MICROROS_LIB_DIR)/micro_ros_src/src
	cd $(MICROROS_LIB_DIR)/micro_ros_src && \
	  git clone -b ros2 --depth 1 \
	    https://github.com/eProsima/micro-CDR src/micro-CDR && \
	  git clone -b ros2 --depth 1 \
	    https://github.com/eProsima/Micro-XRCE-DDS-Client src/Micro-XRCE-DDS-Client && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/micro-ROS/rmw-microxrcedds src/rmw_microxrcedds && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/micro-ROS/rcl src/rcl && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rclc src/rclc && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/micro-ROS/rcutils src/rcutils && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rosidl src/rosidl && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rosidl_defaults src/rosidl_defaults && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rosidl_dynamic_typesupport src/rosidl_dynamic_typesupport && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rosidl_core src/rosidl_core && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rmw src/rmw && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rmw_implementation src/rmw_implementation && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rcl_interfaces src/rcl_interfaces && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/common_interfaces src/common_interfaces && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/rcl_logging src/rcl_logging && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/micro-ROS/micro_ros_msgs src/micro_ros_msgs && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/micro-ROS/rosidl_typesupport src/rosidl_typesupport && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/micro-ROS/rosidl_typesupport_microxrcedds src/rosidl_typesupport_microxrcedds && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/micro-ROS/micro_ros_utilities src/micro_ros_utilities && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/unique_identifier_msgs src/unique_identifier_msgs && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/test_interface_files src/test_interface_files && \
	  git clone -b $(MICROROS_DISTRO) --depth 1 \
	    https://github.com/ros2/ros2_tracing src/ros2_tracing && \
	  touch src/ros2_tracing/test_tracetools/COLCON_IGNORE && \
	  touch src/ros2_tracing/lttngpy/COLCON_IGNORE && \
	  touch src/rmw/rmw_security_common/COLCON_IGNORE && \
	  touch src/rcl_logging/rcl_logging_spdlog/COLCON_IGNORE && \
	  touch src/rclc/rclc_examples/COLCON_IGNORE && \
	  touch src/rcl/rcl_yaml_param_parser/COLCON_IGNORE && \
	  touch src/rosidl/rosidl_typesupport_introspection_cpp/COLCON_IGNORE && \
	  touch src/rcl_interfaces/test_msgs/COLCON_IGNORE && \
	  touch src/common_interfaces/actionlib_msgs/COLCON_IGNORE && \
	  touch src/common_interfaces/std_srvs/COLCON_IGNORE && \
	  patch -p1 --forward --ignore-whitespace \
	    -d src < $(MICROROS_LIB_DIR)/patches/0001-dir-typedef-nuttx-guard.patch || true && \
	  patch -p1 --forward --ignore-whitespace \
	    -d src < $(MICROROS_LIB_DIR)/patches/0002-rcutils-process-nuttx-compat.patch || true && \
	  patch -p1 --forward --ignore-whitespace \
	    -d src < $(MICROROS_LIB_DIR)/patches/0003-rcutils-strcasecmp-include-strings-h.patch || true

# ---------------------------------------------------------------------------
# Step 3 – Generate toolchain.cmake from template
# ---------------------------------------------------------------------------

$(MICROROS_LIB_DIR)/toolchain.cmake: $(MICROROS_LIB_DIR)/toolchain.cmake.in
	sed \
	  -e 's/@CMAKE_C_COMPILER@/$(ESCAPED_CC)/g' \
	  -e 's/@CMAKE_CXX_COMPILER@/$(ESCAPED_CXX)/g' \
	  -e 's/@ARCH_C_FLAGS@/$(ESCAPED_CFLAGS)/g' \
	  -e 's/@ARCH_CPP_FLAGS@/$(ESCAPED_CXXFLAGS)/g' \
	  -e 's/@NUTTX_TOPDIR@/$(ESCAPED_TOPDIR)/g' \
	  -e 's/@NUTTX_APPDIR@/$(ESCAPED_APPDIR)/g' \
	  -e 's/@INCLUDE_OVR_DIR@/$(ESCAPED_LIBDIR)/g' \
	  -e 's/@CMAKE_SYSTEM_PROCESSOR@/$(MICROROS_ARCH)/g' \
	  $< > $@

# ---------------------------------------------------------------------------
# Step 4 – Generate colcon.meta from template
# ---------------------------------------------------------------------------

$(MICROROS_LIB_DIR)/colcon.meta: $(MICROROS_LIB_DIR)/colcon.meta.in
	sed \
	  -e 's/@MAX_NODES@/$(MAX_NODES)/g' \
	  -e 's/@MAX_PUBLISHERS@/$(MAX_PUB)/g' \
	  -e 's/@MAX_SUBSCRIPTIONS@/$(MAX_SUB)/g' \
	  -e 's/@MAX_SERVICES@/$(MAX_SVC)/g' \
	  -e 's/@MAX_CLIENTS@/$(MAX_CLIENTS)/g' \
	  $< > $@

# ---------------------------------------------------------------------------
# Step 5 – Cross-build micro-ROS packages with colcon
# ---------------------------------------------------------------------------

$(MICROROS_LIB_DIR)/micro_ros_src/install: \
    $(MICROROS_LIB_DIR)/toolchain.cmake \
    $(MICROROS_LIB_DIR)/colcon.meta \
    $(MICROROS_LIB_DIR)/micro_ros_src/src
	cd $(MICROROS_LIB_DIR)/micro_ros_src && \
	  unset AMENT_PREFIX_PATH && \
	  . ../micro_ros_dev/install/local_setup.sh && \
	  colcon build \
	    --merge-install \
	    "--packages-ignore-regex=(.*_cpp$$|.*_py$$|test_rmw_implementation|rclc_examples)" \
	    --metas $(MICROROS_LIB_DIR)/colcon.meta \
	    --cmake-args \
	      --no-warn-unused-cli \
	      -DCMAKE_POSITION_INDEPENDENT_CODE=OFF \
	      -DTHIRDPARTY=ON \
	      -DBUILD_SHARED_LIBS=OFF \
	      -DBUILD_TESTING=OFF \
	      -DCMAKE_BUILD_TYPE=Release \
	      "-DCMAKE_TOOLCHAIN_FILE=$(MICROROS_LIB_DIR)/toolchain.cmake" \
	      -DCMAKE_VERBOSE_MAKEFILE=OFF

# ---------------------------------------------------------------------------
# Step 6 – Merge all .a files into a single libmicroros.a
# ---------------------------------------------------------------------------

$(MICROROS_DIR)/libmicroros.a: $(MICROROS_LIB_DIR)/micro_ros_src/install
	mkdir -p $(MICROROS_LIB_DIR)/libmicroros_obj
	cd $(MICROROS_LIB_DIR)/libmicroros_obj && \
	  for file in $$(find $(MICROROS_LIB_DIR)/micro_ros_src/install/lib/ -name '*.a'); do \
	    folder=$$(basename $$file .a); \
	    mkdir -p $$folder && cd $$folder; \
	    $(MICROROS_AR) x $$file; \
	    for f in *; do \
	      [ -f "$$f" ] && mv "$$f" "../$${folder}-$${f}" 2>/dev/null || true; \
	    done; \
	    cd ..; \
	    rm -rf $$folder; \
	  done && \
	  $(MICROROS_AR) rc libmicroros.a $$(find . -maxdepth 1 -type f ! -name 'libmicroros.a') && \
	  cp libmicroros.a $(MICROROS_DIR)/
	cd $(MICROROS_LIB_DIR) && rm -rf libmicroros_obj
	cp -R $(MICROROS_LIB_DIR)/micro_ros_src/install/include $(MICROROS_DIR)/include

# ---------------------------------------------------------------------------
# Cleanup targets
# ---------------------------------------------------------------------------

clean::
	$(call DELFILE,$(MICROROS_DIR)/libmicroros.a)
	$(call DELDIR,$(MICROROS_DIR)/include)
	$(call DELFILE,$(MICROROS_LIB_DIR)/toolchain.cmake)
	$(call DELFILE,$(MICROROS_LIB_DIR)/colcon.meta)

distclean:: clean
	$(call DELDIR,$(MICROROS_LIB_DIR)/micro_ros_src)
	$(call DELDIR,$(MICROROS_LIB_DIR)/micro_ros_dev)

include $(APPDIR)/Application.mk
