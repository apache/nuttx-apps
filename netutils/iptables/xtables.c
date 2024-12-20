/****************************************************************************
 * apps/netutils/iptables/xtables.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <errno.h>

#include <nuttx/net/netfilter/ip_tables.h>
#include <nuttx/net/netfilter/ip6_tables.h>

#include <netutils/xtables.h>

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* Keep track of fully registered external matches/targets: linked lists. */

FAR struct xtables_match *xtables_matches;
FAR struct xtables_target *xtables_targets;
FAR struct xtables_globals *xt_params;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int xtables_insmod(FAR const char *modname, FAR const char *modprobe,
                   bool quiet)
{
  return -ENOSYS;
}

int xtables_init_all(FAR struct xtables_globals *xtp, uint8_t nfproto)
{
  return -ENOSYS;
}

uint16_t xtables_parse_protocol(FAR const char *s)
{
  return -ENOSYS;
}

void xtables_option_tpcall(unsigned int c, FAR char **argv, bool invert,
                           FAR struct xtables_target *t, FAR void *fw)
{
}

void xtables_option_mpcall(unsigned int c, FAR char **argv, bool invert,
                           FAR struct xtables_match *m, FAR void *fw)
{
}

void xtables_option_tfcall(FAR struct xtables_target *t)
{
}

void xtables_option_mfcall(FAR struct xtables_match *m)
{
}

FAR struct option *xtables_options_xfrm(FAR struct option *orig_opts,
                                    FAR struct option *oldopts,
                                    FAR const struct xt_option_entry *entry,
                                    FAR unsigned int *offset)
{
  return NULL;
}

FAR struct option *xtables_merge_options(FAR struct option *origopts,
                                         FAR struct option *oldopts,
                                         FAR const struct option *newopts,
                                         FAR unsigned int *option_offset)
{
  return NULL;
}

FAR struct xtables_match *xtables_find_match(FAR const char *name,
                                      enum xtables_tryload tryload,
                                      FAR struct xtables_rule_match **match)
{
  return NULL;
}

FAR struct xtables_target *xtables_find_target(FAR const char *name,
                                               enum xtables_tryload tryload)
{
  return NULL;
}

int xtables_compatible_revision(FAR const char *name, uint8_t revision,
                                int opt)
{
  return -ENOSYS;
}
