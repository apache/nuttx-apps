/****************************************************************************
 * apps/graphics/ft80x/ft80x.h
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

#ifndef __APPS_GRAPHICS_FT80X_FT80X_H
#define __APPS_GRAPHICS_FT80X_FT80X_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include <stdio.h>
#include <stdint.h>
#include <debug.h>

#ifdef CONFIG_GRAPHICS_FT80X

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* NOTE: These rely on internal definitions from compiler.h and debug.h.
 * Could be a porting issue.
 */

#if !defined(CONFIG_GRAPHICS_FT80X_DEBUG_ERROR)
#  define ft80x_err _none
#elif defined(CONFIG_CPP_HAVE_VARARGS)
#  define ft80x_err(format, ...) \
     fprintf(stderr, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#else
#  define ft80x_err printf
#endif

#if !defined(CONFIG_GRAPHICS_FT80X_DEBUG_WARN)
#  define ft80x_warn _none
#elif defined(CONFIG_CPP_HAVE_VARARGS)
#  define ft80x_warn(format, ...) \
     fprintf(stderr, EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#else
#  define ft80x_warn printf
#endif

#if !defined(CONFIG_GRAPHICS_FT80X_DEBUG_INFO)
#  define ft80x_info _none
#elif defined(CONFIG_CPP_HAVE_VARARGS)
#  define ft80x_info(format, ...) \
     printf(EXTRA_FMT format EXTRA_ARG, ##__VA_ARGS__)
#else
#  define ft80x_info printf
#endif

#define FT80X_CMDFIFO_MASK  (FT80X_CMDFIFO_SIZE - 1)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

struct ft80x_dlbuffer_s;  /* Forward reference3 */

/****************************************************************************
 * Name: ft80x_getreg8/16/32
 *
 * Description:
 *   Read an 8-, 16-, or 32-bit FT80x register value.
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   addr  - The 32-bit aligned, 22-bit register value
 *   value - The location to return the register value
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_getreg8(int fd, uint32_t addr, FAR uint8_t *value);
int ft80x_getreg16(int fd, uint32_t addr, FAR uint16_t *value);
int ft80x_getreg32(int fd, uint32_t addr, FAR uint32_t *value);

/****************************************************************************
 * Name: ft80x_getregs
 *
 * Description:
 *   Read multiple 32-bit FT80x register values.
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   addr  - The 32-bit aligned, 22-bit start register address
 *   nregs - The number of registers to read.
 *   value - The location to return the register values
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_getregs(int fd, uint32_t addr, uint8_t nregs, FAR uint32_t *value);

/****************************************************************************
 * Name: ft80x_putreg8/16/32
 *
 * Description:
 *   Wtite an 8-, 16-, or 32-bit FT80x register value.
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   addr  - The 32-bit aligned, 22-bit register value
 *   value - The register value to write.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_putreg8(int fd, uint32_t addr, uint8_t value);
int ft80x_putreg16(int fd, uint32_t addr, uint16_t value);
int ft80x_putreg32(int fd, uint32_t addr, uint32_t value);

/****************************************************************************
 * Name: ft80x_putregs
 *
 * Description:
 *   Write multiple 32-bit FT80x register values.
 *
 * Input Parameters:
 *   fd    - The file descriptor of the FT80x device.  Opened by the caller
 *           with write access.
 *   addr  - The 32-bit aligned, 22-bit start register address
 *   nregs - The number of registers to write.
 *   value - The of the register values to be written.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_putregs(int fd, uint32_t addr, uint8_t nregs,
                  FAR const uint32_t *value);

/****************************************************************************
 * Name: ft80x_ramdl_rewind
 *
 * Description:
 *   Reset to the start of RAM DL memory
 *
 * Input Parameters:
 *   fd - The file descriptor of the FT80x device.  Opened by the caller
 *        with write access.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramdl_rewind(int fd);

/****************************************************************************
 * Name: ft80x_ramdl_append
 *
 * Description:
 *   Append new display list data to RAM DL
 *
 * Input Parameters:
 *   fd   - The file descriptor of the FT80x device.  Opened by the caller
 *          with write access.
 *   data - A pointer to the start of the data to be written.
 *   len  - The number of bytes to be written.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramdl_append(int fd, FAR const void *data, size_t len);

/****************************************************************************
 * Name: ft80x_ramcmd_append
 *
 * Description:
 *   Append new display list data to RAM CMD
 *
 * Input Parameters:
 *   fd   - The file descriptor of the FT80x device.  Opened by the caller
 *          with write access.
 *   data - A pointer to the start of the data to be append to RAM CMD.
 *   len  - The number of bytes to be appended.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramcmd_append(int fd, FAR const void *data, size_t len);

/****************************************************************************
 * Name: ft80x_ramcmd_freespace
 *
 * Description:
 *   Return the free space in RAM CMD memory
 *
 * Input Parameters:
 *   fd     - The file descriptor of the FT80x device.  Opened by the caller
 *            with write access.
 *   offset - Pointer to location to return the write offset to use if the
 *            FIFO is not full.
 *   avail  - Pointer to location to return the FIFO free space
 *
 * Returned Value:
 *   The (positive) number of free bytes in RAM CMD on success.  A negated
 *   errno value is returned on any failure.
 *
 ****************************************************************************/

uint16_t ft80x_ramcmd_freespace(int fd, FAR uint16_t *offset,
                                FAR uint16_t *avail);

/****************************************************************************
 * Name: ft80x_ramcmd_waitfifoempty
 *
 * Description:
 *   Wait until co processor completes the operation and the FIFO is again
 *   empty.
 *
 * Input Parameters:
 *   fd - The file descriptor of the FT80x device.  Opened by the caller
 *        with write access.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_ramcmd_waitfifoempty(int fd);

/****************************************************************************
 * Name: ft80x_dl_swap
 *
 * Description:
 *   Perform the display swap operation.
 *
 * Input Parameters:
 *   fd - The file descriptor of the FT80x device.  Opened by the caller
 *        with write access.
 *
 * Returned Value:
 *   Zero (OK) on success.  A negated errno value on failure.
 *
 ****************************************************************************/

int ft80x_dl_swap(int fd);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* CONFIG_GRAPHICS_FT80X */
#endif /* __APPS_GRAPHICS_FT80X_FT80X_H */
