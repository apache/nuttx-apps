/****************************************************************************
 * apps/nshlib/nsh_vars.c
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>

#include "nsh.h"
#include "nsh_console.h"

#ifdef CONFIG_NSH_VARS

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_cmpname
 ****************************************************************************/

static bool nsh_cmpname(FAR const char *pszname, FAR const char *peqname)
{
  /* Search until we find anything different in the two names */

  for (; *pszname == *peqname; pszname++, peqname++)
    {
    }

  /* On success, pszname will end with '\0' and peqname with '=' */

  if (*pszname == '\0' && *peqname == '=')
    {
      return true;
    }

  return false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_findvar
 *
 * Description:
 *   Search the provided NSH console structure for the variable of the
 *   specified name.
 *
 * Input Parameters:
 *   pstate - The console state containing NSH variable array to be searched.
 *   name - The variable name to find
 *
 * Returned Value:
 *   A pointer to the name=value string in the NSH variable buffer
 *
 * Assumptions:
 *   - Not called from an interrupt handler
 *   - Pre-emption is disabled by caller
 *
 ****************************************************************************/

FAR char *nsh_findvar(FAR struct console_stdio_s *pstate,
                      FAR const char *name)
{
  FAR char *ptr;
  FAR char *end;

  /* Verify input parameters */

  DEBUGASSERT(pstate != NULL && name != NULL);

  /* Search for a name=value string with matching name */

  end = &pstate->varp[pstate->varsz];
  for (ptr = pstate->varp;
       ptr < end && !nsh_cmpname(name, ptr);
       ptr += (strlen(ptr) + 1));

  /* Check for success */

  return (ptr < end) ? ptr : NULL;
}

/****************************************************************************
 * Name: nsh_removevar
 *
 * Description:
 *   Remove the referenced name=value pair from the NSH variable buffer
 *
 * Input Parameters:
 *   pstate - The task pstate with the NSH variable buffer containing the
 *            name=value pair
 *   pair  - A pointer to the name=value pair in the restroom
 *
 * Returned Value:
 *   Zero on success
 *
 ****************************************************************************/

int nsh_removevar(FAR struct console_stdio_s *pstate, FAR char *pair)
{
  FAR char *end;                 /* Pointer to the end+1 of the NSH variable buffer */
  int alloc;                     /* Size of the allocated NSH variable buffer */
  int ret = ERROR;

  /* Verify that the pointer lies within the NSH variable region */

  alloc = pstate->varsz;         /* Size of the allocated NSH variable buffer */
  end   = &pstate->varp[alloc];  /* Pointer to the end+1 of the NSH variable buffer */

  if (pair >= pstate->varp && pair < end)
    {
      /* Set up for the removal */

      int len        = strlen(pair) + 1; /* Length of name=value string to remove */
      FAR char *src  = &pair[len];       /* Address of name=value string after */
      FAR char *dest = pair;             /* Location to move the next string */
      int count      = end - src;        /* Number of bytes to move (might be zero) */

      /* Move all of the NSH variable strings after the removed one 'down.'
       * this is inefficient, but probably not high duty.
       */

      while (count-- > 0)
        {
          *dest++ = *src++;
        }

      /* Then set to the new allocation size.  The caller is expected to
       * call realloc at some point but we don't do that here because the
       * caller may add more stuff to the NSH variable buffer.
       */

      pstate->varsz -= len;
      ret = OK;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_getvar, nsh_setvar, and nsh_setvar
 *
 * Description:
 *   Get, set, or unset an NSH variable.
 *
 * Input Parameters:
 *   vtbl  - NSH session data
 *   name  - The name of the variable to get or set
 *   value - The value to use with nsh_setvar()
 *
 * Returned value:
 *   nsh_getvar() returns a read-only reference to the variable value on
 *   success or NULL on failure.
 *   nset_unsetvar() returns OK on success or an negated errno value on
 *   failure.
 *
 ****************************************************************************/

/****************************************************************************
 * Name: nsh_getvar
 ****************************************************************************/

FAR char *nsh_getvar(FAR struct nsh_vtbl_s *vtbl, FAR const char *name)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  FAR char *pair;
  FAR char *value = NULL;

  DEBUGASSERT(pstate != NULL && name != NULL);

  /* Check if the variable exists */

  if ((pair = nsh_findvar(pstate, name)) == NULL)
    {
      return NULL;
    }

  /* It does!  Get the value sub-string from the name=value string */

  value = strchr(pair, '=');
  DEBUGASSERT(value != NULL);

  /* Adjust the pointer so that it points to the value right after the '=' */

  value++;
  return value;
}

/****************************************************************************
 * Name: nsh_setvar
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_SET
int nsh_setvar(FAR struct nsh_vtbl_s *vtbl, FAR const char *name,
               FAR const char *value)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  FAR char *pair;
  FAR char *newvarp;
  int newsize;
  int varlen;

  DEBUGASSERT(pstate != NULL && name != NULL && value != NULL);

  /* Check if the variable already exists */

  if (pstate->varp != NULL &&
      (pair = nsh_findvar(pstate, name)) != NULL)
    {
      /* Yes.. Remove the name=value pair from the NSH variable buffer.  It
       * will be added again below.
       */

      nsh_removevar(pstate, pair);
    }

  /* Get the size of the new name=value string.  The +2 is for the '=' and
   * for null terminator
   */

  varlen = strlen(name) + strlen(value) + 2;

  /* Then allocate or reallocate the NSH variable buffer */

  if (pstate->varp != NULL)
    {
      newsize = pstate->varsz + varlen;
      newvarp = (FAR char *)realloc(pstate->varp, newsize);
      if (newvarp == NULL)
        {
          return -ENOMEM;
        }

      pair = &newvarp[pstate->varsz];
    }
  else
    {
      newsize = varlen;
      newvarp = (FAR char *)malloc(varlen);
      if (!newvarp)
        {
          return -ENOMEM;
        }

      pair = newvarp;
    }

  /* Save the new buffer and size */

  pstate->varp  = newvarp;
  pstate->varsz = newsize;

  /* Now, put the new name=value string into the NSH variable buffer */

  sprintf(pair, "%s=%s", name, value);
  return OK;
}
#endif

/****************************************************************************
 * Name: nsh_unsetvar
 ****************************************************************************/

#if !defined(CONFIG_NSH_DISABLE_UNSET) || !defined(CONFIG_NSH_DISABLE_EXPORT)
int nsh_unsetvar(FAR struct nsh_vtbl_s *vtbl, FAR const char *name)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  FAR char *pair;
  FAR char *newvarp;
  int newsize;
  int ret = OK;

  DEBUGASSERT(name != NULL && pstate != NULL);

  /* Check if the variable exists */

  if (pstate != NULL && (pair = nsh_findvar(pstate, name)) != NULL)
    {
      /* It does!  Remove the name=value pair from the NSH variables. */

      nsh_removevar(pstate, pair);

      /* Reallocate the new NSH variable buffer */

      newsize = pstate->varsz;
      if (newsize <= 0)
        {
          /* Free the old NSH variable (if there was one) */

          if (pstate->varp != NULL)
            {
              free(pstate->varp);
              pstate->varp = NULL;
            }

          pstate->varsz = 0;
        }
      else
        {
          /* Reallocate the NSH variable buffer to reclaim a little memory */

          newvarp = (FAR char *)realloc(pstate->varp, newsize);
          if (newvarp == NULL)
            {
              return -ENOMEM;  /* Shouldn't happen when realloc'ing down */
            }
          else
            {
              /* Save the new NSH variable pointer (it might have changed due
               * to reallocation).
               */

              pstate->varp = newvarp;
            }
        }
    }

  sched_unlock();
  return ret;
}
#endif

