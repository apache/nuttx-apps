/****************************************************************************
 * apps/system/nxdiag/nxdiag.c
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

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>

#include <nuttx/config.h>
#include <nuttx/version.h>

#include "sysinfo.h"
#ifdef CONFIG_SYSTEM_NXDIAG_ESPRESSIF_CHIP_WO_TOOL
#include <fcntl.h>
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Valid Command Line Arguments */

static const char *g_valid_args[] =
{
  "-h",
  "--help",
  "-n",
  "--nuttx",
#ifdef CONFIG_SYSTEM_NXDIAG_COMP_FLAGS
  "-f",
  "--flags",
#endif
#ifdef CONFIG_SYSTEM_NXDIAG_CONF
  "-c",
  "--config",
#endif
  "-o",
  "--host-os",
#ifdef CONFIG_SYSTEM_NXDIAG_HOST_PATH
  "-p",
  "--host-path",
#endif
#ifdef CONFIG_SYSTEM_NXDIAG_HOST_PACKAGES
  "-k",
  "--host-packages",
#endif
#ifdef CONFIG_SYSTEM_NXDIAG_HOST_MODULES
  "-m",
  "--host-modules",
#endif
  "-v",
  "--vendor-specific",
  "--all"
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: search_str_in_arr
 *
 * Description:
 *   Search for a string in an array of strings.
 *
 * Input Parameters:
 *   size - Size of the array.
 *   arr  - Array of strings.
 *   str  - String to search for.
 *
 * Returned Value:
 *   True if string is found, false otherwise.
 *
 ****************************************************************************/

bool search_str_in_arr(int size, char *arr[], char *str)
{
  int i;

  for (i = 0; i < size; i++)
    {
      if (strcmp(arr[i], str) == 0)
        {
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Name: are_valid_args
 *
 * Description:
 *   Check if provided command line arguments are valid. Valid arguments are
 *   defined in g_valid_args array.
 *
 * Input Parameters:
 *   argc - Number of arguments.
 *   argv - Array of arguments.
 *
 * Returned Value:
 *   True if all arguments are valid, false otherwise.
 *
 ****************************************************************************/

bool are_valid_args(int argc, char *argv[])
{
  int i;

  for (i = 1; i < argc; i++)
    {
      if (!search_str_in_arr(nitems(g_valid_args),
                             (char **)g_valid_args, argv[i]))
        {
          return false;
        }
    }

  return true;
}

/****************************************************************************
 * Name: print_array
 *
 * Description:
 *   Print a constant array of strings.
 *
 * Input Parameters:
 *   arr  - Array of strings.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

void print_array(const char *arr[], int size)
{
  int i;

  for (i = 0; i < size; i++)
    {
      printf("\t%s\n", arr[i]);
    }

  printf("\n");
}

/****************************************************************************
 * Name: print_array
 *
 * Description:
 *   Print an array of strings.
 *
 * Input Parameters:
 *   arr  - Array of strings.
 *
 * Returned Value:
 *   None.
 *
 ****************************************************************************/

static void print_usage(char *prg)
{
  fprintf(stderr, "%s - Get host and target system debug information.\n",
          prg);
  fprintf(stderr, "\nUsage: %s [options]\n", prg);
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "         -h                       \
          Show this message\n");
  fprintf(stderr, "         -n, --nuttx              \
          Output the NuttX operational system information.\n");
#ifdef CONFIG_SYSTEM_NXDIAG_COMP_FLAGS
  fprintf(stderr, "         -f, --flags              \
          Output the NuttX compilation and linker flags used.\n");
#endif
#ifdef CONFIG_SYSTEM_NXDIAG_CONF
  fprintf(stderr, "         -c, --config             \
          Output the NuttX configuration options used.\n");
#endif
  fprintf(stderr, "         -o, --host-os            \
          Output the host system operational system information.\n");
#ifdef CONFIG_SYSTEM_NXDIAG_HOST_PATH
  fprintf(stderr, "         -p, --host-path          \
          Output the host PATH environment variable.\n");
#endif
#ifdef CONFIG_SYSTEM_NXDIAG_HOST_PACKAGES
  fprintf(stderr, "         -k, --host-packages      \
          Output the host installed system packages.\n");
#endif
#ifdef CONFIG_SYSTEM_NXDIAG_HOST_MODULES
  fprintf(stderr, "         -m, --host-modules       \
          Output the host installed Python modules.\n");
#endif
  fprintf(stderr, "         -v, --vendor-specific    \
          Output vendor specific information.\n");
  fprintf(stderr, "         --all                    \
          Output all available information.\n");
  fprintf(stderr, "\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
#ifdef CONFIG_SYSTEM_NXDIAG_ESPRESSIF_CHIP_WO_TOOL
  int fd;
  int ret;
#endif

  if (argc == 1 || !are_valid_args(argc, argv))
    {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }

  if (search_str_in_arr(argc, argv, "-h") ||
      search_str_in_arr(argc, argv, "--help"))
    {
      print_usage(argv[0]);
      exit(EXIT_SUCCESS);
    }

  printf("Nxdiag Report:\n\n");

  /* NuttX Info */

  if (search_str_in_arr(argc, argv, "-n") ||
      search_str_in_arr(argc, argv, "--nuttx") ||
      search_str_in_arr(argc, argv, "--all"))
    {
      char hostname[HOST_NAME_MAX];
      gethostname(hostname, HOST_NAME_MAX);
      hostname[HOST_NAME_MAX - 1] = '\0';

      printf("NuttX RTOS info:\n");
      printf("\tHostname: %s\n", hostname);
      printf("\tRelease: %s\n", CONFIG_VERSION_STRING);
#if defined(__DATE__) && defined(__TIME__)
      printf("\tBuild: %s %s %s\n",
             CONFIG_VERSION_BUILD, __DATE__, __TIME__);
#else
      printf("\tBuild: %s\n", CONFIG_VERSION_BUILD);
#endif
      printf("\tArch: %s\n", CONFIG_ARCH);
      printf("\tDefconfig: %s\n\n", CONFIG_BASE_DEFCONFIG);
    }

#ifdef CONFIG_SYSTEM_NXDIAG_COMP_FLAGS
  if (search_str_in_arr(argc, argv, "-f") ||
      search_str_in_arr(argc, argv, "--flags") ||
      search_str_in_arr(argc, argv, "--all"))
    {
      printf("NuttX CFLAGS:\n");
      print_array(NUTTX_CFLAGS, NUTTX_CFLAGS_ARRAY_SIZE);
      printf("NuttX CXXFLAGS:\n");
      print_array(NUTTX_CXXFLAGS, NUTTX_CXXFLAGS_ARRAY_SIZE);
      printf("NuttX LDFLAGS:\n");
      print_array(NUTTX_LDFLAGS, NUTTX_LDFLAGS_ARRAY_SIZE);
    }
#endif

#ifdef CONFIG_SYSTEM_NXDIAG_CONF
  if (search_str_in_arr(argc, argv, "-c") ||
      search_str_in_arr(argc, argv, "--config") ||
      search_str_in_arr(argc, argv, "--all"))
    {
      printf("NuttX configuration options:\n");
      print_array(NUTTX_CONFIG, NUTTX_CONFIG_ARRAY_SIZE);
    }
#endif

  /* Host Info */

  if (search_str_in_arr(argc, argv, "-o") ||
      search_str_in_arr(argc, argv, "--host-os") ||
      search_str_in_arr(argc, argv, "--all"))
    {
      printf("Host system OS:\n");
      printf("\t%s\n\n", OS_VERSION);
    }

#ifdef CONFIG_SYSTEM_NXDIAG_HOST_PATH
  if (search_str_in_arr(argc, argv, "-p") ||
      search_str_in_arr(argc, argv, "--host-path") ||
      search_str_in_arr(argc, argv, "--all"))
    {
      printf("Host system PATH:\n");
      print_array(SYSTEM_PATH, SYSTEM_PATH_ARRAY_SIZE);
    }
#endif

#ifdef CONFIG_SYSTEM_NXDIAG_HOST_PACKAGES
  if (search_str_in_arr(argc, argv, "-k") ||
      search_str_in_arr(argc, argv, "--host-packages") ||
      search_str_in_arr(argc, argv, "--all"))
    {
      printf("Host system installed packages:\n");
      print_array(INSTALLED_PACKAGES, INSTALLED_PACKAGES_ARRAY_SIZE);
    }
#endif

#ifdef CONFIG_SYSTEM_NXDIAG_HOST_MODULES
  if (search_str_in_arr(argc, argv, "-m") ||
      search_str_in_arr(argc, argv, "--host-modules") ||
      search_str_in_arr(argc, argv, "--all"))
    {
      printf("Host system installed python modules:\n");
      print_array(PYTHON_MODULES, PYTHON_MODULES_ARRAY_SIZE);
    }
#endif

  /* Vendor Specific Info */

  if (search_str_in_arr(argc, argv, "-v") ||
      search_str_in_arr(argc, argv, "--vendor-specific") ||
      search_str_in_arr(argc, argv, "--all"))
    {
      /* Please don't forget to add the vendor specific information
       * in alphabetical order. Also, please update the documentation
       * in Documentation/applications/nxdiag
       */

#ifdef CONFIG_SYSTEM_NXDIAG_ESPRESSIF
      printf("Espressif specific information:\n\n");
      printf("Bootloader version:\n");
      print_array(ESPRESSIF_BOOTLOADER, ESPRESSIF_BOOTLOADER_ARRAY_SIZE);
      printf("Toolchain version:\n");
      print_array(ESPRESSIF_TOOLCHAIN, ESPRESSIF_TOOLCHAIN_ARRAY_SIZE);
      printf("Esptool version: %s\n\n", ESPRESSIF_ESPTOOL);
      printf("HAL version: %s\n\n", ESPRESSIF_HAL);
#ifdef CONFIG_SYSTEM_NXDIAG_ESPRESSIF_CHIP
#ifndef CONFIG_SYSTEM_NXDIAG_ESPRESSIF_CHIP_WO_TOOL
      printf("CHIP ID: %s\n\n", ESPRESSIF_CHIP_ID);
      printf("Flash ID:\n");
      print_array(ESPRESSIF_FLASH_ID, ESPRESSIF_FLASH_ID_ARRAY_SIZE);
      printf("Security information:\n");
      print_array(ESPRESSIF_SECURITY_INFO,
                  ESPRESSIF_SECURITY_INFO_ARRAY_SIZE);
      printf("Flash status: %s\n\n", ESPRESSIF_FLASH_STAT);
      printf("MAC address: %s\n\n", ESPRESSIF_MAC_ADDR);
#else
      fd = open("/dev/nxdiag", O_RDONLY);
      if (fd < 0)
        {
          printf("Failed to open device\n");
          return ERROR;
        }

      ret = read(fd, ESPRESSIF_INFO, ESPRESSIF_INFO_SIZE);
      if (ret <= 0)
        {
          printf("Failed to read device\n");
          return ERROR;
        }

      printf("%s\n\n", ESPRESSIF_INFO);

#endif /* CONFIG_SYSTEM_NXDIAG_ESPRESSIF_CHIP_WO_TOOL */
#endif /* CONFIG_SYSTEM_NXDIAG_ESPRESSIF_CHIP */
#endif /* CONFIG_SYSTEM_NXDIAG_ESPRESSIF */
    }

  return 0;
}
