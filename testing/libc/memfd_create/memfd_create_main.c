/****************************************************************************
 * apps/testing/libc/memfd_create/memfd_create_main.c
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

#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Linux accepts names of up to 249 characters */

#define MEMFD_CREATE_TEST_NAMELEN 200
#define MEMFD_CREATE_TEST_SIZE    64

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Print the result of one check.  Return the number of errors. */

static int check(FAR const char *name, bool ok, int errcode)
{
  if (!ok)
    {
      printf("ERROR: %s: errno %d\n", name, errcode);
      return 1;
    }

  printf("%s: OK\n", name);
  return 0;
}

/* Make a file with memfd_create() and check that it is there */

static int create(FAR const char *test, FAR const char *name,
                  unsigned int flags)
{
  int fd;

  errno = 0;
  fd = memfd_create(name, flags);
  if (fd >= 0)
    {
      close(fd);
    }

  return check(test, fd >= 0, errno);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  char longname[MEMFD_CREATE_TEST_NAMELEN + 1];
  char buf[6];
  struct stat st;
  int nerrors = 0;
  bool ok;
  int fd;
  int fd2;

  printf("memfd_create test\n");

  nerrors += create("flags 0", "memfd_test", 0);
  nerrors += create("MFD_ALLOW_SEALING", "memfd_test",
                    MFD_CLOEXEC | MFD_ALLOW_SEALING);

  memset(longname, 'n', MEMFD_CREATE_TEST_NAMELEN);
  longname[MEMFD_CREATE_TEST_NAMELEN] = '\0';
  nerrors += create("long name", longname, MFD_CLOEXEC);

  errno = 0;
  fd = memfd_create("memfd_test", MFD_CLOEXEC);
  ok = fd >= 0 && (fcntl(fd, F_GETFD) & FD_CLOEXEC) != 0;
  nerrors += check("MFD_CLOEXEC", ok, errno);
  if (fd < 0)
    {
      goto out;
    }

  /* The size first: a shmfs file has no memory before ftruncate() */

  errno = 0;
  ok = ftruncate(fd, MEMFD_CREATE_TEST_SIZE) == 0 && fstat(fd, &st) == 0 &&
       st.st_size == MEMFD_CREATE_TEST_SIZE;
  nerrors += check("ftruncate", ok, errno);

  errno = 0;
  ok = write(fd, "hello", 5) == 5 && lseek(fd, 0, SEEK_SET) == 0 &&
       read(fd, buf, 5) == 5 && memcmp(buf, "hello", 5) == 0;
  nerrors += check("write and read", ok, errno);

  /* A second file with the same name is a new, empty file */

  errno = 0;
  fd2 = memfd_create("memfd_test", MFD_CLOEXEC);
  ok = fd2 >= 0 && fstat(fd2, &st) == 0 && st.st_size == 0;
  nerrors += check("same name", ok, errno);
  if (fd2 >= 0)
    {
      close(fd2);
    }

  close(fd);

out:
  printf("memfd_create test: %d errors\n", nerrors);
  return nerrors > 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}