/****************************************************************************
 * Name: nsh_foreach_var
 *
 * Description:
 *   Visit each name-value pair in the environment.
 *
 * Input Parameters:
 *   vtbl  - NSH session data
 *   cb    - The callback function to be invoked for each environment
 *           variable.
 *
 * Returned Value:
 *   Zero if the all NSH variables have been traversed.  A non-zero value
 *   means that the callback function requested early termination by
 *   returning a nonzero value.
 *
 ****************************************************************************/

#ifndef CONFIG_NSH_DISABLE_SET
int nsh_foreach_var(FAR struct nsh_vtbl_s *vtbl, nsh_foreach_var_t cb,
                    FAR void *arg)
{
  FAR struct console_stdio_s *pstate = (FAR struct console_stdio_s *)vtbl;
  FAR char *ptr;
  FAR char *end;
  int ret = OK;

  /* Verify input parameters */

  DEBUGASSERT(pstate != NULL && cb != NULL);

  /* Search for a name=value string with matching name */

  end = &pstate->varp[pstate->varsz];
  for (ptr = pstate->varp; ptr < end; ptr += (strlen(ptr) + 1))
    {
      /* Perform the callback */

      ret = cb(vtbl, arg, ptr);

      /* Terminate the traversal early if the callback so requests by
       * returning a non-zero value.
       */

      if (ret != 0)
        {
          break;
        }
    }

  return ret;
}
#endif

#endif /* CONFIG_NSH_VARS */
