/****************************************************************************
 * apps/examples/stepper/stepper.c
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

#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdbool.h>

#include <nuttx/motor/stepper.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

struct stepper_args
{
  FAR const char *path;
  int microstep;
  int speed;
  int steps;
};

static int parse_args(int argc, FAR char *argv[],
                      FAR struct stepper_args *args)
{
  int i;

  if (argc < 3)
    {
      return -1;
    }

  args->path = argv[1];
  args->microstep = -1;
  args->speed = 200;

  for (i = 2; i < argc; ++i)
    {
      if (strncmp("-m", argv[i], 2) == 0)
        {
          i++;
          if (i >= argc)
            {
              return -1;
            }

          args->microstep = atoi(argv[i]);
          i++;
        }

      if (strncmp("-s", argv[i], 2) == 0)
        {
          i++;
          if (i >= argc)
            {
              return -1;
            }

          args->speed = atoi(argv[i]);
          i++;
        }
    }

  args->steps = atoi(argv[argc - 1]);
  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct stepper_args args;
  struct stepper_status_s status;
  struct stepper_job_s job;
  int fd;
  int rc;

  if (parse_args(argc, argv, &args) < 0)
    {
      printf("usage:\n"
            "stepper path\n"
            "              [-m microstep (sticky)]\n"
            "              [-s speed in steps/s (default 200)]\n"
            "              steps (positive: CW, negative: CCW)\n");
      return 1;
    }

  fd = open(args.path, O_RDWR);
  if (fd < 0)
    {
      printf("Oops: %s\n", strerror(errno));
      exit(1);
    }

  if (args.microstep > 0)
    {
      printf("Set microstepping to %d\n", args.microstep);
      rc = ioctl(fd, STEPIOC_MICROSTEPPING, args.microstep);
      if (rc < 0)
        {
          printf("STEPIOC_MICROSTEPPING: %s\n", strerror(errno));
          exit(2);
        }
    }

  job.steps = args.steps;
  job.speed = args.speed;

  /* Retrieve the absolute position - before movement */

  rc = read(fd, &status, sizeof(struct stepper_status_s));
  if (rc != sizeof(struct stepper_status_s))
    {
      printf("read: %s\n", strerror(errno));
    }

  printf("Position before stepper motor moving: %ld\n", status.position);

  /* Move the stepper */

  printf("GO -> %ld @ %ld steps/s\n", job.steps, job.speed);

  rc = write(fd, &job, sizeof(struct stepper_job_s)); /* blocking */
  if (rc != sizeof(struct stepper_job_s))
    {
      printf("write: %s\n", strerror(errno));
    }

  /* Retrieve the absolute position - after movement */

  rc = read(fd, &status, sizeof(struct stepper_status_s));
  if (rc != sizeof(struct stepper_status_s))
    {
      printf("read: %s\n", strerror(errno));
    }

  printf("Position after stepper motor moving: %ld\n", status.position);

  return 0;
}
