############################################################################
# apps/testing/drivers/drivertest/Makefile
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
############################################################################

include $(APPDIR)/Make.defs

PRIORITY  = $(CONFIG_TESTING_DRIVER_TEST_PRIORITY)
STACKSIZE = $(CONFIG_TESTING_DRIVER_TEST_STACKSIZE)
MODULE    = $(CONFIG_TESTING_DRIVER_TEST)

ifneq ($(CONFIG_TESTING_DRIVER_TEST_SIMPLE),)
MAINSRC  += drivertest_simple.c
PROGNAME += cmocka_driver_simple
endif

ifeq ($(CONFIG_BCH)$(CONFIG_BUILD_FLAT),yy)
CFLAGS += ${INCDIR_PREFIX}${TOPDIR}/fs
PROGNAME += cmocka_driver_block
MAINSRC  += drivertest_block.c
endif

ifneq ($(CONFIG_RTC),)
ifneq ($(CONFIG_SIG_EVTHREAD),)
MAINSRC  += drivertest_rtc.c
PROGNAME += cmocka_driver_rtc
endif
endif

ifneq ($(CONFIG_TIMER),)
MAINSRC  += drivertest_timer.c
PROGNAME += cmocka_driver_timer
endif

ifneq ($(CONFIG_TESTING_ONESHOT_TEST),)
MAINSRC  += drivertest_oneshot.c
PROGNAME += cmocka_driver_oneshot
endif

ifneq ($(CONFIG_DISABLE_POSIX_TIMERS),y)
ifneq ($(CONFIG_SIG_EVTHREAD),)
MAINSRC  += drivertest_posix_timer.c
PROGNAME += cmocka_posix_timer
endif
endif

ifneq ($(CONFIG_WATCHDOG),)
ifneq ($(CONFIG_BOARDCTL_RESET_CAUSE),)
MAINSRC  += drivertest_watchdog.c
PROGNAME += cmocka_driver_watchdog
endif
endif

ifneq ($(CONFIG_PWM),)
MAINSRC  += drivertest_pwm.c
PROGNAME += cmocka_driver_pwm
endif

ifneq ($(CONFIG_ADC),)
MAINSRC  += drivertest_adc.c
PROGNAME += cmocka_driver_adc
endif

ifneq ($(CONFIG_I2C)$(CONFIG_SPI),)
ifneq ($(CONFIG_SENSORS_BMI160),)
MAINSRC  += drivertest_i2c_spi.c
PROGNAME += cmocka_driver_i2c_spi
endif
endif

ifneq ($(CONFIG_I2C_DRIVER)$(CONFIG_I2C_SLAVE_DRIVER),)
MAINSRC  += drivertest_i2c_write.c
PROGNAME += cmocka_driver_i2c_write

MAINSRC  += drivertest_i2c_read.c
PROGNAME += cmocka_driver_i2c_read
endif

ifneq ($(CONFIG_DEV_GPIO),)
PROGNAME += cmocka_driver_gpio
MAINSRC  += drivertest_gpio.c
endif

ifneq ($(CONFIG_RELAY),)
MAINSRC  += drivertest_relay.c
PROGNAME += cmocka_driver_relay
endif

ifneq ($(CONFIG_SERIAL),)
MAINSRC  += drivertest_uart.c
PROGNAME += cmocka_driver_uart
endif

ifneq ($(CONFIG_AUDIO),)
MAINSRC  += drivertest_audio.c
PROGNAME += cmocka_driver_audio
endif

ifneq ($(CONFIG_VIDEO_FB),)
MAINSRC  += drivertest_framebuffer.c
PROGNAME += cmocka_driver_framebuffer
endif

ifneq ($(CONFIG_LCD_DEV),)
MAINSRC  += drivertest_lcd.c
PROGNAME += cmocka_driver_lcd
endif

ifneq ($(CONFIG_REGULATOR),)
MAINSRC  += drivertest_regulator.c
PROGNAME += cmocka_driver_regulator
endif

ifneq ($(CONFIG_LV_USE_NUTTX_TOUCHSCREEN),)
MAINSRC  += drivertest_touchpanel.c
PROGNAME += cmocka_driver_touchpanel
endif

ifeq ($(CONFIG_ARCH_CHIP_MPS2_AN500),y)
ifeq ($(CONFIG_ARCH_IRQPRIO),y)
MAINSRC  += drivertest_mps2.c
PROGNAME += cmocka_driver_mps2
MAINSRC  += drivertest_mps2_zerointerrupt.c
PROGNAME += cmocka_driver_mps2_zerointerrupt
endif
MAINSRC  += drivertest_mps2_isr_signal.c
PROGNAME += cmocka_driver_mps2_isr_signal
endif

ifeq ($(CONFIG_PM),y)
MAINSRC  += drivertest_pm.c
PROGNAME += cmocka_driver_pm
ifeq ($(CONFIG_SMP),y)
MAINSRC  += drivertest_pm_smp.c
PROGNAME += cmocka_driver_pm_smp
endif
endif

ifneq ($(CONFIG_PM_RUNTIME),)
MAINSRC  += drivertest_pm_runtime.c
PROGNAME += cmocka_driver_pm_runtime
endif

include $(APPDIR)/Application.mk
