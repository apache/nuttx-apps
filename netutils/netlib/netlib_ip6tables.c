/****************************************************************************
 * apps/netutils/netlib/netlib_ip6tables.c
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

#include <nuttx/config.h>

#include <sys/socket.h>

#include <nuttx/net/netfilter/ip6_tables.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IP6T_FILL_MATCH(e, match_name) \
  do \
    { \
      strlcpy((e)->match.u.user.name, (match_name), \
              sizeof((e)->match.u.user.name)); \
      (e)->match.u.match_size = offsetof(typeof(*(e)), target) - \
                                offsetof(typeof(*(e)), match); \
    } \
  while(0)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct ip6t_filter_entry_s
{
  struct ip6t_entry entry;

  /* Compatible with ACCEPT/DROP/REJECT target. */

  struct xt_standard_target target;
};

struct ip6t_filter_tcp_entry_s
{
  struct ip6t_entry entry;
  struct xt_entry_match match;
  struct xt_tcp tcp;
  struct xt_standard_target target;
};

struct ip6t_filter_udp_entry_s
{
  struct ip6t_entry entry;
  struct xt_entry_match match;
  struct xt_udp udp;
  struct xt_standard_target target;
};

struct ip6t_filter_icmp_entry_s
{
  struct ip6t_entry entry;
  struct xt_entry_match match;
  struct ip6t_icmp icmp;
  struct xt_standard_target target;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ip6t_entry_by_rulenum
 *
 * Description:
 *   Get entry in repl at rulenum (1 = first) in hook.
 *
 * Input Parameters:
 *   repl       - The config (to set into kernel later).
 *   hook       - The hook of the entry.
 *   rulenum    - The place to get.
 *   allow_last - Whether allow to get last entry (at underflow), may insert
 *                entry just before last entry, but don't delete last entry.
 *
 ****************************************************************************/

static FAR struct ip6t_entry *
netlib_ip6t_entry_by_rulenum(FAR struct ip6t_replace *repl,
                             enum nf_inet_hooks hook, int rulenum,
                             bool allow_last)
{
  FAR struct ip6t_entry *e;
  FAR uint8_t *head = (FAR uint8_t *)repl->entries + repl->hook_entry[hook];
  int size = repl->underflow[hook] - repl->hook_entry[hook];

  ip6t_entry_for_every(e, head, size)
    {
      if (--rulenum <= 0)
        {
          return e;
        }
    }

  return (allow_last && rulenum == 1) ? e : NULL;
}

/****************************************************************************
 * Name: netlib_ip6t_insert_internal
 *
 * Description:
 *   Insert an entry into config at insert_point.
 *
 * Input Parameters:
 *   repl         - The config (to set into kernel later).
 *   entry        - The entry to insert.
 *   hook         - The hook of the entry.
 *   insert_point - The offset to put the entry.
 *
 ****************************************************************************/

static int netlib_ip6t_insert_internal(FAR struct ip6t_replace **replace,
                                       FAR const struct ip6t_entry *entry,
                                       enum nf_inet_hooks hook,
                                       unsigned int insert_point)
{
  FAR struct ip6t_replace *repl = *replace;
  FAR uint8_t *base;
  size_t new_size;

  new_size = sizeof(*repl) + repl->size + entry->next_offset;
  repl = realloc(repl, new_size);
  if (repl == NULL)
    {
      return -ENOMEM;
    }

  /* Insert new entry into entry table. */

  base = (FAR uint8_t *)repl->entries;
  memmove(base + insert_point + entry->next_offset, base + insert_point,
          repl->size - insert_point);
  memcpy(base + insert_point, entry, entry->next_offset);

  /* Adjust metadata. */

  repl->num_entries++;
  repl->size += entry->next_offset;

  /* Adjust hook_entry and underflow. */

  repl->underflow[hook++] += entry->next_offset;
  for (; hook < NF_INET_NUMHOOKS; hook++)
    {
      if (repl->valid_hooks & (1 << hook))
        {
          repl->hook_entry[hook] += entry->next_offset;
          repl->underflow[hook] += entry->next_offset;
        }
    }

  *replace = repl;
  return OK;
}

/****************************************************************************
 * Name: netlib_ip6t_delete_internal
 *
 * Description:
 *   Delete an entry from config.
 *
 * Input Parameters:
 *   repl   - The config (to set into kernel later).
 *   entry  - The entry to remove, should be in repl.
 *   hook   - The hook of the entry.
 *
 ****************************************************************************/

static void netlib_ip6t_delete_internal(FAR struct ip6t_replace *repl,
                                        FAR struct ip6t_entry *entry,
                                        enum nf_inet_hooks hook)
{
  unsigned int delete_len = entry->next_offset;

