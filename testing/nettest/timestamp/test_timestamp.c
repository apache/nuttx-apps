/****************************************************************************
 * apps/testing/nettest/timestamp/test_timestamp.c
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

#include <setjmp.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <cmocka.h>

#include "test_timestamp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  const struct CMUnitTest timestamp_tests[] =
    {
      cmocka_unit_test(test_timestamp_setsockopt),
      cmocka_unit_test(test_timestamp_compat),
      cmocka_unit_test(test_timestamp_tx),
      cmocka_unit_test(test_timestamp_rx),
      cmocka_unit_test(test_timestamp_poll),
    };

  return cmocka_run_group_tests(timestamp_tests,
                                test_timestamp_group_setup,
                                test_timestamp_group_teardown);
}
