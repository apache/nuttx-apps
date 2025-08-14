/****************************************************************************
 * apps/system/nxinit/import.c
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

#include <errno.h>
#include <sys/param.h>

#include "import.h"
#include "init.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int init_import_parse(FAR const struct parser_s *parser,
                      bool create, FAR char *buf)
{
  FAR char *argv[2];
  int ret;

  if (create)
    {
      init_parse_arguments(buf, false, nitems(argv), argv);
      ret = init_parse_config_file(parser, argv[1]);
      if (ret < 0)
        {
          init_err("Import %s", argv[1]);
          return ret;
        }
    }
  else
    {
      init_err("Parse import");
      return -EINVAL;
    }

  return 0;
}
