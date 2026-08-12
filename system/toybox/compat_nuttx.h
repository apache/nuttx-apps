/****************************************************************************
 * apps/system/toybox/compat_nuttx.h
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

/* Force-included ahead of every Toybox translation unit (../Makefile:
 * CFLAGS += -include compat_nuttx.h), same as e.g. apps/crypto/mbedtls does
 * for its own NuttX shim. Toybox's own portability layer (lib/portability.h)
 * already branches on __linux__/__APPLE__/__FreeBSD__/etc; this file is only
 * for the handful of gaps that don't fit that model cleanly enough to carry
 * as a source patch instead -- prefer a fix under ../patch/ when there's a
 * clear single call site to patch (see ../patch/ for the existing ones).
 *
 * Earlier iterations of this port carried a large set of workarounds (a
 * synthetic fd table, #define open/fstatat/fdopendir redirections) for VFS
 * gaps that no longer reproduce against current NuttX (dirfd()/fdopendir()/
 * O_DIRECTORY all work normally). Add fixes here only for problems actually
 * observed against the current NuttX tree, not preemptively.
 */

#ifndef __APPS_SYSTEM_TOYBOX_COMPAT_NUTTX_H
#define __APPS_SYSTEM_TOYBOX_COMPAT_NUTTX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/* lib/portability.h #define's the bare identifier strncat(...) to a
 * poison name (catching accidental unsafe calls from Toybox's own code --
 * see the comment there), unconditionally on every platform. NuttX's own
 * <string.h> textually defines a function literally named strncat as part
 * of its __builtin_strncat() optimization (BUILTIN_FUNCTION(strncat) ...),
 * which the poison macro corrupts if <string.h> is first parsed *after*
 * that macro is active -- pre-including it here, before toys.h reaches
 * portability.h, makes NuttX's own header guard skip it the second time
 * around instead.
 */

#include <string.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* toy_list[0]'s NEWTOY entry -- the multiplexer's own self-entry -- is
 * renamed from "toybox" to "toybox_multiplex" by scripts/make.sh (see
 * ../patch/0002-*.patch) to avoid a symbol collision: NEWTOY's name##_main
 * convention would otherwise declare a "void toybox_main(void)" in every
 * Toybox translation unit (via toys.h's default NEWTOY macro), conflicting
 * with the "int toybox_main(int, char **)" NuttX's sys/types.h forward-
 * declares everywhere once CONFIG_INIT_ENTRYPOINT="toybox_main" is set --
 * that's our actual task entry point, toybox_entry.c.
 *
 * The rename leaves two macros unresolved that would normally come from
 * generated/help.h and generated/flags.h (both driven by Config.in's
 * "config TOYBOX" stanza, which still says "toybox", not the renamed
 * identifier). The multiplexer entry has TOYFLAG_NOHELP and opts==0 (no
 * option string), so both are safe to stub directly instead of aliasing
 * through to the real (still-"toybox"-named) generated macros.
 */
#define HELP_toybox_multiplex "\0"
#define OPTSTR_toybox_multiplex 0

#endif /* __APPS_SYSTEM_TOYBOX_COMPAT_NUTTX_H */
