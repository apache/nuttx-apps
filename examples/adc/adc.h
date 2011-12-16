/****************************************************************************
 * examples/examples/adc.h
 *
 *   Copyright (C) 2011 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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

#ifndef __APPS_EXAMPLES_ADC_ADC_H
#define __APPS_EXAMPLES_ADC_ADC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Definitions
 ****************************************************************************/
/* Configuration ************************************************************/
/* CONFIG_NSH_BUILTIN_APPS - Build the ADC test as an NSH built-in function.
 *  Default: Built as a standalone problem
 * CONFIG_EXAMPLES_ADC_DEVPATH - The path to the ADC device. Default: /dev/adc0
 * CONFIG_EXAMPLES_ADC_NSAMPLES - If CONFIG_NSH_BUILTIN_APPS
 *   is defined, then the number of samples is provided on the command line
 *   and this value is ignored.  Otherwise, this number of samples is
 *   collected and the program terminates.  Default:  Samples are collected
 *   indefinitely.
 * CONFIG_EXAMPLES_ADC_GROUPSIZE - The number of samples to read at once.
 *   Default: 4
 * CONFIG_EXAMPLES_ADC_SAMPLEWIDTH - The width (in bits) of the on ADC sample.
 *   Default: 16
 */

#ifndef CONFIG_ADC
#  error "ADC device support is not enabled (CONFIG_ADC)"
#endif

#ifndef CONFIG_EXAMPLES_ADC_DEVPATH
#  define CONFIG_EXAMPLES_ADC_DEVPATH "/dev/adc0"
#endif

#ifndef CONFIG_EXAMPLES_ADC_GROUPSIZE
#  define CONFIG_EXAMPLES_ADC_GROUPSIZE 4
#endif

#ifndef CONFIG_EXAMPLES_ADC_SAMPLEWIDTH
#  define CONFIG_EXAMPLES_ADC_SAMPLEWIDTH 16
#endif

/* Sample characteristics ***************************************************/

#undef ADC_SAMPLE_BYTEWIDTH
#if CONFIG_EXAMPLES_ADC_SAMPLEWIDTH <= 8
#  undef CONFIG_EXAMPLES_ADC_SAMPLEWIDTH
#  define CONFIG_EXAMPLES_ADC_SAMPLEWIDTH 8
#  define ADC_SAMPLE_BYTEWIDTH 1
#  define SAMPLE_FMT "%02x"
#elif CONFIG_EXAMPLES_ADC_SAMPLEWIDTH <= 16
#  undef CONFIG_EXAMPLES_ADC_SAMPLEWIDTH
#  define CONFIG_EXAMPLES_ADC_SAMPLEWIDTH 16
#  define ADC_SAMPLE_BYTEWIDTH 2
#  define SAMPLE_FMT "%04x"
#elif CONFIG_EXAMPLES_ADC_SAMPLEWIDTH <= 24
#  undef CONFIG_EXAMPLES_ADC_SAMPLEWIDTH
#  define CONFIG_EXAMPLES_ADC_SAMPLEWIDTH 24
#  define ADC_SAMPLE_BYTEWIDTH 3
#  define SAMPLE_FMT "%06x"
#elif CONFIG_EXAMPLES_ADC_SAMPLEWIDTH <= 32
#  undef CONFIG_EXAMPLES_ADC_SAMPLEWIDTH
#  define CONFIG_EXAMPLES_ADC_SAMPLEWIDTH 32
#  define ADC_SAMPLE_BYTEWIDTH 4
#  define SAMPLE_FMT "%08x"
#else
#  error "Unsupported sample width"
#endif

#undef ADC_SAMPLE_SIZE (CONFIG_EXAMPLES_ADC_GROUPSIZE * ADC_SAMPLE_BYTEWIDTH)

/* Debug ********************************************************************/

#ifdef CONFIG_CPP_HAVE_VARARGS
#  ifdef CONFIG_DEBUG
#    define message(...) lib_rawprintf(__VA_ARGS__)
#    define msgflush()
#  else
#    define message(...) printf(__VA_ARGS__)
#    define msgflush() fflush(stdout)
#  endif
#else
#  ifdef CONFIG_DEBUG
#    define message lib_rawprintf
#    define msgflush()
#  else
#    define message printf
#    define msgflush() fflush(stdout)
#  endif
#endif

/****************************************************************************
 * Public Types
 ****************************************************************************/

#if CONFIG_EXAMPLES_ADC_SAMPLEWIDTH == 8
typedef uint8_t adc_sample_t;
#elif CONFIG_EXAMPLES_ADC_SAMPLEWIDTH == 16
typedef uint16_t adc_sample_t;
#elif CONFIG_EXAMPLES_ADC_SAMPLEWIDTH == 24
typedef uint32_t adc_sample_t;
#elif CONFIG_EXAMPLES_ADC_SAMPLEWIDTH == 32
typedef uint32_t adc_sample_t;
#else
#  error "Unsupported sample width"
#endif

/****************************************************************************
 * Public Variables
 ****************************************************************************/

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: adc_devinit()
 *
 * Description:
 *   Perform architecuture-specific initialization of the ADC hardware.  This
 *   interface must be provided by all configurations using apps/examples/adc
 *
 ****************************************************************************/

int adc_devinit(void);

#endif /* __APPS_EXAMPLES_ADC_ADC_H */
