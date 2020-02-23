/****************************************************************************
 * apps/fsutils/passwd/passwd_lock.c
 *
 *   Copyright (C) 2016 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
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
