/****************************************************************************
 * apps/system/ofloader/segger.c
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
#include <fcntl.h>
#include <stdio.h>
#include <mqueue.h>

#include <nuttx/init.h>

#include "ofloader.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define ONCHIP       1       /* On-chip Flash Memory */
#define ALGO_VERSION 0x101   /* Algo version, must not be modified. */

/****************************************************************************
 * Private Typess
 ****************************************************************************/

struct SECTOR_INFO
{
  uint32_t SectorSize;       /* Sector Size in bytes */
  uint32_t SectorStartAddr;  /* Start address of the sector area
                              * (relative to the "BaseAddr" of the flash)
                              */
};

struct FlashDevice
{
  uint16_t AlgoVer;
  uint8_t  Name[128];
  uint16_t Type;
  uint32_t BaseAddr;
  uint32_t TotalSize;
  uint32_t PageSize;
  uint32_t Reserved;
  uint8_t  ErasedVal;
  uint32_t TimeoutProg;
  uint32_t TimeoutErase;
  struct SECTOR_INFO SectorInfo[2];
};

const struct FlashDevice FlashDevice
locate_data("DevDscr") used_data =
{
  ALGO_VERSION,                      /* Algo version */
  "NuttX",                           /* Flash device name */
  ONCHIP,                            /* Flash device type */
  0x00000000,                        /* Flash base address */
  0xffffffff,                        /* Total flash device size in Bytes */
  CONFIG_SYSTEM_OFLOADER_BUFFERSIZE, /* Page Size (number of bytes that will
                                      * be passed to ProgramPage(). May be
                                      * multiple of min alignment in order
                                      * to reduce overhead for calling
                                      * ProgramPage multiple times
                                      */
  1,                                 /* Reserved, should be 1 */
  0xff,                              /* Flash erased value */
  5000,                              /* Program page timeout in ms */
  5000,                              /* Erase sector timeout in ms */

  /* Flash sector layout definition
   * Sector size equl with page size to skip erase
   */

  {
    {CONFIG_SYSTEM_OFLOADER_BUFFERSIZE, 0x00000000},
    {0xffffffff, 0xffffffff}
  }
};

/* This array is used to mark the stack and transfer buffer
 *locations used by the ofloader
 */

static uint8_t g_stack_buff[CONFIG_SYSTEM_OFLOADER_STACKSIZE +
                            CONFIG_SYSTEM_OFLOADER_BUFFERSIZE]
locate_data("DevStack") used_data;

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int SEGGER_FL_Prepare(uint32_t PreparePara0, uint32_t PreparePara1,
                      uint32_t PreparePara2)
  locate_code("PrgData") used_code;

int SEGGER_FL_Restore(uint32_t PreparePara0, uint32_t PreparePara1,
                      uint32_t PreparePara2)
  locate_code("PrgData") used_code;

int SEGGER_FL_Program(uint32_t DestAddr, uint32_t NumBytes,
                      FAR uint8_t *pSrcBuff)
  locate_code("PrgData") used_code;

int SEGGER_FL_Erase(uint32_t SectorAddr, uint32_t SectorIndex,
                    uint32_t NumSectors)
  locate_code("PrgData") used_code;

int SEGGER_FL_Read(uint32_t Addr, uint32_t NumBytes, FAR uint8_t *pDestBuff)
  locate_code("PrgData") used_code;

uint32_t SEGGER_FL_Verify(uint32_t Addr, uint32_t NumBytes,
                          FAR uint8_t *pData)
  locate_code("PrgData") used_code;

/****************************************************************************
 * Public data
 ****************************************************************************/

/* Reference SEGGER_FL_Prepare to prevent it from being
 * optimized by the compiler
 */

FAR void *g_create_idle = SEGGER_FL_Prepare;

static int SEGGER_FL_SendAndWait(FAR struct ofloader_msg *msg)
{
  int atcion = msg->atcion;
  mqd_t mq;
  int ret;

  mq = mq_open(OFLOADER_QNAME, O_RDWR, 0666, NULL);
  if (mq < 0)
    {
      return -errno;
    }

  ret = mq_send(mq, (FAR void *)&msg, sizeof(msg), 0);
  if (ret < 0)
    {
      ret = -errno;
      goto out;
    }

  /* TODO:SEGGER disables interrupts by default during operation
   * so wait ofloader main thread do somthing.
   */

  while (msg->atcion == atcion);
  if (msg->atcion == OFLOADER_ERROR)
    {
      ret = -EIO;
      goto out;
    }

out:
  mq_close(mq);
  return ret;
}

extern void __start(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int SEGGER_FL_Prepare(uint32_t PreparePara0, uint32_t PreparePara1,
                      uint32_t PreparePara2)
{
  g_create_idle = NULL;
  __start();
  return 0;
}

int SEGGER_FL_Restore(uint32_t PreparePara0, uint32_t PreparePara1,
                      uint32_t PreparePara2)
{
  struct ofloader_msg msg;

  msg.atcion = OFLOADER_SYNC;
  return SEGGER_FL_SendAndWait(&msg);
}

int SEGGER_FL_Program(uint32_t DestAddr, uint32_t NumBytes,
                      FAR uint8_t *pSrcBuff)
{
  struct ofloader_msg msg;

  msg.addr = DestAddr;
  msg.size = NumBytes;
  msg.buff = pSrcBuff;
  msg.atcion = OFLOADER_WRITE;
  return SEGGER_FL_SendAndWait(&msg);
}

int SEGGER_FL_Erase(uint32_t SectorAddr, uint32_t SectorIndex,
                    uint32_t NumSectors)
{
  return 0;
}

int SEGGER_FL_Read(uint32_t Addr, uint32_t NumBytes, FAR uint8_t *pDestBuff)
{
  struct ofloader_msg msg;

  msg.addr = Addr;
  msg.size = NumBytes;
  msg.buff = pDestBuff;
  msg.atcion = OFLOADER_READ;
  return SEGGER_FL_SendAndWait(&msg);
}

uint32_t SEGGER_FL_Verify(uint32_t Addr, uint32_t NumBytes,
                          FAR uint8_t *pData)
{
  struct ofloader_msg msg;

  msg.addr = Addr;
  msg.size = NumBytes;
  msg.buff = pData;
  msg.atcion = OFLOADER_VERIFY;
  if (SEGGER_FL_SendAndWait(&msg) < 0)
    {
      return 0;
    }

  return Addr + NumBytes;
}
