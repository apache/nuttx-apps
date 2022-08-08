/****************************************************************************
 * apps/system/cfgdata/cfgdata_main.c
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
#include <nuttx/mtd/mtd.h>
#include <nuttx/mtd/configdata.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

/****************************************************************************
 * Private data
 ****************************************************************************/

static const char *g_config_dev = "/dev/config";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Print usage information
 ****************************************************************************/

static void cfgdatacmd_help(void)
{
  printf("\nUsage: cfgdata <cmd> [arguments]\n");
  printf("Where <cmd> is one of:\n\n");
  printf("  all:    show all config entries\n");
  printf("  print:  display a specific config entry\n");
  printf("  set:    set or change a config entry\n");
  printf("  unset:  delete a config entry\n");
  printf("  format: delete all config entries\n\n");

  printf("Syntax for 'set' cmd:\n");
#ifdef CONFIG_MTD_CONFIG_NAMED
  printf("  set name [hex_byte,hex_byte,etc.]\n");
  printf("  set name \"string\"\n\n");
#else
  printf("  set id,instance [bytes]\n");
  printf("  set id,instance \"string\"\n\n");
#endif

  printf("Syntax for 'print' cmd:\n");
#ifdef CONFIG_MTD_CONFIG_NAMED
  printf("  print name\n");
#else
  printf("  print id,instance\n");
#endif

  printf("Syntax for 'unset' cmd:\n");
#ifdef CONFIG_MTD_CONFIG_NAMED
  printf("  unset name\n");
#else
  printf("  unset id,instance\n");
#endif
}

/****************************************************************************
 * Parse out the id,inst,name tokens
 ****************************************************************************/

#ifndef CONFIG_MTD_CONFIG_NAMED
static int cfgdatacmd_idtok(int startpos, char *token)
{
  while (token[startpos] != ',' && token[startpos] != '\0')
    {
      startpos++;
    }

  if (token[startpos] != ',')
    {
      /* Error in format */

      printf("Expected config identifier in 'id,instance' format\n");
      return 0;
    }

  /* return, skipping the ',' */

  return startpos + 1;
}
#endif

/****************************************************************************
 * Set a config item value
 *
 * config set 1,0,wr_width_cs0 [0x3]
 * config set 1,1,wr_width_cs1 [0x2]
 *
 ****************************************************************************/

static void cfgdatacmd_parse_byte_array(struct config_data_s *cfg,
                                        int argc, char *argv[])
{
  int   x;
  int   c;
  int   count;
  int   val;

  /* Start with arg3 */

  x = 3;
  c = 1;
  count = 0;

  /* Loop for all remaining arguments until ']' found */

  while (x < argc && argv[x][c] != ']' && argv[x][c] != '\0')
    {
      /* Count this item */

      count++;

      /* Skip to next item */

      while (argv[x][c] != ',' && argv[x][c] != ']' && argv[x][c] != 0)
        {
          c++;
        }

      /* Test for comma separator */

      if (argv[x][c] == ',')
        {
          c++;
        }

      /* Test for space separated items */

      if (argv[x][c] == 0)
        {
          x++;
          c = 0;
        }
    }

  /* Test if static stack space allocation is big enough for data */

  if (count > cfg->len)
    {
      /* Perform dynamic memory allocation */

      cfg->configdata = (FAR uint8_t *)malloc(count);
      cfg->len = count;
    }

  /* Count determined. Start with arg3 again and parse the bytes */

  x = 3;
  c = 1;
  count = 0;

  /* Loop for all remaining arguments until ']' found */

  while (x < argc && argv[x][c] != ']' && argv[x][c] != '\0')
    {
      /* Parse this item */

      if (strncmp(&argv[x][c], "0x", 2) == 0)
        {
          /* Hex byte */

          sscanf(&argv[x][c + 2], "%x", &val);
          cfg->configdata[count] = (uint8_t)val;
        }
      else
        {
          /* Decimal value */

          cfg->configdata[count] = (uint8_t)atoi(&argv[x][c]);
        }

      /* Increment the count */

      count++;

      /* Skip to next item */

      while (argv[x][c] != ',' && argv[x][c] != ']' && argv[x][c] != 0)
        {
          c++;
        }

      /* Test for comma separator */

      if (argv[x][c] == ',')
        {
          c++;
        }

      /* Test for space separated items */

      if (argv[x][c] == 0)
        {
          x++;
          c = 0;
        }
    }

  cfg->len = count;
}

