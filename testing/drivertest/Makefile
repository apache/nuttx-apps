############################################################################
# apps/testing/drivertest/Makefile
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

ifneq ($(CONFIG_RTC),)
MAINSRC  += drivertest_rtc.c
PROGNAME += cmocka_driver_rtc
endif

ifneq ($(CONFIG_TIMER),)
MAINSRC  += drivertest_timer.c
PROGNAME += cmocka_driver_timer
endif

ifneq ($(CONFIG_PWM),)
MAINSRC  += drivertest_pwm.c
PROGNAME += cmocka_driver_pwm
endif

ifneq ($(CONFIG_WATCHDOG),)
MAINSRC  += drivertest_watchdog.c
PROGNAME += cmocka_driver_watchdog
endif

ifneq ($(CONFIG_I2C)$(CONFIG_SPI),)
ifneq ($(CONFIG_SENSORS_BMI160),)
MAINSRC  += drivertest_i2c_spi.c
PROGNAME += cmocka_driver_i2c_spi
endif
endif

ifneq ($(CONFIG_BCH),)
PROGNAME += cmocka_driver_block
MAINSRC  += drivertest_block.c
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

include $(APPDIR)/Application.mk
