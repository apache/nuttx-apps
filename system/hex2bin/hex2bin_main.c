/****************************************************************************
 * apps/system/hex2bin/hex2bin_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <hex2bin.h>

#include <nuttx/streams.h>

#ifdef CONFIG_SYSTEM_HEX2BIN_BUILTIN

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
#ifdef CONFIG_SYSTEM_HEX2BIN_USAGE
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "\t%s [OPTIONS] <hexfile> <binfile>\n", progname);
  fprintf(stderr, "\t%s -h\n", progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t<hexfile>:\n");
  fprintf(stderr, "\t\tThe file containing the Intel HEX data to be\n");
  fprintf(stderr, "\t\tconverted.\n");
  fprintf(stderr, "\t<binfile>:\n");
  fprintf(stderr, "\t\tThe output file to be created contained the\n");
  fprintf(stderr, "\t\tconverted binary data.\n");
  fprintf(stderr, "\t-h:\n");
  fprintf(stderr, "\t\tPrints this message and exits\n");
  fprintf(stderr, "And [OPTIONS] include:\n");
  fprintf(stderr, "\t-s <start address>\n");
  fprintf(stderr, "\t\tSets the start address of the binary output. This\n");
  fprintf(stderr, "\t\tvalue is used to (1) calculate offsets into the\n");
  fprintf(stderr, "\t\toutput stream, and (2) for error checking.\n");
  fprintf(stderr, "\t\tDefault: 0x%08x\n", CONFIG_SYSTEM_HEX2BIN_BASEADDR);
  fprintf(stderr, "\t-e <end address>\n");
  fprintf(stderr, "\t\tSets the maximum address (plus 1) of the binary\n");
  fprintf(stderr, "\t\toutput. This value is used to only for error\n");
  fprintf(stderr, "\t\tchecking. The value zero disables error checking.\n");
  fprintf(stderr, "\t\t Default: 0x%08x\n", CONFIG_SYSTEM_HEX2BIN_ENDPADDR);
  fprintf(stderr, "\t\tno error checking\n");
  fprintf(stderr, "\t-w <swap code>\n");
  fprintf(stderr, "\t\t(0) No swap, (1) swap bytes in 16-bit values, or\n");
  fprintf(stderr, "\t\t(3) swap bytes in 32-bit values.  Default: %d\n",
          CONFIG_SYSTEM_HEX2BIN_SWAP);
#endif
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hex2bin_main
 *
 * Description:
 *   Main entry point when hex2bin is built as an NSH built-in task.
 *
 * Input Parameters:
 *   Standard task inputs
 *
 * Returned Value
 *   EXIT_SUCCESS on success; EXIT_FAILURE on failure
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct lib_stdinstream_s stdinstream;
  struct lib_stdsostream_s stdoutstream;
  FAR const char *hexfile;
  FAR const char *binfile;
  FAR char *endptr;
  FAR FILE *instream;
  FAR FILE *outstream;
  unsigned long baseaddr;
  unsigned long endpaddr;
  unsigned long swap;
  int option;
  int ret;

  /* Parse the command line options */

  baseaddr = CONFIG_SYSTEM_HEX2BIN_BASEADDR;
  endpaddr = CONFIG_SYSTEM_HEX2BIN_ENDPADDR;
  swap     = CONFIG_SYSTEM_HEX2BIN_SWAP;

  while ((option = getopt(argc, argv, ":hs:e:w:")) != ERROR)
    {
      switch (option)
        {
#ifdef CONFIG_SYSTEM_HEX2BIN_USAGE
        case 'h':
          show_usage(argv[0], EXIT_SUCCESS);
          break;
#endif

        case 's':
          baseaddr = strtoul(optarg, &endptr, 16);
          if (endptr == optarg)
            {
              fprintf(stderr, "ERROR: Invalid argument to the -s option\n");
              show_usage(argv[0], EXIT_FAILURE);
            }
          break;

        case 'e':
          endpaddr = strtoul(optarg, &endptr, 16);
          if (endptr == optarg)
            {
              fprintf(stderr, "ERROR: Invalid argument to the -e option\n");
              show_usage(argv[0], EXIT_FAILURE);
            }
          break;

        case 'w':
          swap = strtoul(optarg, &endptr, 16);
          if (endptr == optarg || swap > (unsigned long)HEX2BIN_SWAP32)
            {
              fprintf(stderr, "ERROR: Invalid argument to the -w option\n");
              show_usage(argv[0], EXIT_FAILURE);
            }
          break;

        case ':':
          fprintf(stderr, "ERROR: Missing required argument\n");
          show_usage(argv[0], EXIT_FAILURE);
          break;

        default:
        case '?':
          fprintf(stderr, "ERROR: Unrecognized option\n");
          show_usage(argv[0], EXIT_FAILURE);
          break;
        }
    }

  /* There should be two final parameters remaining on the command line */

  if (optind >= argc)
    {
      printf("ERROR: Missing required <hexfile> and <binfile> arguments\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

  hexfile = argv[optind];
  optind++;

  if (optind >= argc)
    {
      printf("ERROR: Missing required <binfile> argument\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

  binfile = argv[optind];
  optind++;

  if (optind < argc)
    {
      printf("ERROR: Garbage at end of command line\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

  /* Open the HEX file for reading */

  instream = fopen(hexfile, "r");
  if (instream == NULL)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      fprintf(stderr, "ERROR: Failed to open \"%s\" for reading: %d\n",
              hexfile, errcode);
      return -errcode;
    }

  /* Open the BIN file for reading */

  outstream = fopen(binfile, "wb");
  if (outstream == NULL)
    {
      int errcode = errno;
      DEBUGASSERT(errcode > 0);

      fprintf(stderr, "ERROR: Failed to open \"%s\" for writing: %d\n",
              binfile, errcode);
      fclose(instream);
      return -errcode;
    }

  /* Wrap the FILE streams as standard streams */

  lib_stdinstream(&stdinstream, instream);
  lib_stdsostream(&stdoutstream, outstream);

  /* And do the deed */

  ret = hex2bin(&stdinstream.public, &stdoutstream.public,
                (uint32_t)baseaddr, (uint32_t)endpaddr,
                (enum hex2bin_swap_e)swap);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to convert to binary: %d\n", ret);
    }

  /* Clean up and return */

  fflush(outstream);
  fclose(instream);
  fclose(outstream);
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

#endif /* CONFIG_SYSTEM_HEX2BIN_BUILTIN */
