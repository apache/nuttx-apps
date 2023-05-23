/****************************************************************************
 * apps/system/coredump/coredump.c
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
#include <nuttx/streams.h>
#include <nuttx/binfmt/binfmt.h>

#include <sys/types.h>

#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>

#include <execinfo.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * coredump_main
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  FAR struct lib_stdoutstream_s *outstream;
  FAR struct lib_hexdumpstream_s *hstream;
  FAR struct lib_lzfoutstream_s *lstream;
  char *name = NULL;
  FAR void *stream;
  int logmask;
  int pid = 0;
  FILE *file;

  if (argc >= 2)
    {
      pid = atoi(argv[1]);
      if (pid == 0)
        {
          name = argv[1];
        }

      if (argc >= 3)
        {
          if (name == NULL)
            {
              name = argv[2];
            }
          else
            {
              pid = atoi(argv[2]);
            }
        }
    }

  if (pid == 0)
    {
      pid = INVALID_PROCESS_ID;
    }

  if (name != NULL)
    {
      file = fopen(name, "w");
      if (file == NULL)
        {
          return 1;
        }
    }
  else
    {
      file = stdout;
    }

  hstream = malloc(sizeof(*hstream) + sizeof(*lstream) + sizeof(*outstream));
  if (hstream == NULL)
    {
      if (name != NULL)
        {
          fclose(file);
        }

      return 1;
    }

  lstream = (FAR void *)(hstream + 1);
  outstream = (FAR void *)(lstream + 1);

  logmask = setlogmask(LOG_ALERT);

  printf("Start coredump:\n");

  /* Initialize hex output stream */

  lib_stdoutstream(outstream, file);
  lib_hexdumpstream(hstream, (FAR void *)outstream);

  stream = hstream;

#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION

  /* Initialize LZF compression stream */

  lib_lzfoutstream(lstream, stream);
  stream = lstream;

#endif

  /* Do core dump */

  core_dump(NULL, stream, pid);

#ifdef CONFIG_BOARD_COREDUMP_COMPRESSION
  printf("Finish coredump (Compression Enabled).\n");
#else
  printf("Finish coredump.\n");
#endif

  setlogmask(logmask);

  free(hstream);

  if (name != NULL)
    {
      fclose(file);
    }

  return 0;
}
