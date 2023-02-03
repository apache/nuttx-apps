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

include $(APPDIR)/Application.mk
