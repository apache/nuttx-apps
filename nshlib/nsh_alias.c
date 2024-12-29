/****************************************************************************
 * apps/nshlib/nsh_alias.c
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <nuttx/queue.h>

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_ALIAS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Macro to get head of alias list */

#define alias_head(list)        (FAR struct nsh_alias_s *)sq_peek(list)
#define alias_remfirst(list)    (FAR struct nsh_alias_s *)sq_remfirst(list)

/* Alias message format */

#define g_savefail_format       "alias %s='%s' failed\n"

/* Common for both alias / unalias */

#define g_noalias_format        "%s: %s not found\n"

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Alias message format */

static const char g_aliasfmt[]    = "alias %s='%s'\n";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: alias_init
 ****************************************************************************/

void alias_init(FAR struct nsh_vtbl_s *vtbl)
{
  int i;

  if (!sq_empty(&vtbl->alist) || !sq_empty(&vtbl->afreelist))
    {
      /* If either list is non-empty, we are initialized already */

      return;
    }

  sq_init(&vtbl->alist);
  sq_init(&vtbl->afreelist);

  for (i = 0; i < CONFIG_NSH_ALIAS_MAX_AMOUNT; i++)
    {
      sq_addlast((FAR struct sq_entry_s *)&vtbl->atab[i], &vtbl->afreelist);
    }

  return;
}

/****************************************************************************
 * Name: alias_find
 ****************************************************************************/

