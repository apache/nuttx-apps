/****************************************************************************
 * apps/netutils/netlib/netlib_iptables.c
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

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include <nuttx/net/netconfig.h>
#include <nuttx/net/netfilter/ip_tables.h>
#include <nuttx/net/netfilter/netfilter.h>
#include <nuttx/net/netfilter/nf_nat.h>

#include "netutils/netlib.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Following struct represents the layout of an entry with masquerade
 * target (without matches). Mainly used to simplify entry creation.
 */

struct ipt_masquerade_entry_s
{
  struct ipt_entry entry;
  struct /* compatible with IPT_FILL_ENTRY and standard/error target */
    {
      struct xt_entry_target target;
      struct nf_nat_ipv4_multi_range_compat cfg;
    } target;
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: netlib_ipt_entry_by_rulenum
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

static FAR struct ipt_entry *
netlib_ipt_entry_by_rulenum(FAR struct ipt_replace *repl,
                            enum nf_inet_hooks hook, int rulenum,
                            bool allow_last)
{
  FAR struct ipt_entry *e;
  FAR uint8_t *head = (FAR uint8_t *)repl->entries + repl->hook_entry[hook];
  int size = repl->underflow[hook] - repl->hook_entry[hook];

  ipt_entry_for_every(e, head, size)
    {
      if (--rulenum <= 0)
        {
          return e;
        }
    }

  return (allow_last && rulenum == 1) ? e : NULL;
}

/****************************************************************************
 * Name: netlib_ipt_insert_internal
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

static int netlib_ipt_insert_internal(FAR struct ipt_replace **replace,
                                      FAR const struct ipt_entry *entry,
                                      enum nf_inet_hooks hook,
                                      unsigned int insert_point)
{
  FAR struct ipt_replace *repl = *replace;
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
 * Name: netlib_ipt_delete_internal
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

static void netlib_ipt_delete_internal(FAR struct ipt_replace *repl,
                                       FAR struct ipt_entry *entry,
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
 * Name: netlib_ipt_prepare
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

FAR struct ipt_replace *netlib_ipt_prepare(FAR const char *table)
{
  struct ipt_getinfo info;
  FAR struct ipt_get_entries *entries;
  FAR struct ipt_replace *repl = NULL;
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

  if (getsockopt(sockfd, IPPROTO_IP, IPT_SO_GET_INFO, &info, &len) < 0)
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
  if (getsockopt(sockfd, IPPROTO_IP, IPT_SO_GET_ENTRIES, entries, &len) < 0)
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
 * Name: netlib_ipt_commit
 *
 * Description:
 *   Set config into kernel space.
 *
 * Input Parameters:
 *   repl  - The config to commit.
 *
 ****************************************************************************/

int netlib_ipt_commit(FAR const struct ipt_replace *repl)
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

  ret = setsockopt(sockfd, IPPROTO_IP, IPT_SO_SET_REPLACE, repl,
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
 * Name: netlib_ipt_flush
 *
 * Description:
 *   Flush all config in the table.
 *
 * Input Parameters:
 *   table  - The table name to flush.
 *   hook   - The hook to flush, NF_INET_NUMHOOKS for all.
 *
 ****************************************************************************/

int netlib_ipt_flush(FAR const char *table, enum nf_inet_hooks hook)
{
  FAR struct ipt_replace *repl = netlib_ipt_prepare(table);
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
      return -EINVAL;
    }

  for (cur_hook = 0; cur_hook < NF_INET_NUMHOOKS; cur_hook++)
    {
      if ((repl->valid_hooks & (1 << cur_hook)) != 0 &&
          (hook == NF_INET_NUMHOOKS || hook == cur_hook))
        {
          /* Remove all user entries in current hook. */

          while (repl->underflow[cur_hook] > repl->hook_entry[cur_hook])
            {
              ret = netlib_ipt_delete(repl, NULL, cur_hook, 1);
              if (ret < 0)
                {
                  goto errout;
                }
            }
        }
    }

  ret = netlib_ipt_commit(repl);

errout:
  free(repl);
  return ret;
}

/****************************************************************************
 * Name: netlib_ipt_append
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

int netlib_ipt_append(FAR struct ipt_replace **repl,
                      FAR const struct ipt_entry *entry,
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

  return netlib_ipt_insert_internal(repl, entry, hook,
                                    (*repl)->underflow[hook]);
}

/****************************************************************************
 * Name: netlib_ipt_insert
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

int netlib_ipt_insert(FAR struct ipt_replace **repl,
                      FAR const struct ipt_entry *entry,
                      enum nf_inet_hooks hook, int rulenum)
{
  FAR struct ipt_entry *e;

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

  e = netlib_ipt_entry_by_rulenum(*repl, hook, rulenum, true);
  if (e == NULL)
    {
      fprintf(stderr, "Rulenum %d too big!\n", rulenum);
      return -EINVAL;
    }

  return netlib_ipt_insert_internal(repl, entry, hook,
                                (uintptr_t)e - (uintptr_t)(*repl)->entries);
}

/****************************************************************************
 * Name: netlib_ipt_delete
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

int netlib_ipt_delete(FAR struct ipt_replace *repl,
                      FAR const struct ipt_entry *entry,
                      enum nf_inet_hooks hook, int rulenum)
{
  FAR struct ipt_entry *e;
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
      e = netlib_ipt_entry_by_rulenum(repl, hook, rulenum, false);
      if (e == NULL)
        {
          fprintf(stderr, "Rulenum %d too big!\n", rulenum);
          return -EINVAL;
        }

      netlib_ipt_delete_internal(repl, e, hook);
      return OK;
    }

  head = (FAR uint8_t *)repl->entries + repl->hook_entry[hook];
  size = repl->underflow[hook] - repl->hook_entry[hook];
  ipt_entry_for_every(e, head, size)
    {
      if (e->next_offset == entry->next_offset &&
          e->target_offset == entry->target_offset &&
          strcmp(e->ip.outiface, entry->ip.outiface) == 0 &&
          strcmp(IPT_TARGET(e)->u.user.name,
                 IPT_TARGET(entry)->u.user.name) == 0)
        {
          netlib_ipt_delete_internal(repl, e, hook);
          return OK;
        }
    }

  return -ENOENT;
}

/****************************************************************************
 * Name: netlib_ipt_masquerade_entry
 *
 * Description:
 *   Alloc an entry with masquerade target and config to apply on ifname.
 *
 * Input Parameters:
 *   ifname - The device name to apply NAT on.
 *
 * Returned Value:
 *   The pointer to the entry, or NULL if failed.
 *   Caller must free it after use.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_NAT
FAR struct ipt_entry *netlib_ipt_masquerade_entry(FAR const char *ifname)
{
  size_t len;
  FAR struct ipt_masquerade_entry_s *entry;

  len = strlen(ifname);
  if (len + 1 > IFNAMSIZ)
    {
      return NULL;
    }

  entry = zalloc(sizeof(*entry));
  if (entry == NULL)
    {
      return NULL;
    }

  IPT_FILL_ENTRY(entry, XT_MASQUERADE_TARGET);

  strlcpy(entry->entry.ip.outiface, ifname,
          sizeof(entry->entry.ip.outiface));
  memset(entry->entry.ip.outiface_mask, 0xff, len + 1);

  return &entry->entry;
}
#endif
