/****************************************************************************
 * apps/examples/shm_test/shm_main.c
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

#include <fcntl.h>
#include <nuttx/config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <unistd.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void shm_producer(const int shm_size, const char *shm_name)
{
  /* Write Hello World! to shared memory */

  const char *message_0 = "Hello";
  const char *message_1 = "World!";

  /* Shared memory file descriptor */

  int shm_fd;

  /* Pointer to shared memory object */

  void *ptr;

  /* Create the shared memory object */

  shm_fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);

  /* Configure the size of the shared memory object */

  ftruncate(shm_fd, shm_size);

  /* Memory map the shared memory object */

  ptr = mmap(0, shm_size, PROT_WRITE, MAP_SHARED, shm_fd, 0);

  /* Write to the shared memory object */

  sprintf(ptr, "%s", message_0);

  ptr += strlen(message_0);
  sprintf(ptr, "%s", message_1);
  ptr += strlen(message_1);

  printf("Producer Wrote to SHM: %s%s\n", message_0, message_1);
}

void shm_consumer(const int shm_size, const char *shm_name)
{
  /* Shared memory file descriptor */

  int shm_fd;

  /* Pointer to shared memory object */

  void *ptr;

  /* Open the shared memory object */

  shm_fd = shm_open(shm_name, O_RDONLY, 0666);

  /* Memory map the shared memory object */

  ptr = mmap(0, shm_size, PROT_READ, MAP_SHARED, shm_fd, 0);

  /* Read from the shared memory object */

  printf("Consumer Read from SHM: %s\n", (char *)ptr);

  /* Remove the shared memory object */

  shm_unlink(shm_name);
}

/****************************************************************************
 * shm_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const int shm_size = 2048;
  const char *shm_name = "OS";

  shm_producer(shm_size, shm_name);
  shm_consumer(shm_size, shm_name);
  return 0;
}
