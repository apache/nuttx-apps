/****************************************************************************
 * apps/testing/drivers/nand_sim/nand_sim_main.c
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

#include <debug.h>
#include <stdio.h>

#include <nuttx/drivers/drivers.h>
#include <nuttx/mtd/nand.h>
#include <nuttx/mtd/nand_scheme.h>
#include <nuttx/mtd/nand_ram.h>
#include <nuttx/mtd/nand_wrapper.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define NAND_SIM_NAME "nand"
#define NAND_SIM_PATH "/dev/" NAND_SIM_NAME

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Data
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

FAR struct mtd_dev_s    *g_nand_sim_mtd_wrapper;
FAR struct mtd_dev_s    *g_nand_sim_mtd_under;
FAR struct nand_raw_s   *g_nand_mtd_raw;

/****************************************************************************
 * External Functions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: wrapper_init
 *
 * Description:
 *   Initializes the wrapper by allocating memory and assiging the methods.
 *
 * Returned Value:
 *   0: Successful
 *   -ENOMEM: No memory left to allocate device
 *
 ****************************************************************************/

int wrapper_init(void)
{
  struct nand_dev_s *under;
  struct nand_dev_s *wrapper;

  g_nand_sim_mtd_wrapper = kmm_zalloc(sizeof(struct nand_wrapper_dev_s));
  if (g_nand_sim_mtd_wrapper == NULL)
    {
      return -ENOMEM;
    }

  under = &((struct nand_wrapper_dev_s *)g_nand_sim_mtd_wrapper)->under;
  wrapper = &((struct nand_wrapper_dev_s *)g_nand_sim_mtd_wrapper)->wrapper;

  memcpy(under, g_nand_sim_mtd_under, sizeof(struct nand_dev_s));
  memcpy(wrapper, g_nand_sim_mtd_under, sizeof(struct nand_dev_s));

  nand_wrapper_initialize();

  ((struct mtd_dev_s *)wrapper)->name    = NAND_SIM_NAME;
  ((struct mtd_dev_s *)wrapper)->erase   = nand_wrapper_erase;
  ((struct mtd_dev_s *)wrapper)->bread   = nand_wrapper_bread;
  ((struct mtd_dev_s *)wrapper)->bwrite  = nand_wrapper_bwrite;
  ((struct mtd_dev_s *)wrapper)->ioctl   = nand_wrapper_ioctl;
  ((struct mtd_dev_s *)wrapper)->isbad   = nand_wrapper_isbad;
  ((struct mtd_dev_s *)wrapper)->markbad = nand_wrapper_markbad;

  return OK;
}

/****************************************************************************
 * Name: terminate
 *
 * Description:
 *   Handles the SIGTERM signal by exitting gracefully.
 *
 ****************************************************************************/

void terminate(int sig)
{
  kmm_free(g_nand_sim_mtd_under);
  kmm_free(g_nand_sim_mtd_wrapper);

  unregister_mtddriver(NAND_SIM_PATH);
  syslog(LOG_DEBUG, "Exited!\n");
}

/****************************************************************************
 * Name: nand_sim_main
 *
 * Description:
 *   Entry point of the device emulator.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int   ret;
  pid_t pid;

  /* Daemon */

  pid = fork();

  if (pid > 0)
    {
      return OK;
    }

  if (daemon(0, 1) == -1)
    {
      ret = EXIT_FAILURE;
      goto errout;
    }

  /* Signal Handlers */

  signal(SIGTERM, terminate);

  /* Initializers */

  /* Raw NAND MTD Device */

  g_nand_mtd_raw = kmm_zalloc(sizeof(struct nand_raw_s));
  if (g_nand_mtd_raw == NULL)
    {
      ret = -ENOMEM;
      goto errout_with_logs;
    }

  g_nand_sim_mtd_under = nand_ram_initialize(g_nand_mtd_raw);
  if (g_nand_sim_mtd_under == NULL)
    {
      ret = -EINVAL;
      goto errout_with_raw_s;
    }

  ret = wrapper_init();
  if (ret < 0)
    {
      goto errout_with_mtd_under;
    }

  /* Under device driver is already copied to wrapper, so free. */

  kmm_free(g_nand_sim_mtd_under);

  ret = register_mtddriver(NAND_SIM_PATH,
                            g_nand_sim_mtd_wrapper,
                            0777, NULL);
  if (ret < 0)
    {
      goto errout_with_mtd_wrapper;
    }

  printf("Driver running!\n");

  /* To keep the daemon still running. All events are handled by signals */

  while (1)
    {
      sleep(1);
    }

  /* Won't reach this point */

  return OK;

errout_with_mtd_wrapper:
  kmm_free(g_nand_sim_mtd_wrapper);

errout_with_mtd_under:
  kmm_free(g_nand_sim_mtd_under);

errout_with_raw_s:
  kmm_free(g_nand_mtd_raw);

errout_with_logs:
  unregister_mtddriver(NAND_SIM_PATH);

errout:
  return ret;
}
