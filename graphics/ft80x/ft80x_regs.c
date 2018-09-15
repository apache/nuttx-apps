/****************************************************************************
 * apps/graphics/ft80x/ft80x_regs.c
 *
 *   Copyright (C) 2018 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>
#include <sys/ioctl.h>
#include <stdint.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>

#include <nuttx/lcd/ft80x.h>

#include "ft80x.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Sleep for 10 MS between each REG_DLSWAP poll */

#define DLSWAP_DELAY_USEC   (10 * 1000)

/* But timeout after 5 seconds ( 5 * 100 * 10 MS = 5 S) */

#define DLSWAP_TIMEOUT_CSEC (5 * 100)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: ft80x_getreg8/16/32
 *
 * Description:
 *   Read an 8-, 16-, or 32-bit FT80x register value.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   addr   - The 32-bit aligned, 22-bit register address
 *   value  - The location to return the register value
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_getreg8(int fd, uint32_t addr, FAR uint8_t *value)
{
  struct ft80x_register_s reg;
  int ret;

  DEBUGASSERT(value != NULL && (addr & 3) == 0 && addr < 0xffc00000);

  /* Perform the IOCTL to get the register value */

  reg.addr = addr;
  ret      = ioctl(fd, FT80X_IOC_GETREG8, (unsigned long)((uintptr_t)&reg));

  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_GETREG8) failed: %d\n", errcode);
      ret = -errcode;
    }
  else
    {
      *value = reg.value.u8;
    }

  return ret;
}

int ft80x_getreg16(int fd, uint32_t addr, FAR uint16_t *value)
{
  struct ft80x_register_s reg;
  int ret;

  DEBUGASSERT(value != NULL && (addr & 3) == 0 && addr < 0xffc00000);

  /* Perform the IOCTL to get the register value */

  reg.addr = addr;
  ret      = ioctl(fd, FT80X_IOC_GETREG16, (unsigned long)((uintptr_t)&reg));

  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_GETREG16) failed: %d\n", errcode);
      ret = -errcode;
    }
  else
    {
      *value = reg.value.u16;
    }

  return ret;
}

int ft80x_getreg32(int fd, uint32_t addr, FAR uint32_t *value)
{
  struct ft80x_register_s reg;
  int ret;

  DEBUGASSERT(value != NULL && (addr & 3) == 0 && addr < 0xffc00000);

  /* Perform the IOCTL to get the register value */

  reg.addr = addr;
  ret      = ioctl(fd, FT80X_IOC_GETREG32, (unsigned long)((uintptr_t)&reg));

  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_GETREG32) failed: %d\n", errcode);
      ret = -errcode;
    }
  else
    {
      *value = reg.value.u32;
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_getregs
 *
 * Description:
 *   Read multiple 32-bit FT80x register values.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   addr   - The 32-bit aligned, 22-bit start register address
 *   nregs  - The number of registers to read.
 *   value  - The location to return the register values
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_getregs(int fd, uint32_t addr, uint8_t nregs, FAR uint32_t *value)
{
  struct ft80x_registers_s regs;
  int ret;

  DEBUGASSERT(value != NULL && (addr & 3) == 0 && addr < 0xffc00000);

  /* Perform the IOCTL to get the register value */

  regs.addr  = addr;
  regs.nregs = nregs;
  regs.value = value;

  ret = ioctl(fd, FT80X_IOC_GETREGS, (unsigned long)((uintptr_t)&regs));
  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_GETREGS) failed: %d\n", errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_putreg8/16/32
 *
 * Description:
 *   Wtite an 8-, 16-, or 32-bit FT80x register value.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   addr   - The 32-bit aligned, 22-bit register address
 *   value  - The register value to write.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_putreg8(int fd, uint32_t addr, uint8_t value)
{
  struct ft80x_register_s reg;
  int ret;

  DEBUGASSERT((addr & 3) == 0 && addr < 0xffc00000);

  /* Perform the IOCTL to get the register value */

  reg.addr     = addr;
  reg.value.u8 = value;

  ret = ioctl(fd, FT80X_IOC_PUTREG8, (unsigned long)((uintptr_t)&reg));
  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_PUTREG8) failed: %d\n", errcode);
      ret = -errcode;
    }

  return ret;
}

int ft80x_putreg16(int fd, uint32_t addr, uint16_t value)
{
  struct ft80x_register_s reg;
  int ret;

  DEBUGASSERT((addr & 3) == 0 && addr < 0xffc00000);

  /* Perform the IOCTL to get the register value */

  reg.addr      = addr;
  reg.value.u16 = value;

  ret = ioctl(fd, FT80X_IOC_PUTREG16, (unsigned long)((uintptr_t)&reg));
  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_PUTREG16) failed: %d\n", errcode);
      ret = -errcode;
    }

  return ret;
}

int ft80x_putreg32(int fd, uint32_t addr, uint32_t value)
{
  struct ft80x_register_s reg;
  int ret;

  DEBUGASSERT((addr & 3) == 0 && addr < 0xffc00000);

  /* Perform the IOCTL to get the register value */

  reg.addr      = addr;
  reg.value.u32 = value;

  ret = ioctl(fd, FT80X_IOC_PUTREG32, (unsigned long)((uintptr_t)&reg));
  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_PUTREG32) failed: %d\n", errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_putregs
 *
 * Description:
 *   Write multiple 32-bit FT80x register values.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   addr   - The 32-bit aligned, 22-bit start register address
 *   nregs  - The number of registers to write.
 *   value  - The of the register values to be written.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_putregs(int fd, uint32_t addr, uint8_t nregs,
                  FAR const uint32_t *value)
{
  struct ft80x_registers_s regs;
  int ret;

  DEBUGASSERT(value != NULL && (addr & 3) == 0 && addr < 0xffc00000);

  /* Perform the IOCTL to get the register value */

  regs.addr  = addr;
  regs.nregs = nregs;
  regs.value = (FAR uint32_t *)value;  /* Discard const qualifier */

  ret = ioctl(fd, FT80X_IOC_PUTREGS, (unsigned long)((uintptr_t)&regs));
  if (ret < 0)
    {
      int errcode = errno;
      ft80x_err("ERROR: ioctl(FT80X_IOC_PUTREGS) failed: %d\n", errcode);
      ret = -errcode;
    }

  return ret;
}

/****************************************************************************
 * Name: ft80x_dl_swap
 *
 * Description:
 *   Perform the display swap operation.
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_swap(int fd)
{
  unsigned int timeout;
  uint8_t regval8;
  int ret;

  /* Perform the DL swap */

  ret = ft80x_putreg8(fd, FT80X_REG_DLSWAP, DLSWAP_FRAME);
  if (ret < 0)
    {
      ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
      return ret;
    }

  /* Now wait for the swap to complete */

  timeout = 0;
  for (; ; )
    {
      /* Read REG_DLSWAP again */

      ret = ft80x_getreg8(fd, FT80X_REG_DLSWAP, &regval8);
      if (ret < 0)
        {
          ft80x_err("ERROR: ft80x_putreg8 failed: %d\n", ret);
          return ret;
        }

      /* Check if the swap is complete */

      if (regval8 == DLSWAP_DONE)
        {
          return OK;
        }

      if (++timeout > DLSWAP_TIMEOUT_CSEC)
        {
          ft80x_warn("WARNING: Timed out waiting for DLSWAP to complete\n");
          return -ETIMEDOUT;
        }

      /* No.. Wait a bit and try again */

      usleep(DLSWAP_DELAY_USEC);
    }
}
