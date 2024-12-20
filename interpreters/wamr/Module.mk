############################################################################
# apps/interpreters/wamr/Module.mk
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

WAMR_MODULE_REGISTRY = $(APPDIR)$(DELIM)interpreters$(DELIM)wamr$(DELIM)registry

define WAMR_MODULE_REGISTER
	$(Q) echo Register WAMR Module: $1
	$(Q) echo "$2," > "$(WAMR_MODULE_REGISTRY)$(DELIM)$1.bdat"
	$(Q) echo "bool $2(void);" > "$(WAMR_MODULE_REGISTRY)$(DELIM)$1.pdat"
	$(Q) touch "$(WAMR_MODULE_REGISTRY)$(DELIM).updated"
endef

ifneq ($(WAMR_MODULE_NAME),)
WAMR_MODULE_LIST := $(addprefix $(WAMR_MODULE_REGISTRY)$(DELIM),$(addsuffix .bdat,$(WAMR_MODULE_NAME)))
$(WAMR_MODULE_LIST): $(DEPCONFIG) Makefile
	$(call WAMR_MODULE_REGISTER,$(firstword $(WAMR_MODULE_NAME)),$(firstword wamr_module_$(WAMR_MODULE_NAME)_register))
	$(eval WAMR_MODULE_NAME=$(filter-out $(firstword $(WAMR_MODULE_NAME)),$(WAMR_MODULE_NAME)))

register:: $(WAMR_MODULE_LIST)
endif
