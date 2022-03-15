/****************************************************************************
 * apps/system/uorb/test/utility.c
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

#include <stdio.h>
#include <inttypes.h>

#include "utility.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

#ifdef CONFIG_DEBUG_UORB
static void print_orb_test_msg(FAR const struct orb_metadata *meta,
                               FAR const void *buffer)
{
  FAR const struct orb_test_s *message = buffer;
  const orb_abstime now = orb_absolute_time();

  uorbinfo_raw("%s:\ttimestamp: %"PRIu64" (%"PRIu64" us ago) val: %"PRId32"",
               meta->o_name, message->timestamp, now - message->timestamp,
               message->val);
}

static void print_orb_test_medium_msg(FAR const struct orb_metadata *meta,
                                      FAR const void *buffer)
{
  FAR const struct orb_test_medium_s *message = buffer;
  const orb_abstime now = orb_absolute_time();

  uorbinfo_raw("%s:\ttimestamp: %"PRIu64" (%"PRIu64" us ago) val: %"PRId32"",
               meta->o_name, message->timestamp, now - message->timestamp,
               message->val);
}

static void print_orb_test_large_msg(FAR const struct orb_metadata *meta,
                                     FAR const void *buffer)
{
  FAR const struct orb_test_large_s *message = buffer;
  const orb_abstime now = orb_absolute_time();

  uorbinfo_raw("%s:\ttimestamp: %"PRIu64" (%"PRIu64" us ago) val: %"PRId32"",
               meta->o_name, message->timestamp, now - message->timestamp,
               message->val);
}
#endif

/****************************************************************************
 * Public Data
 ****************************************************************************/

ORB_DEFINE(orb_test, struct orb_test_s, print_orb_test_msg);
ORB_DEFINE(orb_multitest, struct orb_test_s, print_orb_test_msg);
ORB_DEFINE(orb_test_medium, struct orb_test_medium_s,
           print_orb_test_medium_msg);
ORB_DEFINE(orb_test_medium_multi, struct orb_test_medium_s,
           print_orb_test_medium_msg);
ORB_DEFINE(orb_test_medium_wrap_around, struct orb_test_medium_s,
           print_orb_test_medium_msg);
ORB_DEFINE(orb_test_medium_queue, struct orb_test_medium_s,
           print_orb_test_medium_msg);
ORB_DEFINE(orb_test_medium_queue_poll, struct orb_test_medium_s,
           print_orb_test_medium_msg);
ORB_DEFINE(orb_test_large, struct orb_test_large_s,
           print_orb_test_large_msg);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int test_note(FAR const char *fmt, ...)
{
  va_list ap;

  fprintf(stderr, "uORB note: ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);

  va_end(ap);

  fprintf(stderr, "\n");
  fflush(stderr);

  return OK;
}

int test_fail(FAR const char *fmt, ...)
{
  va_list ap;

  fprintf(stderr, "uORB fail: ");
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);

  va_end(ap);

  fprintf(stderr, "\n");
  fflush(stderr);

  return ERROR;
}