  /* Adjust metadata. */

  repl->num_entries--;
  repl->size -= delete_len;

  /* Remove entry from entry table. */

  memmove((FAR uint8_t *)entry, (FAR uint8_t *)entry + delete_len,
          repl->size - ((uintptr_t)entry - (uintptr_t)repl->entries));

  /* Adjust hook_entry and underflow. */

  repl->underflow[hook++] -= delete_len;
  for (; hook < NF_INET_NUMHOOKS; hook++)
    {
      if (repl->valid_hooks & (1 << hook))
        {
          repl->hook_entry[hook] -= delete_len;
          repl->underflow[hook] -= delete_len;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ip6t_prepare
 *
 * Description:
 *   Read current config from kernel space.
 *
 * Input Parameters:
 *   table  - The table name to read from.
 *
 * Returned Value:
 *   The pointer to the config, or NULL if failed.
 *   Caller must free it after use.
 *
 ****************************************************************************/

FAR struct ip6t_replace *netlib_ip6t_prepare(FAR const char *table)
{
  struct ip6t_getinfo info;
  FAR struct ip6t_get_entries *entries;
  FAR struct ip6t_replace *repl = NULL;
  socklen_t len;
  int sockfd;

  if (table == NULL)
    {
      return NULL;
    }

  sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sockfd < 0)
    {
      fprintf(stderr, "Failed to create socket %d!\n", errno);
      return NULL;
    }

  strlcpy(info.name, table, sizeof(info.name));
  len = sizeof(info);

  if (getsockopt(sockfd, IPPROTO_IPV6, IP6T_SO_GET_INFO, &info, &len) < 0)
    {
      fprintf(stderr, "Failed to get info for table %s %d!\n", table, errno);
      goto errout;
    }

  len = sizeof(*entries) + info.size;
  entries = malloc(len);
  if (entries == NULL)
    {
      goto errout;
    }

  strlcpy(entries->name, table, sizeof(entries->name));
  entries->size = info.size;
  if (getsockopt(sockfd, IPPROTO_IPV6, IP6T_SO_GET_ENTRIES, entries, &len)
      < 0)
    {
      fprintf(stderr, "Failed to get entries for table %s %d!\n",
              table, errno);
      goto errout_with_entries;
    }

  repl = malloc(sizeof(*repl) + info.size);
  if (repl == NULL)
    {
      goto errout_with_entries;
    }

  strlcpy(repl->name, table, sizeof(repl->name));

  repl->valid_hooks  = info.valid_hooks;
  repl->num_entries  = info.num_entries;
  repl->size         = info.size;
  repl->num_counters = 0;
  repl->counters     = NULL;

  memcpy(repl->hook_entry, info.hook_entry, sizeof(repl->hook_entry));
  memcpy(repl->underflow, info.underflow, sizeof(repl->underflow));
  memcpy(repl->entries, entries->entrytable, info.size);

errout_with_entries:
  free(entries);

errout:
  close(sockfd);
  return repl;
}

/****************************************************************************
 * Name: netlib_ip6t_commit
 *
 * Description:
 *   Set config into kernel space.
 *
 * Input Parameters:
 *   repl  - The config to commit.
 *
 ****************************************************************************/

int netlib_ip6t_commit(FAR const struct ip6t_replace *repl)
{
  int ret;
  int sockfd;

  if (repl == NULL)
    {
      return -EINVAL;
    }

  sockfd = socket(NET_SOCK_FAMILY, NET_SOCK_TYPE, NET_SOCK_PROTOCOL);
  if (sockfd < 0)
    {
      fprintf(stderr, "Failed to create socket %d!\n", errno);
      return -errno;
    }

  ret = setsockopt(sockfd, IPPROTO_IPV6, IP6T_SO_SET_REPLACE, repl,
                   sizeof(*repl) + repl->size);
  if (ret < 0)
    {
      ret = -errno;
      fprintf(stderr, "Failed to commit %d!\n", ret);
    }

  close(sockfd);
  return ret;
}

/****************************************************************************
 * Name: netlib_ip6t_flush
 *
 * Description:
 *   Flush all config in the table.
 *
 * Input Parameters:
 *   table  - The table name to flush.
 *   hook   - The hook to flush, NF_INET_NUMHOOKS for all.
 *
 ****************************************************************************/

int netlib_ip6t_flush(FAR const char *table, enum nf_inet_hooks hook)
{
  FAR struct ip6t_replace *repl = netlib_ip6t_prepare(table);
  unsigned int cur_hook;
  int ret;

  if (repl == NULL)
    {
      fprintf(stderr, "Failed to read table %s from kernel!\n", table);
      return -EIO;
    }

  if (hook != NF_INET_NUMHOOKS && (repl->valid_hooks & (1 << hook)) == 0)
    {
      fprintf(stderr, "Invalid hook number %d for table %s!\n", hook, table);
      ret = -EINVAL;
      goto errout;
    }

  for (cur_hook = 0; cur_hook < NF_INET_NUMHOOKS; cur_hook++)
    {
      if ((repl->valid_hooks & (1 << cur_hook)) != 0 &&
          (hook == NF_INET_NUMHOOKS || hook == cur_hook))
        {
          /* Remove all user entries in current hook. */

          while (repl->underflow[cur_hook] > repl->hook_entry[cur_hook])
            {
              ret = netlib_ip6t_delete(repl, NULL, cur_hook, 1);
              if (ret < 0)
                {
                  goto errout;
                }
            }
        }
    }

  ret = netlib_ip6t_commit(repl);

errout:
  free(repl);
  return ret;
}

/****************************************************************************
 * Name: netlib_ip6t_policy
 *
 * Description:
 *   Set policy for the table.  It's a common operation, but may only take
 *   effect on filter-related tables.
 *
 * Input Parameters:
 *   table   - The table name to set policy.
 *   hook    - The hook to set policy.
 *   verdict - The verdict to set.
 *
 ****************************************************************************/

int netlib_ip6t_policy(FAR const char *table, enum nf_inet_hooks hook,
                       int verdict)
{
  FAR struct ip6t_replace *repl = netlib_ip6t_prepare(table);
  FAR struct ip6t_entry *entry;
  FAR struct xt_standard_target *target;
  int ret;

  if (repl == NULL)
    {
      fprintf(stderr, "Failed to read table %s from kernel!\n", table);
      return -EIO;
    }

  if ((repl->valid_hooks & (1 << hook)) == 0)
    {
      fprintf(stderr, "Invalid hook number %d for table %s!\n", hook, table);
      ret = -EINVAL;
      goto errout;
    }

  /* The underflow entry is the default policy of the chain. */

  entry  = (FAR struct ip6t_entry *)((uintptr_t)repl->entries +
                                                repl->underflow[hook]);
  target = (FAR struct xt_standard_target *)IP6T_TARGET(entry);
  if (strcmp(target->target.u.user.name, XT_STANDARD_TARGET) != 0)
    {
      fprintf(stderr, "Wrong target %s!\n", target->target.u.user.name);
      ret = -EINVAL;
      goto errout;
    }

  target->verdict = verdict;

  ret = netlib_ip6t_commit(repl);

errout:
  free(repl);
  return ret;
}

/****************************************************************************
 * Name: netlib_ip6t_append
 *
 * Description:
 *   Append an entry into config, will be put to as last config of the chain
 * corresponding to hook.
 *
 * Input Parameters:
 *   repl   - The config (to set into kernel later).
 *   entry  - The entry to append.
 *   hook   - The hook of the entry.
 *
 ****************************************************************************/

int netlib_ip6t_append(FAR struct ip6t_replace **repl,
                       FAR const struct ip6t_entry *entry,
                       enum nf_inet_hooks hook)
{
  if (repl == NULL || *repl == NULL || entry == NULL)
    {
      return -EINVAL;
    }

  if (((*repl)->valid_hooks & (1 << hook)) == 0)
    {
      fprintf(stderr, "Not valid hook %d for this table!\n", hook);
      return -EINVAL;
    }

  return netlib_ip6t_insert_internal(repl, entry, hook,
                                     (*repl)->underflow[hook]);
}

/****************************************************************************
 * Name: netlib_ip6t_insert
 *
 * Description:
 *   Insert an entry into config, will be put to as first config of the chain
 * corresponding to hook.
 *
 * Input Parameters:
 *   repl    - The config (to set into kernel later).
 *   entry   - The entry to insert.
 *   hook    - The hook of the entry.
 *   rulenum - The place to insert, 1 = first.
 *
 ****************************************************************************/

int netlib_ip6t_insert(FAR struct ip6t_replace **repl,
                       FAR const struct ip6t_entry *entry,
                       enum nf_inet_hooks hook, int rulenum)
{
  FAR struct ip6t_entry *e;

  if (repl == NULL || *repl == NULL || entry == NULL || rulenum <= 0)
    {
      fprintf(stderr, "Not valid param %p, %p, rulenum %d!\n",
              repl, entry, rulenum);
      return -EINVAL;
    }

  if (((*repl)->valid_hooks & (1 << hook)) == 0)
    {
      fprintf(stderr, "Not valid hook %d for this table!\n", hook);
      return -EINVAL;
    }

  e = netlib_ip6t_entry_by_rulenum(*repl, hook, rulenum, true);
  if (e == NULL)
    {
      fprintf(stderr, "Rulenum %d too big!\n", rulenum);
      return -EINVAL;
    }

  return netlib_ip6t_insert_internal(repl, entry, hook,
                                (uintptr_t)e - (uintptr_t)(*repl)->entries);
}

/****************************************************************************
 * Name: netlib_ip6t_delete
 *
 * Description:
 *   Delete an entry from config.
 *
 * Input Parameters:
 *   repl    - The config (to set into kernel later).
 *   entry   - The entry to delete, choose either entry or rulenum.
 *   hook    - The hook of the entry.
 *   rulenum - The place to delete, 1 = first, set entry to NULL to use this.
 *
 ****************************************************************************/

int netlib_ip6t_delete(FAR struct ip6t_replace *repl,
                       FAR const struct ip6t_entry *entry,
                       enum nf_inet_hooks hook, int rulenum)
{
  FAR struct ip6t_entry *e;
  FAR uint8_t *head;
  int size;

  if (repl == NULL || (entry == NULL && rulenum <= 0))
    {
      fprintf(stderr, "Not valid param %p, %p, rulenum %d!\n",
              repl, entry, rulenum);
      return -EINVAL;
    }

  if ((repl->valid_hooks & (1 << hook)) == 0)
    {
      fprintf(stderr, "Not valid hook %d for this table!\n", hook);
      return -EINVAL;
    }

  if (entry == NULL) /* Use rulenum instead. */
    {
      e = netlib_ip6t_entry_by_rulenum(repl, hook, rulenum, false);
      if (e == NULL)
        {
          fprintf(stderr, "Rulenum %d too big!\n", rulenum);
          return -EINVAL;
        }

      netlib_ip6t_delete_internal(repl, e, hook);
      return OK;
    }

  head = (FAR uint8_t *)repl->entries + repl->hook_entry[hook];
  size = repl->underflow[hook] - repl->hook_entry[hook];
  ip6t_entry_for_every(e, head, size)
    {
      if (e->next_offset == entry->next_offset &&
          e->target_offset == entry->target_offset &&
          memcmp(&e->ipv6, &entry->ipv6, sizeof(struct ip6t_ip6)) == 0 &&
          memcmp(&e->elems, &entry->elems,
                 e->next_offset - offsetof(struct ip6t_entry, elems)) == 0)
        {
          netlib_ip6t_delete_internal(repl, e, hook);
          return OK;
        }
    }

  return -ENOENT;
}

/****************************************************************************
 * Name: netlib_ip6t_fillifname
 *
 * Description:
 *   Fill inifname and outifname into entry.
 *
 * Input Parameters:
 *   entry     - The entry to fill.
 *   inifname  - The input device name, NULL for no change.
 *   outifname - The output device name, NULL for no change.
 *
 ****************************************************************************/

int netlib_ip6t_fillifname(FAR struct ip6t_entry *entry,
                           FAR const char *inifname,
                           FAR const char *outifname)
{
  size_t len;

  if (entry == NULL)
    {
      return -EINVAL;
    }

  if (inifname != NULL)
    {
      len = strlen(inifname);
      if (len + 1 > IFNAMSIZ)
        {
          fprintf(stderr, "Too long inifname %s!\n", inifname);
          return -EINVAL;
        }

      strlcpy(entry->ipv6.iniface, inifname, sizeof(entry->ipv6.iniface));
      memset(entry->ipv6.iniface_mask, 0xff, len + 1);
    }

  if (outifname != NULL)
    {
      len = strlen(outifname);
      if (len + 1 > IFNAMSIZ)
        {
          fprintf(stderr, "Too long outifname %s!\n", outifname);
          return -EINVAL;
        }

      strlcpy(entry->ipv6.outiface, outifname, sizeof(entry->ipv6.outiface));
      memset(entry->ipv6.outiface_mask, 0xff, len + 1);
    }

  return OK;
}

/****************************************************************************
 * Name: netlib_ip6t_filter_entry
 *
 * Description:
 *   Alloc an entry with filter target.
 *
 * Input Parameters:
 *   target      - The target name to apply.
 *   verdict     - The verdict to set, compatible with reject target's code
 *   match_proto - The protocol match type in the entry, 0 for no match.
 *
 * Returned Value:
 *   The pointer to the entry, or NULL if failed.
 *   Caller must free it after use.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPFILTER
FAR struct ip6t_entry *netlib_ip6t_filter_entry(FAR const char *target,
                                                int verdict,
                                                uint8_t match_proto)
{
  if (target == NULL)
    {
      fprintf(stderr, "Empty target!\n");
      return NULL;
    }

  switch (match_proto)
    {
      case 0:
        {
          FAR struct ip6t_filter_entry_s *entry = zalloc(sizeof(*entry));
          if (entry == NULL)
            {
              return NULL;
            }

          IP6T_FILL_ENTRY(entry, target);
          entry->target.verdict = verdict;
          return &entry->entry;
        }

      case IPPROTO_TCP:
        {
          FAR struct ip6t_filter_tcp_entry_s *entry = zalloc(sizeof(*entry));
          if (entry == NULL)
            {
              return NULL;
            }

          IP6T_FILL_ENTRY(entry, target);
          IP6T_FILL_MATCH(entry, XT_MATCH_NAME_TCP);
          entry->target.verdict = verdict;
          return &entry->entry;
        }

      case IPPROTO_UDP:
        {
          FAR struct ip6t_filter_udp_entry_s *entry = zalloc(sizeof(*entry));
          if (entry == NULL)
            {
              return NULL;
            }

          IP6T_FILL_ENTRY(entry, target);
          IP6T_FILL_MATCH(entry, XT_MATCH_NAME_UDP);
          entry->target.verdict = verdict;
          return &entry->entry;
        }

      case IPPROTO_ICMP6:
        {
          FAR struct ip6t_filter_icmp_entry_s *entry =
                                                      zalloc(sizeof(*entry));
          if (entry == NULL)
            {
              return NULL;
            }

          IP6T_FILL_ENTRY(entry, target);
          IP6T_FILL_MATCH(entry, XT_MATCH_NAME_ICMP6);
          entry->target.verdict = verdict;
          return &entry->entry;
        }

      default:
        return NULL;
    }
}
#endif
