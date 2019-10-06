/****************************************************************************
 * apps/system/hex2mem_main.c
 *
 *   Copyright (C) 2014, 2016 Gregory Nutt. All rights reserved.
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

#include <nuttx/config.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <hex2bin.h>

#include <nuttx/streams.h>

#ifdef CONFIG_SYSTEM_HEX2MEM_BUILTIN

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: show_usage
 ****************************************************************************/

static void show_usage(FAR const char *progname, int exitcode)
{
#ifdef CONFIG_SYSTEM_HEX2MEM_USAGE
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "\t%s [OPTIONS] <hexfile>\n", progname);
  fprintf(stderr, "\t%s -h\n", progname);
  fprintf(stderr, "Where:\n");
  fprintf(stderr, "\t<hexfile>:\n");
  fprintf(stderr, "\t\tThe file containing the Intel HEX data to be converted.\n");
  fprintf(stderr, "\t-h:\n");
  fprintf(stderr, "\t\tPrints this message and exits\n");
  fprintf(stderr, "And [OPTIONS] include:\n");
  fprintf(stderr, "\t-s <start address>\n");
  fprintf(stderr, "\t\tSets the start memory address for the binary output.  Hex\n");
  fprintf(stderr, "\t\tdata is written to memory relative to this address. Default:\n");
  fprintf(stderr, "\t\t0x%08x\n", CONFIG_SYSTEM_HEX2MEM_BASEADDR);
  fprintf(stderr, "\t-e <end address>\n");
  fprintf(stderr, "\t\tSets the maximum memory address (plus 1).  This value is\n");
  fprintf(stderr, "\t\tused to assure that the program does not write past the end\n");
  fprintf(stderr, "\t\tof memory.  The value zero disables error checking.\n");
  fprintf(stderr, "\t\tDefault: 0x%08x\n", CONFIG_SYSTEM_HEX2MEM_ENDPADDR);
  fprintf(stderr, "\t-w <swap code>\n");
  fprintf(stderr, "\t\t(0) No swap, (1) swap bytes in 16-bit values, or (3) swap\n");
  fprintf(stderr, "\t\tbytes in 32-bit values.  Default: %d\n",
          CONFIG_SYSTEM_HEX2MEM_SWAP);
#endif
  exit(exitcode);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: hex2mem_main
 *
 * Description:
 *   Main entry point when hex2mem is built as an NSH built-in task.
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
  struct lib_memsostream_s memoutstream;
  FAR const char *hexfile;
  FAR char *endptr;
  FAR FILE *instream;
  unsigned long baseaddr;
  unsigned long endpaddr;
  unsigned long swap;
  int option;
  int ret;

  /* Parse the command line options */

  baseaddr = CONFIG_SYSTEM_HEX2MEM_BASEADDR;
  endpaddr = CONFIG_SYSTEM_HEX2MEM_ENDPADDR;
  swap     = CONFIG_SYSTEM_HEX2MEM_SWAP;

  while ((option = getopt(argc, argv, ":hs:e:w:")) != ERROR)
    {
      switch (option)
        {
#ifdef CONFIG_SYSTEM_HEX2MEM_USAGE
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
      printf("ERROR: Missing required <hexfile> argument\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

  hexfile = argv[optind];
  optind++;

  if (optind < argc)
    {
      printf("ERROR: Garbage at end of command line\n");
      show_usage(argv[0], EXIT_FAILURE);
    }

  /* Check memory addresses */

  if (endpaddr <= baseaddr)
    {
      printf("ERROR: Memory end (+1) address must be AFTER memory base address\n");
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

  /* Wrap the FILE stream as a standard stream; wrap the memory as a memory
   * stream.
   */

  lib_stdinstream(&stdinstream, instream);
  lib_memsostream(&memoutstream, (FAR char *)baseaddr,
                  (int)(endpaddr - baseaddr));

  /* And do the deed */

  ret = hex2bin(&stdinstream.public, &memoutstream.public,
                (uint32_t)baseaddr, (uint32_t)endpaddr,
                (enum hex2bin_swap_e)swap);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: Failed to convert to binary: %d\n", ret);
    }

  /* Clean up and return */

  fclose(instream);
  return ret < 0 ? EXIT_FAILURE : EXIT_SUCCESS;
}

#endif /* CONFIG_SYSTEM_HEX2MEM_BUILTIN */
