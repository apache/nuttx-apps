/****************************************************************************
 * apps/fsutils/passwd/passwd_lock.c
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

#include <semaphore.h>
#include <assert.h>
#include <errno.h>

#include "passwd.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifdef CONFIG_FS_NAMED_SEMAPHORES
/* In the kernel build mode, we need to use a named semaphore so that all
 * processes will share the same, named semaphore instance.
 */

#  define PASSWD_SEMNAME "pwsem" /* Global named semaphore */
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

#ifndef CONFIG_FS_NAMED_SEMAPHORES
/* In the FLAT and PROTECTED build modes, we do not need to bother with a
 * named semaphore.  We use a single global semaphore in these cases.
 */

static sem_t g_passwd_sem =  SEM_INITIALIZER(1);
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/
/****************************************************************************
 * Name: passwd_lock
 *
 * Description:
 *   Lock the /etc/passwd file.  This is not a real lock at the level of the
 *   file system.  Rather, it only prevents concurrent modification of the
 *   /etc/passwd file by passwd_adduser(), passwd_deluser(), and
 *   passwd_update().  Other accesses to /etc/passwd could still cause
 *   concurrency problem and file corruption.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero (OK) is returned on success; a negated errno value is returned on
 *   failure.
 *
 ****************************************************************************/

int passwd_lock(FAR sem_t **semp)
{
  FAR sem_t *sem;

#ifdef CONFIG_FS_NAMED_SEMAPHORES
  /* Open the shared, named semaphore */

  sem = sem_open(PASSWD_SEMNAME, O_CREAT, 0644, 1);
  if (sem == SEM_FAILED)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);
      return -errcode;
    }
#else
  /* Use the global semaphore */

  sem = &g_passwd_sem;
#endif

  /* Take the semaphore.  Only EINTR errors are expected. */

  while (sem_wait(sem) < 0)
    {
      int errcode = errno;
      DEBUGASSERT(errcode == EINTR || errcode == ECANCELED);
      UNUSED(errcode);
    }

  *semp = sem;
  return OK;
}

/****************************************************************************
 * Name: passwd_unlock
 *
 * Description:
 *   Undo the work done by passwd_lock.
 *
 * Input Parameters:
 *   sem  Pointer to the semaphore
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

void passwd_unlock(FAR sem_t *sem)
{
  /* Release our count on the semaphore */

  sem_post(sem);

#ifdef CONFIG_FS_NAMED_SEMAPHORES
  /* Close the named semaphore */

  sem_close(sem);
#endif
}