static FAR struct nsh_alias_s *alias_find(FAR struct nsh_vtbl_s *vtbl,
                                          FAR const char *name)
{
  FAR struct nsh_alias_s *alias;

  for (alias = alias_head(&vtbl->alist); alias; alias = alias->next)
    {
      if (strcmp(alias->name, name) == 0)
        {
          return alias;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: alias_delete
 ****************************************************************************/

static void alias_delete(FAR struct nsh_vtbl_s *vtbl,
                         FAR struct nsh_alias_s *alias)
{
  if (alias)
    {
      if (alias->exp)
        {
          /* Mark it for removal, but keep the data intact */

          alias->rem = 1;
          return;
        }

      if (alias->name)
        {
          free(alias->name);
        }

      if (alias->value)
        {
          free(alias->value);
        }

      alias->name = NULL;
      alias->value = NULL;
      alias->flags = 0;

      sq_rem((FAR sq_entry_t *)alias, &vtbl->alist);
      sq_addfirst((FAR sq_entry_t *)alias, &vtbl->afreelist);
    }
}

/****************************************************************************
 * Name: alias_save
 ****************************************************************************/

static int alias_save(FAR struct nsh_vtbl_s *vtbl, FAR const char *name,
                      FAR const char *value)
{
  FAR struct nsh_alias_s *alias;
  int ret = OK;

  if (!name || *name == '\0' || !value)
    {
      return -EINVAL;
    }

  if ((alias = alias_find(vtbl, name)) != NULL)
    {
      /* Update the value */

      free(alias->value);
      alias->value = strdup(value);
    }
  else if ((alias = alias_remfirst(&vtbl->afreelist)) != NULL)
    {
      /* Create new value */

      alias->name = strdup(name);
      alias->value = strdup(value);
      sq_addlast((FAR sq_entry_t *)alias, &vtbl->alist);
    }

  if (!alias || !alias->name || !alias->value)
    {
      /* Something went wrong, clean up after ourselves */

      alias_delete(vtbl, alias);
      ret = -ENOMEM;
    }

  return ret;
}

/****************************************************************************
 * Name: alias_printall
 ****************************************************************************/

static void alias_printall(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct nsh_alias_s *alias;

  for (alias = alias_head(&vtbl->alist); alias; alias = alias->next)
    {
      nsh_output(vtbl, g_aliasfmt, alias->name, alias->value);
    }
}

/****************************************************************************
 * Name: alias_removeall
 ****************************************************************************/

static void alias_removeall(FAR struct nsh_vtbl_s *vtbl)
{
  FAR struct nsh_alias_s *alias;
  FAR struct nsh_alias_s *next;

  alias = alias_head(&vtbl->alist);

  while (alias)
    {
      next = alias->next;
      alias_delete(vtbl, alias);
      alias = next;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_aliasfind
 *
 * Description:
 *   Find alias for token. Returns the alias structure or NULL if not found.
 *
 * Input Parameters:
 *   vtbl  - NSH session data.
 *   token - The argument string to find.
 *
 * Returned Value:
 *   The alias is returned, if one found, otherwise NULL.
 *
 ****************************************************************************/

FAR struct nsh_alias_s *nsh_aliasfind(FAR struct nsh_vtbl_s *vtbl,
                                      FAR const char *token)
{
  FAR struct nsh_alias_s *alias;

  /* Init, if necessary */

  alias_init(vtbl);

  if (token)
    {
      /* See if such an alias exists ? */

      alias = alias_find(vtbl, token);
      if (alias && !alias->exp && alias->value)
        {
          /* Yes, return the alias */

          return alias;
        }
    }

  return NULL;
}

/****************************************************************************
 * Name: nsh_aliasfree
 *
 * Description:
 *   Free memory for any deleted alias, aliases are kept in memory until all
 *   references to it have been freed.
 *
 * Input Parameters:
 *   vtbl  - NSH session data.
 *   alias - Pointer to alias data that is freed.
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void nsh_aliasfree(FAR struct nsh_vtbl_s *vtbl,
                   FAR struct nsh_alias_s *alias)
{
  alias_delete(vtbl, alias);
}

/****************************************************************************
 * Name: cmd_alias
 *
 * Description:
 *   Handle 'alias' command from terminal. Multiple alias values can be given
 *   by a single command.
 *
 *   Command does one of three things:
 *   1) If no arguments are given, every alias is printed to terminal.
 *   2) If a known alias name is given, the value is printed to terminal.
 *   3) If a "name=value" pair is given, a new alias is created, if there is
 *      room.
 *
 * Input Parameters:
 *   vtbl - The NSH console.
 *   argc - Amount of argument strings in command.
 *   argv - The argument strings.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int cmd_alias(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR struct nsh_alias_s *alias;
  FAR char **arg;
  FAR char *value;
  int ret = OK;

  /* Init, if necessary */

  alias_init(vtbl);

  if (argc < 2)
    {
      /* Print the alias list */

      alias_printall(vtbl);
      return ret;
    }

  /* Traverse through the argument vector */

  for (arg = argv + 1; *arg; arg++)
    {
      /* Look for name=value */

      if ((value = strchr(*arg, '=')) != NULL)
        {
          /* Save new / modify existing alias */

          *value++ = '\0';

          ret = alias_save(vtbl, *arg, value);
          if (ret < 0)
            {
              nsh_error(vtbl, g_savefail_format, *arg, value);
            }
        }
      else if ((alias = alias_find(vtbl, *arg)) != NULL)
        {
          /* Found it */

          nsh_output(vtbl, g_aliasfmt, alias->name, alias->value);
        }
      else
        {
          /* Nothing found */

          nsh_error(vtbl, g_noalias_format, "alias", *arg);
          ret = -ENOENT;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: cmd_unalias
 *
 * Description:
 *   Handle 'cmd_unalias' command from terminal.
 *
 *   Command does one of two things:
 *   1) If the '-a' argument is given, all aliases are destroyed.
 *   2) If a known alias name is given, the name=value pair is destroyed.
 *
 * Input Parameters:
 *   vtbl - The NSH console.
 *   argc - Amount of argument strings in command.
 *   argv - The argument strings.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int cmd_unalias(FAR struct nsh_vtbl_s *vtbl, int argc, FAR char **argv)
{
  FAR struct nsh_alias_s *alias;
  FAR char **arg;
  int ret = OK;

  /* Init, if necessary */

  alias_init(vtbl);

  if (argc < 2)
    {
      /* No arguments given, this is an error */

      return -EINVAL;
    }

  /* If '-a' is provided, then just wipe them all */

  if (getopt(argc, argv, "a") == 'a')
    {
      alias_removeall(vtbl);
      return ret;
    }

  /* Traverse through the argument vector */

  for (arg = argv + 1; *arg; arg++)
    {
      if ((alias = alias_find(vtbl, *arg)) != NULL)
        {
          /* Found it */

          alias_delete(vtbl, alias);
        }
      else
        {
          /* Nothing found */

          nsh_error(vtbl, g_noalias_format, "unalias", *arg);
          ret = -ENOENT;
        }
    }

  return ret;
}

#endif /* CONFIG_NSH_ALIAS */
