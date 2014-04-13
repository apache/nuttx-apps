/****************************************************************************
 * include/nuttx/compiler.h
 *
 *   Copyright (C) 2013 Gregory Nutt. All rights reserved.
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

#ifndef __APPS_SYSTEM_ZMODEM_HOST_NUTTX_COMPILER_H
#define __APPS_SYSTEM_ZMODEM_HOST_NUTTX_COMPILER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Definitions
 ****************************************************************************/

/* GCC-specific definitions *************************************************/

#ifdef __GNUC__

/* Pre-processor */

# define CONFIG_CPP_HAVE_VARARGS 1 /* Supports variable argument macros */
# define CONFIG_CPP_HAVE_WARNING 1 /* Supports #warning */

/* Intriniscs */

# define CONFIG_HAVE_FUNCTIONNAME 1 /* Has __FUNCTION__ */
# define CONFIG_HAVE_FILENAME     1 /* Has __FILE__ */

/* Attributes
 *
 * GCC supports weak symbols which can be used to reduce code size because
 * unnecessary "weak" functions can be excluded from the link.
 */

# ifndef __CYGWIN__
#  define CONFIG_HAVE_WEAKFUNCTIONS 1
#  define weak_alias(name, aliasname) \
   extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));
#  define weak_function __attribute__ ((weak))
#  define weak_const_function __attribute__ ((weak, __const__))
# else
#  undef  CONFIG_HAVE_WEAKFUNCTIONS
#  define weak_alias(name, aliasname)
#  define weak_function
#  define weak_const_function
# endif

/* The noreturn attribute informs GCC that the function will not return. */

# define noreturn_function __attribute__ ((noreturn))

/* The farcall_function attribute informs GCC that is should use long calls
 * (even though -mlong-calls does not appear in the compilation options)
 */

# define farcall_function __attribute__ ((long_call))

/* The packed attribute informs GCC that the stucture elements are packed,
 * ignoring other alignment rules.
 */

# define packed_struct __attribute__ ((packed))

/* GCC does not support the reentrant attribute */

# define reentrant_function

/* The naked attribute informs GCC that the programmer will take care of
 * the function prolog and epilog.
 */

# define naked_function __attribute__ ((naked,no_instrument_function))

/* The inline_function attribute informs GCC that the function should always
 * be inlined, regardless of the level of optimization.  The noinline_function
 * indicates that the function should never be inlined.
 */

# define inline_function __attribute__ ((always_inline))
# define noinline_function __attribute__ ((noinline))

/* GCC has does not use storage classes to qualify addressing */

# define FAR
# define NEAR
# define DSEG
# define CODE

/* Unknown compiler *********************************************************/

#else
# warning Unknown Compiler

# undef  CONFIG_CPP_HAVE_VARARGS
# undef  CONFIG_CPP_HAVE_WARNING
# undef  CONFIG_HAVE_FUNCTIONNAME
# undef  CONFIG_HAVE_FILENAME
# undef  CONFIG_HAVE_WEAKFUNCTIONS
# define weak_alias(name, aliasname)
# define weak_function
# define weak_const_function
# define noreturn_function
# define farcall_function
# define packed_struct
# define reentrant_function
# define naked_function
# define inline_function
# define noinline_function

# define FAR
# define NEAR
# define DSEG
# define CODE

# undef  CONFIG_SMALL_MEMORY
# undef  CONFIG_LONG_IS_NOT_INT
# undef  CONFIG_PTR_IS_NOT_INT
# undef  CONFIG_HAVE_INLINE
# define inline 1
# undef  CONFIG_HAVE_LONG_LONG
# define CONFIG_HAVE_FLOAT 1
# undef  CONFIG_HAVE_DOUBLE
# undef  CONFIG_HAVE_LONG_DOUBLE
# undef  CONFIG_CAN_PASS_STRUCTS

#endif

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Global Function Prototypes
 ****************************************************************************/

#endif /* __APPS_SYSTEM_ZMODEM_HOST_NUTTX_COMPILER_H */
