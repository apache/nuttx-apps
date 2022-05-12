/****************************************************************************
 * apps/epaper/write_dta/writeDta_main.c
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

#include <sys/ioctl.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * epwritedta_main
 ****************************************************************************/

static uint8_t hex_val(char symbol)
{
  if (symbol >= '0' && symbol <= '9')
    return symbol - '0';

  if (symbol >= 'a' && symbol <= 'f')
    return 10 + symbol - 'a';

  if (symbol >= 'A' && symbol <= 'F')
	return 10 + symbol - 'A';
  return 0;
}

int main(int argc, FAR char *argv[])
{
  uint8_t buffer[16];
  int buf_len = 0;

  struct ssd1306_dev_s *priv = ssd1306_get_device();// board_ssd1306_getdev();

  if (argc != 2)
    {
      printf("usage %s 0x1234\nMax hex len is %d\n", argv[0], (int)(2 * sizeof(buffer)));
      return -1;
    }
  char *tmp = argv[1];

  if (*tmp == '0' && *(tmp+1) == 'x')
	  tmp+=2;
  while (*tmp != '\0' && *(tmp+1) != '\0')
    {
	  if (buf_len >= sizeof(buffer)) break;
	  buffer[buf_len] = 16*hex_val(*tmp) + hex_val(*(tmp+1));
	  buf_len++;
	  tmp+= 2;
    }

  printf("Sending buffer with %d bytes\n", buf_len);

  ssd1306_cmddata(priv, false);
  ssd1306_select(priv, true);
  ssd1306_sendblk(priv, buffer, buf_len);
  ssd1306_select(priv, false);

  return 0;
}