/****************************************************************************
 * Set a config item value
 *
 * config set 1,0,wr_width_cs0 [0x3]
 * config set 1,1,wr_width_cs1 [0x2]
 *
 ****************************************************************************/

static void cfgdatacmd_set(int argc, char *argv[])
{
  int                   ret;
  int                   fd;
  int                   x;
  struct config_data_s  cfg;
  uint8_t               data[32];

#ifdef CONFIG_MTD_CONFIG_NAMED

  /* Copy the name to the cfg struct */

  strncpy(cfg.name, argv[2], CONFIG_MTD_CONFIG_NAME_LEN);

#else

  /* Parse the id and instance */

  cfg.id = atoi(argv[2]);

  /* Advance past ',' to instance number */

  x = cfgdatacmd_idtok(0, argv[2]);
  if (x == 0)
    {
      return;
    }

  /* Convert instance to integer */

  cfg.instance = atoi(&argv[2][x]);
#endif

  /* Test if data is an array of bytes or simple string */

  if (argv[3][0] == '[')
    {
      /* It is an array of bytes.  Count the number of bytes */

      cfg.configdata = data;
      cfg.len = sizeof(data);
      cfgdatacmd_parse_byte_array(&cfg, argc, argv);
    }
  else
    {
      bool isnumber = true;

      /* It is a simple string.  Test if it looks like a number */

      cfg.configdata = data;
      if (strncmp(argv[3], "0x", 2) == 0)
        {
          /* Test for all hex digit values */

          for (x = 2; x < strlen(argv[3]); x++)
            {
              if (!isxdigit(argv[3][x]))
                {
                  isnumber = false;
                  break;
                }
            }

          if (isnumber)
            {
              sscanf(&argv[3][2], "%" SCNx32, (int32_t *)&cfg.configdata);
              cfg.len = 4;
            }
        }
      else
        {
          /* Test for all hex digit values */

          for (x = 0; x < strlen(argv[3]); x++)
            {
              if (!isdigit(argv[3][x]))
                {
                  isnumber = false;
                  break;
                }
            }

          if (isnumber)
            {
              int32_t temp = atoi(argv[3]);
              *((int32_t *)cfg.configdata) = temp;
              cfg.len = 4;
            }
        }

      if (!isnumber)
        {
          /* Point to the string and calculate the length */

          cfg.configdata = (FAR uint8_t *)argv[3];
          cfg.len = strlen(argv[3]) + 1;
        }
    }

  /* Now open the /dev/config file and set the config item */

  if ((fd = open(g_config_dev, 0)) < 2)
    {
      /* Display error */

      printf("error: unable to open %s\n", g_config_dev);
      return;
    }

  ret = ioctl(fd, CFGDIOC_SETCONFIG, (unsigned long)(uintptr_t)&cfg);

  /* Close the file and report error if any */

  close(fd);
  if (ret != OK)
    {
      printf("Error %d setting config entry\n", errno);
    }

  /* Free the cfg.configdata if needed */

  if (cfg.configdata != (FAR uint8_t *)argv[3] &&
      cfg.configdata != data)
    {
      free(cfg.configdata);
    }
}

/****************************************************************************
 * Unset a config item value
 ****************************************************************************/

static void cfgdatacmd_unset(int argc, char *argv[])
{
  int                   ret;
  int                   fd;
  struct config_data_s  cfg;

#ifdef CONFIG_MTD_CONFIG_NAMED
  /* Copy the name to the cfg struct */

  strncpy(cfg.name, argv[2], CONFIG_MTD_CONFIG_NAME_LEN);

#else
  int                   x;

  /* Parse the id and instance */

  cfg.id = atoi(argv[2]);

  /* Advance past ',' to instance number */

  x = cfgdatacmd_idtok(0, argv[2]);
  if (x == 0)
    {
      return;
    }

  /* Convert instance to integer */

  cfg.instance = atoi(&argv[2][x]);
#endif

  cfg.configdata = NULL;
  cfg.len = 0;

  /* Try to open the /dev/config file */

  if ((fd = open(g_config_dev, 0)) < 2)
    {
      /* Display error */

      printf("error: unable to open %s\n", g_config_dev);
      return;
    }

  /* Delete the config item */

  ret = ioctl(fd, CFGDIOC_DELCONFIG, (unsigned long)(uintptr_t)&cfg);
  close(fd);

  if (ret != OK)
    {
      printf("Error deletign config entry '%s'\n", argv[2]);
    }
}

