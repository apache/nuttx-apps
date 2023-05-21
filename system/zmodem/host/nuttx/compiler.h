/****************************************************************************
 * apps/system/zmodem/host/nuttx/compiler.h
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

#ifndef __APPS_SYSTEM_ZMODEM_HOST_NUTTX_COMPILER_H
#define __APPS_SYSTEM_ZMODEM_HOST_NUTTX_COMPILER_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GCC-specific definitions *************************************************/

#ifdef __GNUC__

/* Pre-processor */

#  define CONFIG_CPP_HAVE_VARARGS 1 /* Supports variable argument macros */
#  define CONFIG_CPP_HAVE_WARNING 1 /* Supports #warning */

/* Intriniscs */

#  define CONFIG_HAVE_FUNCTIONNAME 1 /* Has __FUNCTION__ */
#  define CONFIG_HAVE_FILENAME     1 /* Has __FILE__ */

/* Indicate that a local variable is not used */

#  define UNUSED(a) ((void)(a))

/* Attributes
 *
 * GCC supports weak symbols which can be used to reduce code size because
 * unnecessary "weak" functions can be excluded from the link.
 */

#ifndef __CYGWIN__
#  define CONFIG_HAVE_WEAKFUNCTIONS 1
#  define weak_alias(name, aliasname) \
   extern __typeof (name) aliasname __attribute__ ((weak, alias (#name)));
#  define weak_function __attribute__ ((weak))
#  define weak_const_function __attribute__ ((weak, __const__))
#else
#  undef  CONFIG_HAVE_WEAKFUNCTIONS
#  define weak_alias(name, aliasname)
#  define weak_function
#  define weak_const_function
#endif

/* The noreturn attribute informs GCC that the function will not return. */

#  define noreturn_function __attribute__ ((noreturn))

/* The farcall_function attribute informs GCC that is should use long calls
 * (even though -mlong-calls does not appear in the compilation options)
 */

#  define farcall_function __attribute__ ((long_call))

/* The packed attribute informs GCC that the structure elements are packed,
 * ignoring other alignment rules.
 */

#  define begin_packed_struct
#  define end_packed_struct __attribute__ ((packed))

/* GCC does not support the reentrant attribute */

#  define reentrant_function

/* The naked attribute informs GCC that the programmer will take care of
 * the function prolog and epilog.
 */

#  define naked_function __attribute__ ((naked,no_instrument_function))

/* The inline_function attribute informs GCC that the function should always
 * be inlined, regardless of the level of optimization.  The
 * noinline_function indicates that the function should never be inlined.
 */

#  define inline_function __attribute__ ((always_inline,no_instrument_function))
#  define noinline_function __attribute__ ((noinline))

/* GCC has does not use storage classes to qualify addressing */

#  define FAR
#  define NEAR
#  define DSEG
#  define CODE

/* Handle cases where sizeof(int) is 16-bits, sizeof(long) is 32-bits, and
 * pointers are 16-bits.
 */

#if defined(__m32c__)
/* No I-space access qualifiers */

#  define IOBJ
#  define IPTR

/* Select the small, 16-bit addressing model */

#  define CONFIG_SMALL_MEMORY 1

/* Long and int are not the same size */

#  define CONFIG_LONG_IS_NOT_INT 1

/* Pointers and int are the same size */

# undef  CONFIG_PTR_IS_NOT_INT

#elif defined(__AVR__)
#  if defined(CONFIG_AVR_HAS_MEMX_PTR)
  /* I-space access qualifiers needed by Harvard architecture */

#    define IOBJ __flash
#    define IPTR __memx

#  else
/* No I-space access qualifiers */

#    define IOBJ
#    define IPTR
#  endif

/* Select the small, 16-bit addressing model (for D-Space) */

#  define CONFIG_SMALL_MEMORY 1

/* Long and int are not the same size */

#  define CONFIG_LONG_IS_NOT_INT 1

/* Pointers and int are the same size */

# undef  CONFIG_PTR_IS_NOT_INT

/* Uses a 32-bit FAR pointer only from accessing data outside of the 16-bit
 * data space.
 */

#  define CONFIG_HAVE_FARPOINTER 1

#elif defined(__mc68hc1x__)
/* No I-space access qualifiers */

#  define IOBJ
#  define IPTR

/* Select the small, 16-bit addressing model */

#  define CONFIG_SMALL_MEMORY 1

/* Normally, mc68hc1x code is compiled with the -mshort option
 * which results in a 16-bit integer.  If -mnoshort is defined
 * then an integer is 32-bits.  GCC will defined __INT__ accordingly:
 */

#  if __INT__ == 16
/* int is 16-bits, long is 32-bits */

#    define CONFIG_LONG_IS_NOT_INT 1

/* Pointers and int are the same size (16-bits) */

#    undef  CONFIG_PTR_IS_NOT_INT
#  else
/* int and long are both 32-bits */

#    undef  CONFIG_LONG_IS_NOT_INT

/* Pointers and int are NOT the same size */

#    define CONFIG_PTR_IS_NOT_INT 1
#  endif

#else

/* No I-space access qualifiers */

#  define IOBJ
#  define IPTR

/* Select the large, 32-bit addressing model */

# undef  CONFIG_SMALL_MEMORY

/* Long and int are (probably) the same size (32-bits) */

# undef  CONFIG_LONG_IS_NOT_INT

/* Pointers and int are the same size (32-bits) */

# undef  CONFIG_PTR_IS_NOT_INT
#endif

/* GCC supports both types double and long long */

#  define CONFIG_HAVE_LONG_LONG 1
#  define CONFIG_HAVE_FLOAT 1
#  define CONFIG_HAVE_DOUBLE 1
#  define CONFIG_HAVE_LONG_DOUBLE 1

/* SDCC-specific definitions ************************************************/

#elif defined(SDCC)

/* Pre-processor */

#  define CONFIG_CPP_HAVE_VARARGS 1 /* Supports variable argument macros */
#  define CONFIG_CPP_HAVE_WARNING 1 /* Supports #warning */

/* Intriniscs */

#  define CONFIG_HAVE_FUNCTIONNAME 1 /* Has __FUNCTION__ */
#  define CONFIG_HAVE_FILENAME     1 /* Has __FILE__ */

/* Pragmas
 *
 * Disable warnings for unused function arguments.
 */

# pragma disable_warning 85

/* Attributes
 *
 * SDCC does not support weak symbols.
 */

# undef  CONFIG_HAVE_WEAKFUNCTIONS
#  define weak_alias(name, aliasname)
#  define weak_function
#  define weak_const_function
#  define restrict /* REVISIT */

/* SDCC does not support the noreturn or packed attributes */

#  define noreturn_function
#  define begin_packed_struct
#  define end_packed_struct

/* REVISIT: */

#  define farcall_function

/* SDCC does support "naked" functions */

#  define naked_function __naked

/* SDCC does not support forced inlining. */

#  define inline_function
#  define noinline_function

/* The reentrant attribute informs SDCC that the function
 * must be reentrant.  In this case, SDCC will store input
 * arguments on the stack to support reentrancy.
 */

#  define reentrant_function __reentrant

/* It is assumed that the system is build using the small
 * data model with storage defaulting to internal RAM.
 * The NEAR storage class can also be used to address data
 * in internal RAM; FAR can be used to address data in
 * external RAM.
 */

#if defined(__SDCC_z80) || defined(__SDCC_z180) || defined(__SDCC_gbz80)
#  define FAR
#  define NEAR
#  define CODE
#  define DSEG
#else
#  define FAR    __xdata
#  define NEAR   __data
#  define CODE   __code
#  if defined(SDCC_MODEL_SMALL)
#    define DSEG __data
#  else
#    define DSEG __xdata
#  endif
#endif

/* Select small, 16-bit address model */

#  define CONFIG_SMALL_MEMORY 1

/* Long and int are not the same size */

#  define CONFIG_LONG_IS_NOT_INT 1

/* The generic pointer and int are not the same size
 * (for some SDCC architectures)
 */

#if !defined(__z80) && !defined(__gbz80)
#  define CONFIG_PTR_IS_NOT_INT 1
#endif

/* SDCC does not support type long long or type double */

# undef  CONFIG_HAVE_LONG_LONG
#  define CONFIG_HAVE_FLOAT 1
# undef  CONFIG_HAVE_DOUBLE
# undef  CONFIG_HAVE_LONG_DOUBLE

/* Zilog-specific definitions ***********************************************/

#elif defined(__ZILOG__)

/* At present, only the following Zilog compilers are recognized */

#  if !defined(__ZNEO__) && !defined(__EZ8__) && !defined(__EZ80__)
#    warning "Unrecognized Zilog compiler"
#  endif

/* Pre-processor */

# undef CONFIG_CPP_HAVE_VARARGS /* No variable argument macros */
# undef CONFIG_CPP_HAVE_WARNING /* Does not support #warning */

/* Intrinsics */

#  define CONFIG_HAVE_FUNCTIONNAME 1 /* Has __FUNCTION__ */
#  define CONFIG_HAVE_FILENAME     1 /* Has __FILE__ */

/* No I-space access qualifiers */

#  define IOBJ
#  define IPTR

/* Attributes
 *
 * The Zilog compiler does not support weak symbols
 */

# undef  CONFIG_HAVE_WEAKFUNCTIONS
#  define weak_alias(name, aliasname)
#  define weak_function
#  define weak_const_function
#  define restrict

/* The Zilog compiler does not support the noreturn, packed, naked
 * attributes.
 */

#  define noreturn_function
#  define begin_packed_struct
#  define end_packed_struct
#  define naked_function
#  define inline_function
#  define noinline_function

/* REVISIT: */

#  define farcall_function

/* The Zilog compiler does not support the reentrant attribute */

#  define reentrant_function

/* Addressing.
 *
 * Z16F ZNEO:  Far is 24-bits; near is 16-bits of address.
 *             The supported model is (1) all code on ROM, and (2) all data
 *             and stacks in external (far) RAM.
 * Z8Encore!:  Far is 16-bits; near is 8-bits of address.
 *             The supported model is (1) all code on ROM, and (2) all data
 *             and stacks in internal (far) RAM.
 * Z8Acclaim:  In Z80 mode, all pointers are 16-bits.  In ADL mode, all
 *             pointers are 24 bits.
 */

#  if defined(__ZNEO__)
#    define FAR   _Far
#    define NEAR  _Near
#    define DSEG  _Far
#    define CODE  _Erom
#    undef  CONFIG_SMALL_MEMORY       /* Select the large, 32-bit addressing model */
#    undef  CONFIG_LONG_IS_NOT_INT    /* Long and int are the same size */
#    undef  CONFIG_PTR_IS_NOT_INT     /* FAR pointers and int are the same size */
#  elif defined(__EZ8__)
#    define FAR   far
#    define NEAR  near
#    define DSEG  far
#    define CODE  rom
#    define CONFIG_SMALL_MEMORY 1     /* Select small, 16-bit address model */
#    define CONFIG_LONG_IS_NOT_INT 1  /* Long and int are not the same size */
#    undef  CONFIG_PTR_IS_NOT_INT     /* FAR pointers and int are the same size */
#  elif defined(__EZ80__)
#    define FAR
#    define NEAR
#    define DSEG
#    define CODE
#    undef  CONFIG_SMALL_MEMORY       /* Select the large, 32-bit addressing model */
#    define CONFIG_LONG_IS_NOT_INT 1  /* Long and int are not the same size */
#    ifdef CONFIG_EZ80_Z80MODE
#      define CONFIG_PTR_IS_NOT_INT 1 /* Pointers and int are not the same size */
#    else
#      undef  CONFIG_PTR_IS_NOT_INT   /* Pointers and int are the same size */
#    endif
#  endif

/* Older Zilog compilers support both types double and long long, but the
 * size is 32-bits (same as long and single precision) so it is safer to
 * say that they are not supported.  Later versions are more ANSII compliant
 * and simply do not support long long or double.
 */

# undef  CONFIG_HAVE_LONG_LONG
#  define CONFIG_HAVE_FLOAT 1
# undef  CONFIG_HAVE_DOUBLE
# undef  CONFIG_HAVE_LONG_DOUBLE

/* ICCARM-specific definitions **********************************************/

#elif defined(__ICCARM__)

#  define CONFIG_CPP_HAVE_VARARGS 1 /* Supports variable argument macros */
#  define CONFIG_HAVE_FILENAME 1    /* Has __FILE__ */
#  define CONFIG_HAVE_FLOAT 1

/* Indicate that a local variable is not used */

#  define UNUSED(a) ((void)(a))

#  define weak_alias(name, aliasname)
#  define weak_function        __weak
#  define weak_const_function
#  define noreturn_function
#  define farcall_function
#  define begin_packed_struct  __packed
#  define end_packed_struct
#  define reentrant_function
#  define naked_function
#  define inline_function
#  define noinline_function

#  define FAR
#  define NEAR
#  define DSEG
#  define CODE
#  define IPTR

#  define __asm__       asm
#  define __volatile__  volatile

/* For operatots __sfb() and __sfe() */

# pragma section = ".bss"
# pragma section = ".data"
# pragma section = ".data_init"
# pragma section = ".text"

/* Unknown compiler *********************************************************/

#else
# warning Unknown Compiler

# undef  CONFIG_CPP_HAVE_VARARGS
# undef  CONFIG_CPP_HAVE_WARNING
# undef  CONFIG_HAVE_FUNCTIONNAME
# undef  CONFIG_HAVE_FILENAME
# undef  CONFIG_HAVE_WEAKFUNCTIONS
#  define weak_alias(name, aliasname)
#  define weak_function
#  define weak_const_function
#  define restrict
#  define noreturn_function
#  define farcall_function
#  define begin_packed_struct
#  define end_packed_struct
#  define reentrant_function
#  define naked_function
#  define inline_function
#  define noinline_function

#  define FAR
#  define NEAR
#  define DSEG
#  define CODE

# undef  CONFIG_SMALL_MEMORY
# undef  CONFIG_LONG_IS_NOT_INT
# undef  CONFIG_PTR_IS_NOT_INT
# undef  CONFIG_HAVE_LONG_LONG
#  define CONFIG_HAVE_FLOAT 1
# undef  CONFIG_HAVE_DOUBLE
# undef  CONFIG_HAVE_LONG_DOUBLE

#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#endif /* __APPS_SYSTEM_ZMODEM_HOST_NUTTX_COMPILER_H */
