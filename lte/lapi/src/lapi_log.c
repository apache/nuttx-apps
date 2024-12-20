/****************************************************************************
 * apps/lte/lapi/src/lapi_log.c
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

#include <errno.h>
#include <sys/param.h>
#include <nuttx/wireless/lte/lte_ioctl.h>

#include "lte/lapi.h"
#include "lte/lte_log.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: lte_log_collect
 ****************************************************************************/

int lte_log_collect(FAR char *output_fname, size_t len)
{
  FAR void *inarg[] =
    {
      (FAR void *)&len
    };

  FAR void *outarg[] =
    {
      output_fname, (FAR void *)&len
    };

  if ((output_fname != NULL) && (len != LTE_LOG_NAME_LEN))
    {
      return -ENOBUFS;
    }

  len = LTE_LOG_NAME_LEN;

  return lapi_req(LTE_CMDID_SAVE_LOG,
                 inarg, nitems(inarg),
                 outarg, nitems(outarg),
                 NULL);
}

/****************************************************************************
 * Name: lte_log_getlist
 ****************************************************************************/

int lte_log_getlist(size_t listsize, size_t fnamelen,
                    char list[listsize][fnamelen])
{
  FAR void *inarg[] =
    {
      (FAR void *)fnamelen
    };

  FAR void *outarg[] =
    {
      list, (FAR void *)listsize, (FAR void *)fnamelen
    };

  if ((listsize == 0) || (list == NULL))
    {
      return -EINVAL;
    }

  if (fnamelen != LTE_LOG_NAME_LEN)
    {
      return -ENOBUFS;
    }

  return lapi_req(LTE_CMDID_GET_LOGLIST,
                 inarg, nitems(inarg),
                 outarg, nitems(outarg),
                 NULL);
}

#ifdef CONFIG_LTE_LAPI_LOG_ACCESS

/****************************************************************************
 * Name: lte_log_open
 ****************************************************************************/

int lte_log_open(FAR const char *filename)
{
  int dummy_arg; /* Dummy for blocking API call */
  FAR void *inarg[] =
    {
      (FAR void *)filename
    };

  if (filename == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_LOGOPEN,
                  inarg, nitems(inarg),
                  (FAR void *)&dummy_arg, 0,
                  NULL);
}

/****************************************************************************
 * Name: lte_log_close
 ****************************************************************************/

int lte_log_close(int fd)
{
  int dummy_arg; /* Dummy for blocking API call */
  FAR void *inarg[] =
    {
      (FAR void *)fd
    };

  return lapi_req(LTE_CMDID_LOGCLOSE,
                  inarg, nitems(inarg),
                  (FAR void *)&dummy_arg, 0,
                  NULL);
}

/****************************************************************************
 * Name: lte_log_read
 ****************************************************************************/

ssize_t lte_log_read(int fd, FAR void *buf, size_t len)
{
  FAR void *inarg[] =
    {
      (FAR void *)fd, (FAR void *)len
    };

  FAR void *outarg[] =
    {
      buf, (FAR void *)len
    };

  if ((buf == NULL) || (len == 0))
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_LOGREAD,
                  inarg, nitems(inarg),
                  outarg, nitems(outarg),
                  NULL);
}

/****************************************************************************
 * Name: lte_log_lseek
 ****************************************************************************/

int lte_log_lseek(int fd, off_t offset, int whence)
{
  int dummy_arg; /* Dummy for blocking API call */
  FAR void *inarg[] =
    {
      (FAR void *)fd, (FAR void *)&offset, (FAR void *)whence
    };

  return lapi_req(LTE_CMDID_LOGLSEEK,
                  inarg, nitems(inarg),
                  (FAR void *)&dummy_arg, 0,
                  NULL);
}

/****************************************************************************
 * Name: lte_log_remove
 ****************************************************************************/

int lte_log_remove(FAR const char *filename)
{
  int dummy_arg; /* Dummy for blocking API call */
  FAR void *inarg[] =
    {
      (FAR void *)filename
    };

  if (filename == NULL)
    {
      return -EINVAL;
    }

  return lapi_req(LTE_CMDID_LOGREMOVE,
                  inarg, nitems(inarg),
                  (FAR void *)&dummy_arg, 0,
                  NULL);
}

#endif /* CONFIG_LTE_LAPI_LOG_ACCESS */