/****************************************************************************
 * Print a config item value
 *
 * config print 1,1
 * config print wr_width_cs0
 *
 ****************************************************************************/

static void cfgdatacmd_print(int argc, char *argv[])
{
  int                   ret;
  int                   fd;
  int                   x;
  struct config_data_s  cfg;
  bool                  isstring;

#ifdef CONFIG_MTD_CONFIG_NAMED

  /* Copy the name to the cfg struct */

  strncpy(cfg.name, argv[2], CONFIG_MTD_CONFIG_NAME_LEN);

#else

  /* Parse the id and instance */

  cfg.id = atoi(argv[2]);

  /* Advance past ',' to instance number */

  x = cfgdatacmd_idtok(0, argv[2]);
  if (x == 0)
    {
      return;
    }

  /* Convert instance to integer */

  cfg.instance = atoi(&argv[2][x]);
#endif

  /* Try to open the /dev/config file */

  if ((fd = open(g_config_dev, O_RDONLY)) < 2)
    {
      /* Display error */

      printf("error: unable to open %s\n", g_config_dev);
      return;
    }

  cfg.configdata = (FAR uint8_t *)malloc(256);
  cfg.len = 256;
  if (cfg.configdata == NULL)
    {
      printf("Error allocating buffer\n");
      return;
    }

  /* Get the config item */

  ret = ioctl(fd, CFGDIOC_GETCONFIG, (unsigned long)(uintptr_t)&cfg);
  close(fd);

  if (ret != OK)
    {
      printf("Error reading config entry '%s'\n", argv[2]);
      free(cfg.configdata);
      return;
    }

  /* Display the data */

  isstring = cfg.configdata[cfg.len - 1] == 0;
  for (x = 0; x < cfg.len - 1; x++)
    {
      /* Test for all ascii characters */

      if (cfg.configdata[x] < ' ' || cfg.configdata[x] > '~')
        {
          isstring = false;
          break;
        }
    }

  /* Display the data */

  if (isstring)
    {
      printf("%s\n", cfg.configdata);
    }
  else
    {
      /* Loop though all bytes and display them */

      for (x = 0; x < cfg.len; x++)
        {
          /* Print the next byte */

          printf("0x%02X ", cfg.configdata[x]);

          if (((x + 1) & 7) == 0 && x + 1 != cfg.len)
            {
              printf("\n");
            }
        }

      printf("\n");
    }

  free(cfg.configdata);
}

/****************************************************************************
 * Enumerate and display all config items
 ****************************************************************************/

static void cfgdatacmd_show_all_config_items(void)
{
  int                   ret;
  int                   fd;
  int                   x;
  struct config_data_s  cfg;
  char                  fmtstr[24];
  bool                  isstring;

  /* Try to open the /dev/config file */

  if ((fd = open(g_config_dev, 0)) < 2)
    {
      /* Display error */

      printf("error: unable to open %s\n", g_config_dev);
      return;
    }

  /* Print header */

#ifdef CONFIG_MTD_CONFIG_NAMED
  sprintf(fmtstr, "%%-%ds%%-6sData\n", CONFIG_MTD_CONFIG_NAME_LEN);
  printf(fmtstr, "Name", "Len");
  sprintf(fmtstr, "%%-%ds%%-6d", CONFIG_MTD_CONFIG_NAME_LEN);
#else
  strcpy(fmtstr, "%-6s%-6s%-6sData\n");
  printf(fmtstr, "ID", "Inst", "Len");
  strcpy(fmtstr, "%-6d%-6d%-6d");
#endif

  /* Get the first config item */

  cfg.configdata = (FAR uint8_t *)malloc(256);
  cfg.len = 256;
  if (cfg.configdata == NULL)
    {
      printf("Error allocating buffer\n");
      return;
    }

  ret = ioctl(fd, CFGDIOC_FIRSTCONFIG, (unsigned long)(uintptr_t)&cfg);

  while (ret != -1)
    {
      /* Print this entry */

#ifdef CONFIG_MTD_CONFIG_NAMED
      printf(fmtstr, cfg.name, cfg.len);
#else
      printf(fmtstr, cfg.id, cfg.instance, cfg.len);
#endif

      /* Test if data is a string */

      isstring = cfg.configdata[cfg.len - 1] == 0;
      for (x = 0; x < cfg.len - 1; x++)
        {
          /* Test for all ascii characters */

          if (cfg.configdata[x] < ' ' || cfg.configdata[x] > '~')
            {
              isstring = false;
              break;
            }
        }

      /* Display the data */

      if (isstring)
        {
          printf("%s\n", cfg.configdata);
        }
      else
        {
          char fmtstr2[10];

#ifdef CONFIG_MTD_CONFIG_NAMED
          sprintf(fmtstr2, "\n%ds", CONFIG_MTD_CONFIG_NAME_LEN + 6);
#else
          strcpy(fmtstr2, "\n%18s");
#endif
          /* Loop though all bytes and display them */

          for (x = 0; x < cfg.len; x++)
            {
              /* Print the next byte */

              printf("0x%02X ", cfg.configdata[x]);

              if (((x + 1) & 7) == 0 && x + 1 != cfg.len)
                {
                  printf(fmtstr2, " ");
                }
            }

          printf("\n");
        }

      /* Get the next config item */

      cfg.len = 256;
      ret = ioctl(fd, CFGDIOC_NEXTCONFIG, (unsigned long)(uintptr_t)&cfg);
    }

  close(fd);
  free(cfg.configdata);
}

/****************************************************************************
 * Erase all config items
 ****************************************************************************/

static void cfgdatacmd_format(void)
{
  int fd;
  int ret;

  /* Try to open the /dev/config file */

  if ((fd = open(g_config_dev, 0)) < 2)
    {
      /* Display error */

      printf("error: unable to open %s\n", g_config_dev);
      return;
    }

  ret = ioctl(fd, MTDIOC_BULKERASE, 0);
  close(fd);

  if (ret != OK)
    {
      printf("Error %d config format\n", errno);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  /* Argument given? */

  if (argc == 1)
    {
      /* Show usage info and exit */

      cfgdatacmd_help();
      return 0;
    }

  /* Test for "all" cmd */

  if (strcmp(argv[1], "all") == 0)
    {
      /* Print the existing config items */

      cfgdatacmd_show_all_config_items();
      return 0;
    }

  /* Test for "set" cmd */

  if (strcmp(argv[1], "set") == 0)
    {
      if (argc < 4)
        {
          printf("At least 2 arguments needed for 'set' command\n");
          return 0;
        }

      /* Call the routine to set a config item */

      cfgdatacmd_set(argc, argv);
      return 0;
    }

  /* Test for "print" cmd */

  if (strcmp(argv[1], "print") == 0)
    {
      /* Test for print all */

      if (strcmp(argv[2], "all") == 0)
        {
          cfgdatacmd_show_all_config_items();
        }
      else
        {
          /* Call the routine to print a config item */

          cfgdatacmd_print(argc, argv);
        }

      return 0;
    }

  /* Test for "unset" cmd */

  if (strcmp(argv[1], "unset") == 0)
    {
      if (argc < 3)
        {
          printf("Need 1 argument for 'unset' command\n");
          return 0;
        }

      /* Call the routine to set a config item */

      cfgdatacmd_unset(argc, argv);
      return 0;
    }

  /* Test for "format" cmd */

  if (strcmp(argv[1], "format") == 0)
    {
      /* Call the routine to erase all config items */

      cfgdatacmd_format();
      return 0;
    }

  /* Unknown cmd */

  printf("Unknown config command: %s\n", argv[1]);

  return 0;
}
